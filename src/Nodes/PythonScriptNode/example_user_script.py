# init_interface(): 首次加载或脚本改变时调用，可用于构建界面。可选。
def init_interface():
    # 这里可创建/更新 UI（通过 C++ 暴露的控件 API），示例不做 UI。
    print("Python界面初始化完成")

# input_event_handler(index): 每次某个输入端口更新时触发
def input_event_handler(index):
    # 读取输入端口数据（QVariantMap 已转换为 Python dict/嵌套结构）
    data = worker.get_input_value(index)
    print(f"输入端口 {index} 的数据:", data)

    # 假设 Data Info 结构中 default.channels 是一个列表
    default = data.get("default", {})
    channels = default.get("channels", [])

    # 简单处理：把每个通道值 +1
    new_channels = [(x + 1) for x in channels] if isinstance(channels, list) else []

    # 组织输出结构（会自动转换为 QVariantMap）
    result = {
        "default": {
            "channels": new_channels,
            "channelCount": len(new_channels),
        }
    }

    # 把处理结果写到输出端口 0
    worker.set_output_value(0, result)