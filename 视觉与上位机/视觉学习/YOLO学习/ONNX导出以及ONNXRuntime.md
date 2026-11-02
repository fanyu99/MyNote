---
title: ONNX 导出与 ONNXRuntime
type: 课程笔记
topic: 视觉与上位机
status: 已完成
created: 2026-09-23
tags:
  - yolo
  - onnx
series: YOLO 目标检测
lessons: "10"
order: 10
canvas: 白板/YOLO.canvas
---

# ONNX 导出与 ONNXRuntime

> YOLO 模型通过 ultralytics 与 PyTorch 两条路径导出 ONNX，并用 ONNXRuntime 推理。

## 1- 使用YOLO`ultralytics`导出
```python
from ultralytics import YOLO
model = YOLO("yolov26n.pt"); # 模型配置路径
path = model.export(
	format = "onnx",
	imgsz = 640, # 和训练/推理的尺寸一致
	batch = 1, # 批次大小
	dynamic = False, # 固定尺寸使用False,动态尺寸使用True
	simplify = True, # 使用onnxslim 进行去冗余节点
	opset = 17, # ONNX算子集版本,一般使用较新的
	nms = False, # 默认不把NMS拷进图
	device = 0 # 使用GPU
)
```
## 2- 使用Pytorch导出
```python
import torch
class YOLOModel(torch.nn.Module):
    ...
m = YOLOModel().eval()
dummy = torch.randn(1,3,640,640)
torch.onnx.export(
    m, dummy, "mymodel.onnx",
    input_names=["images"], output_names=["output"],
    dynamic_axes={"images":{0:"batch",2:"h",3:"w"}, "output":{0:"batch"}} if dynamic else None,
    opset_version=17, do_constant_folding=True,
)
```