"""把 YOLOE 导出成运行时仍可更换参考图的两个 ONNX。

参考图不会写进权重。登记照片时走 yoloe-prompt.onnx，每帧检测走 yoloe-detect.onnx，
检测模型额外接收当前各 ID 的向量。

输出：
  yoloe-prompt.onnx
    image  float32 [1,3,640,640]   RGB，0-1
    mask   float32 [1,1,640,640]   参考图有效区域为 1，letterbox 灰边为 0
    embedding float32 [1,D]        L2 归一化
  yoloe-detect.onnx
    image  float32 [1,3,640,640]
    embeddings float32 [1,num_ids,D]
    output0 float32 [1,4+num_ids,anchors]   640 画布上的 xywh + 各类别概率

用法（在已安装 torch 与 ultralytics 的环境里）：
  python export_yoloe_onnx.py --out ../../../plugins/Models
"""

from __future__ import annotations

import argparse
from pathlib import Path

import torch
import torch.nn.functional as F


def unwrap_prediction(output):
    if isinstance(output, (tuple, list)):
        first = output[0]
        if isinstance(first, (tuple, list)):
            return first[0]
        return first
    return output


def load_yoloe(weights: str):
    from ultralytics import YOLOE

    wrapper = YOLOE(weights)
    net = wrapper.model
    net.eval()
    net.cpu()
    for parameter in net.parameters():
        parameter.requires_grad_(False)
    return net


def discover_mask_size(net, image: torch.Tensor) -> tuple[int, int]:
    height, width = int(image.shape[-2]), int(image.shape[-1])
    candidates = [(height, width)]
    for divisor in (4, 8, 16, 32):
        candidates.append((max(1, height // divisor), max(1, width // divisor)))
    last_error = None
    for mask_h, mask_w in candidates:
        visuals = torch.ones(1, 1, mask_h, mask_w)
        try:
            with torch.no_grad():
                embedding = net.predict(image, vpe=visuals, return_vpe=True)
            if embedding.ndim == 3 and embedding.shape[0] == 1 and embedding.shape[1] >= 1 and embedding.shape[-1] > 0:
                print(f"visual prompt mask size: {mask_h}x{mask_w}, embed {embedding.shape[-1]}")
                return mask_h, mask_w
        except Exception as exc:  # 探测空间尺寸，失败就换下一个
            last_error = exc
    raise RuntimeError(f"YOLOE 不接受这些视觉提示尺寸: {last_error}")


def patch_dynamic_prompt_head():
    from ultralytics.nn.modules.head import YOLOEDetect

    original = YOLOEDetect.forward_head

    def forward_head(self, x, box_head, cls_head, contrastive_head):
        batch = x[0].shape[0]
        boxes = torch.cat(
            [box_head[i](x[i]).view(batch, 4 * self.reg_max, -1) for i in range(self.nl)],
            dim=-1,
        )
        class_count = x[-1].shape[1]
        scores = torch.cat(
            [
                contrastive_head[i](cls_head[i](x[i]), x[-1]).reshape(batch, class_count, -1)
                for i in range(self.nl)
            ],
            dim=-1,
        )
        return {"boxes": boxes, "scores": scores, "feats": x[:3]}

    YOLOEDetect.forward_head = forward_head
    return original, YOLOEDetect


class PromptEncoder(torch.nn.Module):
    def __init__(self, net, mask_hw: tuple[int, int]):
        super().__init__()
        self.net = net
        self.mask_hw = mask_hw

    def forward(self, image, mask):
        if mask.shape[-2] != self.mask_hw[0] or mask.shape[-1] != self.mask_hw[1]:
            mask = F.interpolate(mask, size=self.mask_hw, mode="nearest")
        embedding = self.net.predict(image, vpe=mask, return_vpe=True)
        return F.normalize(embedding[:, 0, :], p=2, dim=-1)


class Detector(torch.nn.Module):
    def __init__(self, net, channel_major: bool):
        super().__init__()
        self.net = net
        self.channel_major = channel_major

    def forward(self, image, embeddings):
        embeddings = F.normalize(embeddings, p=2, dim=-1)
        prediction = unwrap_prediction(self.net.predict(image, vpe=embeddings))
        if self.channel_major:
            class_count = embeddings.shape[1]
            return prediction[:, : 4 + class_count, :]
        return prediction


def export_onnx(module, args, path: Path, input_names, output_names, dynamic_axes):
    path.parent.mkdir(parents=True, exist_ok=True)
    kwargs = dict(
        input_names=input_names,
        output_names=output_names,
        opset_version=17,
        do_constant_folding=True,
        dynamo=False,
    )
    if dynamic_axes:
        kwargs["dynamic_axes"] = dynamic_axes
    try:
        torch.onnx.export(module, args, str(path), **kwargs)
    except TypeError:
        kwargs.pop("dynamo", None)
        torch.onnx.export(module, args, str(path), **kwargs)


def session_runnable(path: Path, feeds: dict) -> bool:
    try:
        import onnxruntime as ort
    except ImportError:
        print("未安装 onnxruntime，跳过导出后的形状检查")
        return True
    try:
        session = ort.InferenceSession(str(path), providers=["CPUExecutionProvider"])
        session.run(None, feeds)
        return True
    except Exception as exc:
        print(f"动态 ID 检查失败: {exc}")
        return False


def main():
    parser = argparse.ArgumentParser(description="导出可在运行时更换参考图的 YOLOE ONNX")
    parser.add_argument("--weights", default="yoloe-11s-seg.pt")
    parser.add_argument("--out", type=Path, default=Path("./plugins/Models"))
    parser.add_argument("--imgsz", type=int, default=640)
    args = parser.parse_args()

    torch.set_grad_enabled(False)
    net = load_yoloe(args.weights)
    head = net.model[-1]
    head.export = False
    if getattr(head, "end2end", False):
        print("当前权重是 end-to-end 头，检测输出会是框列表而不是密集网格")

    image = torch.rand(1, 3, args.imgsz, args.imgsz)
    mask_hw = discover_mask_size(net, image)
    original_forward, head_cls = patch_dynamic_prompt_head()
    try:
        prompt = PromptEncoder(net, mask_hw).eval()
        mask = torch.ones(1, 1, args.imgsz, args.imgsz)
        with torch.no_grad():
            embedding = prompt(image, mask)
        embed_dim = int(embedding.shape[-1])
        print(f"prompt embedding: {tuple(embedding.shape)}")

        sample_ids = 2
        sample_embeddings = F.normalize(torch.rand(1, sample_ids, embed_dim), dim=-1)
        with torch.no_grad():
            sample_output = unwrap_prediction(net.predict(image, vpe=sample_embeddings))
        print(f"detect raw output: {tuple(sample_output.shape)}")
        channel_major = sample_output.ndim == 3 and sample_output.shape[1] < sample_output.shape[2]
        detector = Detector(net, channel_major).eval()

        prompt_path = args.out / "yoloe-prompt.onnx"
        detect_path = args.out / "yoloe-detect.onnx"
        export_onnx(
            prompt,
            (image, mask),
            prompt_path,
            ["image", "mask"],
            ["embedding"],
            None,
        )
        export_onnx(
            detector,
            (image, sample_embeddings),
            detect_path,
            ["image", "embeddings"],
            ["output0"],
            {"embeddings": {1: "num_ids"}, "output0": {1: "box_and_ids"}},
        )

        one = torch.rand(1, 1, embed_dim).numpy()
        four = torch.rand(1, 4, embed_dim).numpy()
        image_np = image.numpy()
        dynamic_ok = session_runnable(detect_path, {"image": image_np, "embeddings": one}) and session_runnable(
            detect_path, {"image": image_np, "embeddings": four}
        )
        if not dynamic_ok:
            print("动态 ID 数量不可用，改为固定 16 个 ID 槽位重新导出")
            fixed = F.normalize(torch.rand(1, 16, embed_dim), dim=-1)
            export_onnx(
                detector,
                (image, fixed),
                detect_path,
                ["image", "embeddings"],
                ["output0"],
                None,
            )
            print("检测模型固定为 16 个 ID。更换参考图不用重新导出，同时跟踪的 ID 不超过 16 个。")
        else:
            print("检测模型的 ID 数量是动态的，更换参考图不用重新导出。")
        print(f"已写入 {prompt_path}")
        print(f"已写入 {detect_path}")
    finally:
        head_cls.forward_head = original_forward


if __name__ == "__main__":
    main()
