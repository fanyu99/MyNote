---
title: YOLO 第六课：训练实战——预训练模型、训练命令与冒烟测试
type: 课程笔记
课程: YOLO 目标检测
课次: 6
created: 2026-09-20
updated: 2026-09-20
tags:
  - yolo
  - 目标检测
  - 深度学习
  - 课程笔记
  - 训练
status: 已完成
---

# 第六课：训练实战——预训练模型、训练命令与冒烟测试

> [!abstract] 本课目标
> 分清"从头训练"与"预训练模型 + 迁移学习"的区别，掌握 `yolo TASK MODE ARGS` 的命令结构与常用训练参数，理解**冒烟测试**的作用，并跑通第一次训练。
>
> **前置 / 对应实现**：
> - [[YOLO第五课 数据集目录,标签格式与标注转换|第五课]]——数据集目录、标签格式与 `data.yaml`
> - [[Pytorch学习|PyTorch 学习笔记]]——`Dataset` / `DataLoader`、损失函数、优化器与反向传播

## 1. 从头训练与预训练模型

### 从头训练（from scratch）

加载模型结构配置：

```python
model = YOLO("yolo26n.yaml")
```

`.yaml` 主要加载的是**网络结构**，权重通常从**随机初始化**开始学习。可以理解为：

```
请一个从来没有见过图片的人,从零开始学习什么是边缘、纹理、形状、猫和狗
```

因此从头训练通常更难，需要更多数据和更长的训练时间。

### 预训练模型（pretrained）

```python
model = YOLO("yolo26n.pt")
```

`.pt` 文件一般已经包含**预训练权重**。预训练模型通常已经学习过：

- 边缘；
- 颜色；
- 纹理；
- 轮廓；
- 常见物体结构；
- 不同尺度的视觉特征。

在此基础上再用自己的数据集继续训练，这就叫 **迁移学习（Transfer Learning）**。

> [!tip] `.yaml` 与 `.pt` 的区别
> `.yaml` 是**结构说明书**（有哪些层、每层多少通道）；`.pt` 是**结构 + 权重**。只给 `.yaml` 就是从零学起，给 `.pt` 则是站在预训练模型的肩膀上微调。

## 2. 训练命令的基本结构

Ultralytics 命令行指令的基础结构为：

```
yolo TASK MODE ARGS
```

例如：

```powershell
yolo detect train model=yolo26n.pt data=D:/datasets/cat_dog/data.yaml epochs=50 imgsz=640 batch=8 device=0
```

> [!warning] 三个最容易写错的地方
> 1. 轮数参数是 **`epochs`**，不是 `epoch`；
> 2. 参数必须写成 **`key=value`**，等号两边**不能加空格**——`data = "..."` 会被拆成多个参数而报错；
> 3. `TASK` 与 `MODE` 是不带 `=` 的位置参数，顺序为 `yolo <task> <mode> key=value ...`。

### 常见参数

#### 1. `TASK` 任务类型

`detect` 表示***目标检测***。其他任务类型：

```
classify    图像分类
segment     实例分割
pose        姿态估计
obb         旋转框检测
```

#### 2. `MODE` 模式

`train` 表示***训练模式***。其他模式：

```
val         验证
predict     推理
export      导出模型
track       目标跟踪
benchmark   基准测试
```

#### 3. `model`

***模型配置***，可以是 `yolo26n.pt`（预训练权重）或 `yolo26n.yaml`（仅结构）。

#### 4. `data`

***数据集配置文件路径***，推荐用引号包起来（路径含空格时必需）。

#### 5. `epochs` / `imgsz` / `batch` / `device`

| 参数 | 含义 |
| --- | --- |
| `epochs` | 训练轮数 |
| `imgsz` | 输入图片尺寸，常用 `640` |
| `batch` | 一批送入多少张图片 |
| `device` | 训练设备 |

`device` 的常用取值：

```
device=0       选择第一张 NVIDIA GPU
device=0,1     使用第 0、1 张 GPU
device=-1      自动选择 GPU
device=cpu     使用 CPU 训练
```

#### 6. `workers`

控制数据加载使用的工作进程数量。Windows 下若出现数据加载相关的报错，可先设为 `workers=0` 排查。

## 3. 冒烟测试 Smoke Test

即**用最小的训练成本检查整个流程能不能跑通**。

典型做法是选一个很小的数据集（例如 Ultralytics 自带的 `coco8.yaml`，只有 8 张图片），把 `epochs` 设为 `2~3`，先确认：

- 数据能正常读取、标签能正常解析；
- 模型能加载，能前向传播、反向传播；
- 显存装得下；
- 能正常保存权重与结果文件。

> [!warning] 冒烟测试的指标不能当结论
> `coco8` 这类小数据集上跑出的指标很高，主要来自**预训练权重**加上**极小的验证集**，只能说明"流程通了"，**不能说明模型的泛化能力好**。正式评估必须用独立、有代表性的验证集或测试集。

## 4. 第一次训练：环境与结果

> [!note] 本次实验记录（详见 [[YOLO学习进度]]）
> - 环境：Python `3.13.14` / PyTorch `2.14.0+cu132` / Ultralytics `8.4.155` / CUDA `13.2`
> - GPU：NVIDIA GeForce RTX 5070 Laptop，约 8 GB 显存
> - 冒烟训练配置：`model=yolo26n.pt`、`data=coco8.yaml`、`epochs=3`、`imgsz=640`、`batch=4`、`device=0`、`workers=0`
> - 结果：3 个 epoch 完整跑通，显存占用约 `0.8 GB`，并生成 `best.pt`、`last.pt`、`results.csv`、`results.png`

> [!tip] `best.pt` 与 `last.pt`
> - `best.pt`：验证指标最好的那一轮权重，**推理优先用它**；
> - `last.pt`：最后一轮的权重，**恢复/继续训练时用它**。

如何解读训练结果中的指标、混淆矩阵与曲线，见 [[YOLO第八课 训练结果分析,错误诊断与优化|第八课]]。

> [!note] 本课小结
> - `.yaml` 只给结构（从头训练），`.pt` 带预训练权重（迁移学习）
> - 命令结构为 `yolo TASK MODE ARGS`，参数写成 `key=value`，注意是 `epochs` 而不是 `epoch`
> - `device=-1` 表示自动选择 GPU，`device=cpu` 使用 CPU 训练
> - 正式训练前先用小数据集 + 少量 `epochs` 做**冒烟测试**，确认流程能跑通
> - 小数据集上的冒烟指标不能代表真实泛化能力
> - 推理用 `best.pt`，继续训练用 `last.pt`

---

## 导航

- 上一课：[[YOLO第五课 数据集目录,标签格式与标注转换]]
- 下一课：[[YOLO第七课 模型加载与图片推理]]
- 总览：[[视觉学习/YOLO学习/_MOC|YOLO 学习总览]]
- 进度与练习：[[YOLO学习进度]]
