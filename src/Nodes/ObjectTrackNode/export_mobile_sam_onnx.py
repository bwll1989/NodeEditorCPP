"""把 MobileSAM 导出成点击分割 ONNX。

只在用户点击时运行。跟踪仍使用 YOLOE。

输入：
  image  float32 [1,3,1024,1024]  RGB，0-255，左上对齐，不足处补 0
  point  float32 [1,2]            点击坐标，单位是 1024 画布上的像素
输出：
  masks  float32 [1,3,1024,1024]  三张候选掩码的 logits
  scores float32 [1,3]            每张掩码的质量分

用法：
  python export_mobile_sam_onnx.py --out ../../../build/bin/plugins/Models
"""

from __future__ import annotations

import argparse
from pathlib import Path

import torch
import torch.nn.functional as F
from ultralytics import SAM
from ultralytics.models.sam.modules.encoders import PromptEncoder


def embed_points_onnx(self, points, labels, pad):
    points = points + 0.5
    if pad:
        padding_point = torch.zeros((points.shape[0], 1, 2), dtype=points.dtype, device=points.device)
        padding_label = -torch.ones((labels.shape[0], 1), dtype=labels.dtype, device=labels.device)
        points = torch.cat([points, padding_point], dim=1)
        labels = torch.cat([labels, padding_label], dim=1)
    point_embedding = self.pe_layer.forward_with_coords(points, self.input_image_size)
    labels = labels.unsqueeze(-1)
    not_point = labels == -1
    point_embedding = torch.where(not_point, self.not_a_point_embed.weight.view(1, 1, -1), point_embedding)
    point_embedding = point_embedding + (labels == 0).to(point_embedding.dtype) * self.point_embeddings[0].weight.view(1, 1, -1)
    point_embedding = point_embedding + (labels == 1).to(point_embedding.dtype) * self.point_embeddings[1].weight.view(1, 1, -1)
    return point_embedding


class ClickSegment(torch.nn.Module):
    def __init__(self, sam):
        super().__init__()
        self.image_encoder = sam.image_encoder
        self.prompt_encoder = sam.prompt_encoder
        self.mask_decoder = sam.mask_decoder
        self.register_buffer("pixel_mean", sam.pixel_mean.detach().clone())
        self.register_buffer("pixel_std", sam.pixel_std.detach().clone())

    def forward(self, image, point):
        features = self.image_encoder((image - self.pixel_mean) / self.pixel_std)
        coords = point.view(1, 1, 2)
        labels = torch.ones((1, 1), dtype=torch.int64, device=point.device)
        sparse = self.prompt_encoder._embed_points(coords, labels, True)
        dense = self.prompt_encoder.no_mask_embed.weight.reshape(1, -1, 1, 1).expand(
            1, -1, self.prompt_encoder.image_embedding_size[0], self.prompt_encoder.image_embedding_size[1]
        )
        masks, scores = self.mask_decoder.predict_masks(
            image_embeddings=features,
            image_pe=self.prompt_encoder.get_dense_pe(),
            sparse_prompt_embeddings=sparse,
            dense_prompt_embeddings=dense,
        )
        masks = F.interpolate(masks[:, 1:], size=(1024, 1024), mode="bilinear", align_corners=False)
        return masks, scores[:, 1:]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--weights", default="mobile_sam.pt")
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / "mobile-sam-click.onnx"

    PromptEncoder._embed_points = embed_points_onnx
    sam = SAM(args.weights)
    model = ClickSegment(sam.model).eval()
    image = torch.zeros(1, 3, 1024, 1024)
    point = torch.tensor([[512.0, 512.0]])
    with torch.no_grad():
        masks, scores = model(image, point)
    print("torch", tuple(masks.shape), tuple(scores.shape))

    torch.onnx.export(
        model,
        (image, point),
        str(out_path),
        input_names=["image", "point"],
        output_names=["masks", "scores"],
        opset_version=17,
        dynamo=False,
    )
    print("wrote", out_path, out_path.stat().st_size)


if __name__ == "__main__":
    main()
