---
title: YOLO 第七课：模型加载与图片推理
type: 课程笔记
课程: YOLO 目标检测
课次: 7
created: 2026-09-20
updated: 2026-09-20
tags:
  - yolo
  - 目标检测
  - 深度学习
  - 课程笔记
  - 推理
status: 已完成
---

# 第七课：模型加载与图片推理

> [!abstract] 本课目标
> 学会加载训练好的权重，用 `model.predict()` 对图片推理，读懂 `predict` 的常用参数，以及阅读返回的 `Results` 结果对象。
>
> **前置 / 对应实现**：
> - [[YOLO第六课 YOLO训练实战|第六课]]——训练得到的 `best.pt` 从哪里来
> - [[YOLO第四课 检测输出、边界框解码与NMS|第四课]]——`conf` 与 NMS `iou` 在后处理中的分工
> - [[Pytorch学习|PyTorch 学习笔记]]——张量与模型前向传播

## 1. 加载模型

```python
from ultralytics import YOLO

model = YOLO(权重路径)      # 例如 best.pt / last.pt / yolo26n.pt
```

- 用自己训练得到的 `best.pt` 做针对性检测；
- 也可以直接用官方预训练权重 `yolo26n.pt` 做通用检测。

> [!tip] 推理优先用 `best.pt`
> `best.pt` 是验证指标最好的那一轮权重；`last.pt` 主要用于恢复训练。两者的区别见 [[YOLO第六课 YOLO训练实战|第六课]]。

## 2. 推理 predict

```python
results = model.predict(
    source=图片路径,      # 单张图片、图片列表、文件夹、视频,或 0 表示摄像头
    conf=置信度阈值,       # 只保留置信度高于该阈值的预测框
    iou=NMS使用的IoU阈值,  # 控制重叠框的抑制力度
    device=推理设备,       # 例如 0 / cpu
    save=是否保存检测结果,  # True 则把画好框的图片写到 runs/detect/predict*/
)
```

常用参数：

| 参数 | 作用 |
| --- | --- |
| `source` | 推理输入：图片 / 文件夹 / 视频 / `0`（摄像头） |
| `conf` | 置信度阈值，**过滤不可信的预测框** |
| `iou` | NMS 的 IoU 阈值，**去除重复框** |
| `device` | `0`（第一张 GPU）/ `cpu` / `-1`（自动选择） |
| `save` | 是否保存可视化结果 |
| `imgsz` | 推理输入尺寸 |

> [!warning] `conf` 与 `iou` 不是一回事
> `conf` 判断"这个框可不可信"，`iou` 判断"两个框是不是重复"。混淆二者是最常见的错误。详见 [[YOLO第四课 检测输出、边界框解码与NMS|第四课]]。

## 3. 预测结果对象 `Results`

推理返回的 `Results` 中最常用的字段：

| 字段 | 含义 |
| --- | --- |
| `boxes.xyxy` | 边界框的**像素**坐标 `[x1, y1, x2, y2]` |
| `boxes.xyxyn` | 边界框的**归一化**坐标（0~1） |
| `boxes.conf` | 每个框的置信度 |
| `boxes.cls` | 每个框的类别编号 |
| `names` | 类别编号到类别名称的映射 |
| `path` | 该结果对应的图片路径 |
| `speed` | 预处理 / 推理 / 后处理耗时 |

```python
r = results[0]
print(r.boxes.xyxy)    # 像素坐标
print(r.boxes.xyxyn)   # 归一化坐标
print(r.boxes.conf)    # 置信度
print(r.boxes.cls)     # 类别编号
```

> [!tip] 像素坐标 vs 归一化坐标
> `xyxy` 随图片尺寸变化，适合直接画框；`xyxyn` 与图片尺寸无关，适合跨尺寸比较与计算。两者的互转见 [[Pytorch学习#十六、YOLO 目标检测数据格式和边界框|YOLO 数据格式和边界框]]。

> [!note] 本课小结
> - 用 `YOLO(权重路径)` 加载模型，推理优先使用 `best.pt`
> - `model.predict(source=..., conf=..., iou=..., save=...)` 完成推理
> - `conf` 过滤不可信的框，`iou` 控制 NMS 的去重力度
> - `Results` 提供 `xyxy` / `xyxyn` / `conf` / `cls` / `names` / `path` / `speed`
> - 推理参数只影响"显示与筛选"，不会让模型本身变好——真正改进模型见 [[YOLO第八课 训练结果分析,错误诊断与优化|第八课]]

---

## 导航

- 上一课：[[YOLO第六课 YOLO训练实战]]
- 下一课：[[YOLO第八课 训练结果分析,错误诊断与优化]]
- 总览：[[视觉学习/YOLO学习/_MOC|YOLO 学习总览]]
- 进度与练习：[[YOLO学习进度]]
