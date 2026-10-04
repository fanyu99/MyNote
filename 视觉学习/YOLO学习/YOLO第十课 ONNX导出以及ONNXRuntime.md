# 1.PythonYOLO导出ONNX文件
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

