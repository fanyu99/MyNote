---
title: 视觉学习知识库总览
type: MOC
created: 2026-09-16
updated: 2026-09-18
tags:
  - 计算机视觉
  - 深度学习
  - MOC
  - 索引
---

> 从图像处理基础 → 深度学习框架 → 目标检测实战的完整学习路径。

---

## 🗺️ 知识库结构

```
视觉学习/
├── _MOC.md                    ← 本页：总览 / 学习路径
├── Pytorch学习.md             ← 前置：PyTorch 基础（为 YOLO 打基础）
├── YOLO学习.md                ← 主要：目标检测与 YOLO 系列（第 1~3 课已完成）
├── YOLO学习进度.md            ← 进度记录：分阶段清单、每课练习与反馈
└── 相关笔记链接
    └── [[OpenCV学习笔记/_MOC|OpenCV 知识库]]（图像处理和几何变换）
```

---

## 📚 学习路径

### 第一阶段：图像处理基础（5~10 小时）
**目标**：理解图像是如何被计算机表示和处理的

- [[OpenCV第一课 基础入门|OpenCV 第一课]]：图像加载、显示、基本操作
- [[OpenCV第二课 cv_Mat模型|cv_Mat 核心模型]]：像素、通道、数据类型
- [[OpenCV第三课 图像几何操作|几何变换]]：缩放、旋转、仿射变换（YOLO 数据增强会用）
- [[OpenCV第四课 阈值处理和二值化|阈值和二值化]]：图像预处理基础

**实践**：用 OpenCV 打开图片、调整大小、旋转、灰度转换

---

### 第二阶段：深度学习框架基础（10~15 小时）
**目标**：掌握 PyTorch 中神经网络的基本操作，为深度学习做准备

#### [[Pytorch学习|PyTorch 学习笔记]]（推荐按顺序学习）

| 章 | 章节 | 核心概念 | 对标 YOLO 的用途 |
|---|---|---|---|
| 1 | [[Pytorch学习#一、张量 Tensor\|张量 Tensor]] | n 维数组、shape、reshape | 图片、预测输出的数据格式 |
| 2 | [[Pytorch学习#二、自动求导 Autograd\|自动求导]] | requires_grad、backward() | 理解训练如何调整参数 |
| 3 | [[Pytorch学习#三、第一个神经网络与 nn.Module\|nn.Module]] | 定义模型、forward 方法 | YOLO 模型就是 nn.Module 子类 |
| 4 | [[Pytorch学习#四、损失函数与优化器\|损失函数和优化器]] | MSELoss、SGD、Adam | 选择合适的优化策略 |
| 5 | [[Pytorch学习#五、Epoch / Batch / DataLoader\|Epoch / Batch / DataLoader]] | 数据加载、batch 概念 | 组织训练数据的标准方式 |
| 6 | [[Pytorch学习#六、训练模式 / 预测模式 / no_grad\|训练 vs 推理模式]] | model.train/eval、no_grad | 训练时开梯度，推理时关梯度 |
| 7 | [[Pytorch学习#七、激活函数\|激活函数]] | ReLU、SiLU、非线性 | YOLO 用 SiLU（Swish）|
| 8 | [[Pytorch学习#八、卷积层 Conv2d\|卷积层 Conv2d]] | 卷积核、padding、stride | YOLO backbone 的基本单元 |
| 9 | [[Pytorch学习#九、批归一化 BatchNorm2d\|批归一化 BatchNorm2d]] | 归一化层、train/eval | YOLO 网络标准 |
| 10 | [[Pytorch学习#十、完整 CNN：卷积 → 池化 → 展平 → 全连接\|完整 CNN]] | Conv+ReLU+Pool+Flatten | 图片 → 特征 → 分类的流程 |
| 11 | [[Pytorch学习#十一、分类模型完整训练流程\|分类训练流程]] | CrossEntropyLoss、argmax | 训练循环的标准模板 |
| 12 | [[Pytorch学习#十二、自定义 Dataset 与图片加载\|自定义 Dataset]] | 从文件夹读取数据 | 数据管线核心 |
| 13 | [[Pytorch学习#十三、图片归一化与训练集/验证集\|归一化与训练/验证集]] | Normalize、random_split | 数据集划分、防过拟合 |
| 14 | [[Pytorch学习#十四、GPU 支持与设备管理\|GPU 支持]] | 显卡加速、device 管理 | YOLO 训练必需 |
| 15 | [[Pytorch学习#十五、模型保存、加载与 State Dict\|模型保存/加载]] | 权重管理、Checkpoint | YOLO 权重文件 |
| 16 | [[Pytorch学习#十六、YOLO 目标检测数据格式和边界框\|YOLO 数据格式]] | `class x y w h`、归一化 | 检测标签 |
| 17 | [[Pytorch学习#十七、在图片上绘制和检查边界框\|绘制边界框]] | `cv2.rectangle`、坐标转换 | 可视化检查标签 |
| 18 | [[Pytorch学习#十八、IoU、NMS 与检测指标\|IoU / NMS / 检测指标]] | IoU、P/R、AP、mAP、NMS | 评估检测结果 |

**重点**：第 1~11 章是 PyTorch 主干；**第 16~18 章（检测基础）是转向 YOLO 前的必读**，它们的 YOLO 视角版本见 [[YOLO学习]]。

**实践**：跑完每章的代码示例，尝试修改参数看效果

---

### 第三阶段：目标检测与 YOLO（15~20 小时）
**目标**：理解 YOLO 的工作原理，能读懂源码，能训练/推理模型

详见 [[YOLO学习|YOLO 目标检测学习笔记]]（进度与练习见 [[YOLO学习进度]]）：

| 课次 | 主题 | 状态 |
|---|---|---|
| 第一课 | [[YOLO学习#第一课：从图像分类进入目标检测\|图像分类 vs 目标检测、边界框格式、核心思想]] | ✅ 已完成 |
| 第二课 | [[YOLO学习#第二课：IoU、Precision、Recall、AP、mAP\|IoU、Precision、Recall、AP、mAP]] | ✅ 已完成 |
| 第三课 | [[YOLO学习#第三课：YOLO 整体网络结构\|Backbone / Neck / Head]] | ✅ 已完成 |
| 第四课 | 检测输出解码、边界框预测参数、NMS 实现原理 | ⏳ 待开始 |
| 第五课 | 数据格式与标注转换 | ⏳ 待开始 |
| 第六课 | YOLOv8 模型结构解读 | ⏳ 待开始 |
| 第七课 | 从 Ultralytics 源码读 YOLO | ⏳ 待开始 |
| 第八课 | 在自己的数据集上训练 YOLO | ⏳ 待开始 |
| 第九课 | YOLO 推理与后处理 | ⏳ 待开始 |
| 第十课 | 性能优化与模型部署 | ⏳ 待开始 |

---

## 🔗 关键概念速查

| 问题 | 所在章节 | 核心代码 |
|---|---|---|
| 如何创建张量？ | [[Pytorch学习#一、张量 Tensor\|张量]] | `torch.tensor([...])` |
| 如何自动求导？ | [[Pytorch学习#二、自动求导 Autograd\|自动求导]] | `y.backward(); print(x.grad)` |
| 如何定义神经网络模型？ | [[Pytorch学习#三、第一个神经网络与 nn.Module\|nn.Module]] | `class Model(nn.Module): def forward(self, x):` |
| 如何加载数据并分 batch？ | [[Pytorch学习#五、Epoch / Batch / DataLoader\|DataLoader]] | `DataLoader(dataset, batch_size=32, shuffle=True)` |
| 如何训练和推理？ | [[Pytorch学习#六、训练模式 / 预测模式 / no_grad\|训练 vs 推理]] | `model.train(); ... model.eval(); with torch.no_grad():` |
| 如何构建 CNN？ | [[Pytorch学习#八、卷积层 Conv2d\|Conv2d]] / [[Pytorch学习#十、完整 CNN：卷积 → 池化 → 展平 → 全连接\|完整 CNN]] | `nn.Conv2d(3, 16, kernel_size=3, padding=1)` |
| 如何分类？ | [[Pytorch学习#十一、分类模型完整训练流程\|分类流程]] | `CrossEntropyLoss()` + `output.argmax(dim=1)` |
| 检测标签长什么样？ | [[Pytorch学习#十六、YOLO 目标检测数据格式和边界框\|YOLO 数据格式]] | `class x_center y_center w h`（归一化） |
| 如何算 IoU？ | [[Pytorch学习#十八、IoU、NMS 与检测指标\|IoU]] | `交集面积 / 并集面积` |
| 如何评价检测结果？ | [[Pytorch学习#十八、IoU、NMS 与检测指标\|检测指标]] | Precision / Recall / AP / mAP |
| 如何去掉重复检测框？ | [[Pytorch学习#十八、IoU、NMS 与检测指标\|NMS]] | 置信度排序 + IoU 去重 |
| 为什么要 Backbone/Neck/Head？ | [[YOLO学习#第三课：YOLO 整体网络结构\|YOLO 网络结构]] | 提特征 → 融合多尺度 → 预测 |

---

## ⏱️ 建议学习进度

### Week 1：图像处理基础
- 完成 OpenCV 第 1-4 课
- 做一个小项目：用 OpenCV 加载、显示、变换图片

### Week 2-3：PyTorch 基础
- 完成 PyTorch 笔记全部 11 章
- 关键是第 3、4、5、8、9、10 章，每章都跑一遍代码

### Week 4+：YOLO 实战
- 阅读 [[YOLO学习|YOLO 笔记]]，从已完成的第一~三课开始
- 在 COCO / 自己的数据集上做实验

---

## 📝 笔记更新日志

- **2026-09-16**：创建 MOC，完善 PyTorch 笔记第 1-11 章，计划建立学习路径
- **2026-09-16**：补充代码输出、陷阱、练习建议、速查表
- **2026-09-18**：PyTorch 笔记扩充至 18 章（补 BatchNorm、归一化/数据集划分、检测指标）；[[YOLO学习]] 建立并整理第 1~3 课内容；修正本页章节编号与锚点，与 [[Pytorch学习]]、[[YOLO学习]] 建立双向链接
- *待续*：YOLO 第四课起（NMS 实现、数据格式、YOLOv8 结构与训练实战）

---

## 💡 学习建议

> [!tip] 学习方式
> 1. **不要跳过代码例子**：每个代码块都跑一遍，修改参数观察输出变化
> 2. **遇到公式不懂没关系**：先记住"这个函数是干什么的"，推导留到深入学习时
> 3. **做笔记时**：标记"这个在 YOLO 里怎么用"，后面看源码时速度会快很多
> 4. **卡住时**：回到对应章节查"常见陷阱"，80% 的问题都在那里

> [!warning] 常见误区
> - 觉得"把代码抄一遍就懂了" —— 不懂。一定要改参数、跑输出、对比结果
> - PyTorch 没学透就想学 YOLO —— 会很吃力。第二阶段至少要花 10-15 小时
> - 只看笔记不写代码 —— 深度学习必须手敲代码、调 bug
> - 跳过"还差什么才能上手 YOLO"那章 —— 这章列的内容是必读，不是可选

---

## 🎯 学完这套笔记，你能做什么

- ✅ 理解图像在计算机中如何表示（OpenCV）
- ✅ 从零手写一个简单的神经网络（PyTorch）
- ✅ 理解卷积、池化、全连接的作用（CNN）
- ✅ 组织数据集并进行模型训练（训练循环）
- ✅ 读懂 YOLO 的模型结构和代码（框架理解）
- ✅ 在自己的数据集上训练 / 推理 YOLO（实战能力）
- ❌ 不能（还不够）：从零改进 YOLO 算法、设计新的检测架构

后续可以查阅 YOLO 官方文档、论文、参与开源项目来深化。
