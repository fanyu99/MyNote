---
title: PyTorch 学习笔记（YOLO 前置知识）
type: 课程笔记
created: 2026-09-16
tags:
  - pytorch
  - 深度学习
  - YOLO前置
---

> [!note] 定位
> 本笔记目标是**为学习 YOLO 打基础**，只覆盖 PyTorch 中会被 YOLO 训练/推理直接用到的部分（张量、自动求导、`nn.Module`、CNN 三大组件、分类训练循环），不追求覆盖 PyTorch 全部特性。
> 
> **相关笔记**：
> - [[YOLO学习|YOLO 目标检测学习笔记]]（本笔记第 16~18 章是它的前置；YOLO 侧的概念会在那里继续展开）
> - [[视觉学习/_MOC|视觉学习知识库总览]]（学习路径、全景图）
> - [[视觉学习/OpenCV学习笔记/_MOC|OpenCV 知识库总览]]（图像预处理/几何变换，与 YOLO 数据管线直接相关）

**预计学习时间**：10~15 小时 | **难度**：⭐⭐⭐ | **必读程度**：必读 ✅

## 目录

**基础模块 (第1-7章) —— PyTorch 基础**
1. [张量 Tensor](#一张量-tensor)
2. [自动求导 Autograd](#二自动求导-autograd)
3. [第一个神经网络与 nn.Module](#三第一个神经网络与-nnmodule)
4. [损失函数与优化器](#四损失函数与优化器)
5. [Epoch / Batch / DataLoader](#五epoch--batch--dataloader)
6. [训练模式 / 预测模式 / no_grad](#六训练模式--预测模式--no_grad)
7. [激活函数](#七激活函数)

**卷积神经网络 (第8-11章) —— CNN 核心**
8. [卷积层 Conv2d](#八卷积层-conv2d)
9. [批归一化 BatchNorm2d](#九批归一化-batchnorm2d)
10. [完整 CNN：卷积 → 池化 → 展平 → 全连接](#十完整-cnn卷积--池化--展平--全连接)
11. [分类模型完整训练流程](#十一分类模型完整训练流程)

**数据处理与模型管理 (第12-15章) —— 工程实践**
12. [自定义 Dataset 与图片加载](#十二自定义-dataset-与图片加载)
13. [图片归一化与训练集/验证集](#十三图片归一化与训练集验证集)
14. [GPU 支持与设备管理 (.to(device))](#十四gpu-支持与设备管理)
15. [模型保存、加载与 State Dict](#十五模型保存加载与-state-dict)

**YOLO 检测基础 (第16-18章) —— 检测知识**
16. [YOLO 目标检测数据格式和边界框](#十六yolo-目标检测数据格式和边界框)
17. [在图片上绘制和检查边界框](#十七在图片上绘制和检查边界框)
18. [IoU、NMS 与检测指标](#十八iounms-与检测指标)

> 第 16~18 章的知识点在 [[YOLO学习|YOLO 学习笔记]] 中会以目标检测的视角重新组织一遍，两篇笔记互为正反面。

---

## 一、张量 Tensor

简单来说张量就是 n 维数组。

```python
import torch

a = torch.tensor([2, 3])
print(a.shape)
# 输出：torch.Size([2])，表示长度为2的向量

b = torch.tensor([[2, 3], [4, 5]])
print(b.shape)
# 输出：torch.Size([2, 2])，表示2*2的矩阵
```

### 常用方法

| 方法        | 用途        |
| --------- | --------- |
| `shape`   | 查看形状      |
| `unsqueeze` | 增加维度      |
| `squeeze` | 删除大小为1的维度 |
| `reshape` | 改变形状      |
| `permute` | 调整维度的顺序   |

> [!example] 维度操作示例
> ```python
> x = torch.tensor([[1, 2, 3]])  # shape: [1, 3]
> x_unsqueeze = x.unsqueeze(0)   # [1, 1, 3]
> x_squeeze = x.squeeze()         # [3]
> x_reshape = x.reshape(3, 1)     # [3, 1]
> ```

> [!danger] 常见陷阱
> - `shape` 返回的是 `torch.Size` 对象，不是元组，但可以像元组一样索引：`x.shape[0]` 能用
> - `reshape` 会改变张量形状但通常不改变内存顺序，而 `view` 要求张量连续存储，两者不能混用
> - 张量默认数据类型通常是 `float32`（浮点）和 `int64`（整数），在计算中混用可能报错

> [!tip] 与 OpenCV 的关联
> OpenCV 中的 `cv.Mat` 是 C++ 的 2D/3D 数组（像素矩阵），而 PyTorch 的张量是 n 维的。两者转换时注意通道顺序：
> - OpenCV：`[H, W, C]`（如 BGR 图片）
> - PyTorch（图片标准）：`[B, C, H, W]`（如 RGB 图片）
> - 转换函数：`torch.from_numpy(cv_image)` / `tensor.numpy()` 后需要 `permute` 调整维度

### 动手试试

```python
# 创建一张 480×640 的 RGB 图片张量（4 张）
images = torch.rand(4, 3, 480, 640)  # [批次, 通道, 高, 宽]
print("图片张量形状：", images.shape)
print("第一张图片的 R 通道形状：", images[0, 0].shape)

# 改变图片尺寸为 256×256
images_resized = images.reshape(-1, 3, 256, 256)  # -1 自动推导
print("改变后形状：", images_resized.shape)
```

---

## 二、自动求导 Autograd

神经网络训练时会根据误差自动计算参数应该如何调整，这就是自动求导。

```python
import torch

x = torch.tensor(2.0, requires_grad=True)  # requires_grad=True 表示记录这个变量的计算过程

y = x ** 2 + 3 * x + 1

y.backward()  # 根据 y 反向计算梯度

print("y =", y)
print("x的梯度 =", x.grad)
# 输出：
# y = tensor(11., grad_fn=<AddBackward0>)
# x的梯度 = tensor(7.)
```

$$
y = x^2 + 3x + 1,\quad \frac{dy}{dx} = 2x + 3
$$

代入 $x=2$ 得 $\frac{dy}{dx}=2 \times 2 + 3 = 7$ ✓

> [!danger] 常见陷阱
> 1. **创建张量时忘记 `requires_grad=True`** —— 即使后续操作中包含参数，如果输入张量没有 `requires_grad=True`，梯度也无法回传
> 2. **对同一张量多次调用 `backward()`** —— 梯度会累加。需要先 `x.grad.zero_()` 才能重新计算
> 3. **`backward()` 只能在标量上调用** —— 如果 `y` 是向量，需要先 `loss = y.sum()`，然后 `loss.backward()`

> [!example] 完整示例（包含常见错误）
> ```python
> # ✅ 正确：向量上调用 backward() 前要求和
> x = torch.tensor([[1.0, 2.0]], requires_grad=True)
> y = x ** 2
> loss = y.sum()  # 重要：向量必须先求和得到标量
> loss.backward()
> print(x.grad)
> 
> # ❌ 错误：直接在向量上 backward()
> y.backward()  # RuntimeError: grad can be implicitly created only for scalar outputs
> ```

### 前向传播与反向传播

| 步骤        | 目的                     | 类比            |
| --------- | ---------------------- | ------------- |
| `forward` | 根据当前参数计算预测结果           | 学生做题，写出答案     |
| `backward`| 根据预测与标准答案的差距，算出每个参数应调整的方向和幅度 | 老师批改，指出错误程度 |

### 动手试试

```python
import torch

# 创建模型参数（YOLO 中卷积层的权重就是这样的张量）
w = torch.tensor([1.5], requires_grad=True)
b = torch.tensor([0.5], requires_grad=True)

# 模拟一次预测和反向传播（YOLO 训练循环中的一步）
x = torch.tensor([2.0])
y_true = torch.tensor([5.0])

# 前向传播
y_pred = w * x + b  # 线性模型预测

# 计算损失
loss = (y_pred - y_true) ** 2

# 反向传播
loss.backward()

print(f"预测值: {y_pred.item():.2f}, 真实值: {y_true.item():.2f}")
print(f"权重梯度: {w.grad.item():.2f}, 偏置梯度: {b.grad.item():.2f}")

# 模拟参数更新
learning_rate = 0.01
w.data -= learning_rate * w.grad
b.data -= learning_rate * b.grad
print(f"更新后的权重: {w.item():.2f}, 偏置: {b.item():.2f}")
```

---

## 三、第一个神经网络与 nn.Module

```python
import torch
from torch import nn

class SimpleModel(nn.Module):
    def __init__(self):
        super().__init__()
        self.linear = nn.Linear(2, 1)  # 全连接层：输入特征数2，输出特征数1
        # 输入形如 [[1.0,2.0],[2.0,3.0],...]，每条数据2个特征

    def forward(self, x):
        return self.linear(x)

model = SimpleModel()

x = torch.tensor([[1.0, 2.0]])
output = model(x)

print("输出:", output)
# 输出: tensor([[...]], grad_fn=<AddmmBackward0>)  <- 具体数值取决于初始化权重
```

### 全连接层

`nn.Linear(输入特征数, 输出特征数)`，`(2,1)` 内部计算大致为：

$$
y = x_1 w_1 + x_2 w_2 + b
$$

其中 $x_1,x_2$ 是输入，$w_1,w_2$ 是权重，$b$ 是偏置，$y$ 是输出。

### 查看模型参数

```python
for name, para in model.named_parameters():
    print(name)
    print(para)

# 输出示例：
# linear.weight
# Parameter containing:
# tensor([[-0.1234,  0.5678]], requires_grad=True)
# linear.bias
# Parameter containing:
# tensor([0.0234], requires_grad=True)
```

模型参数包括权重 `weight`、偏置 `bias`。训练的本质就是不断调整这些参数，使预测结果更准确。

```python
import torch
from torch import nn

model = nn.Linear(2, 1)

x = torch.tensor([
    [1.0, 2.0],
    [2.0, 3.0],
    [3.0, 4.0]
])

output = model(x)

print("输入形状：", x.shape)  # torch.Size([3, 2])
print("输出形状：", output.shape)  # torch.Size([3, 1])
print("预测结果：", output)
```

> [!danger] 常见陷阱
> 1. **忘记继承 `nn.Module`** —— 自定义模型必须继承 `nn.Module`，否则无法使用 `parameters()`、`to(device)` 等方法
> 2. **忘记调用 `super().__init__()`** —— 必须在 `__init__` 第一行调用，否则模型无法正常初始化
> 3. **在 `forward()` 之外定义层** —— 所有子层都必须在 `__init__` 里定义并赋给 `self`，否则参数不会被识别
> 4. **模型定义后忘记 `model = Model()`** —— 必须创建实例才能使用

### YOLO 视角

YOLO 的整个网络（backbone + head）都是继承 `nn.Module` 的一个大模型，内部包含数十个卷积层、归一化层、激活函数，通过 `forward()` 一次前向传播得到目标检测结果。

### 动手试试

```python
import torch
from torch import nn

class SimpleNN(nn.Module):
    def __init__(self, input_size, hidden_size, output_size):
        super().__init__()
        self.fc1 = nn.Linear(input_size, hidden_size)
        self.fc2 = nn.Linear(hidden_size, output_size)
    
    def forward(self, x):
        x = self.fc1(x)
        x = torch.relu(x)  # 激活函数
        x = self.fc2(x)
        return x

model = SimpleNN(input_size=10, hidden_size=5, output_size=3)

# 创建随机输入
x = torch.randn(4, 10)  # 4 条数据，每条 10 维特征

# 前向传播
output = model(x)
print("输入形状:", x.shape)
print("输出形状:", output.shape)  # 应该是 [4, 3]

# 查看模型结构
print("\n模型参数数量:")
for name, param in model.named_parameters():
    print(f"{name}: {param.shape} -> {param.numel()} 个参数")
```

---

## 四、损失函数与优化器

### 损失函数

损失函数是打分器，用于评判预测结果的准确性。常见的回归损失是均方误差：

```python
loss_function = nn.MSELoss()
```

$$
MSE = \frac{1}{n}\sum_{i=1}^{n}(预测值_i - 真实值_i)^2
$$

### 优化器

优化器负责根据梯度修改 `model.weight`、`model.bias`：

```python
optimizer = torch.optim.SGD(
    model.parameters(),
    lr=0.01
)
```

- `SGD`：随机梯度下降
- `model.parameters()`：把模型参数交给优化器
- `lr`：学习率，决定每次修改参数的幅度，常见值 `0.01`、`0.001`

> [!tip] 补充：YOLO 实践中更常用 Adam / AdamW
> 本笔记后面分类示例已经改用了 `torch.optim.Adam`。YOLO 官方实现（如 Ultralytics）默认用 **SGD 或 AdamW**，并搭配学习率调度器（如余弦退火、warmup）。目前阶段先掌握 SGD/Adam 的基本用法即可，调度器留到看 YOLO 源码时再学。

### 一次完整的训练过程

```python
import torch
from torch import nn

model = nn.Linear(2, 1)

x = torch.tensor([
    [1.0, 2.0],
    [2.0, 3.0],
    [3.0, 4.0]
])

y_true = torch.tensor([
    [3.0],
    [5.0],
    [7.0]
])

loss_function = nn.MSELoss()

optimizer = torch.optim.SGD(
    model.parameters(),
    lr=0.01
)

# 1. 前向传播：根据输入进行预测
y_pred = model(x)

# 2. 计算损失
loss = loss_function(y_pred, y_true)

# 3. 清空旧梯度
optimizer.zero_grad()

# 4. 反向传播：根据损失计算梯度
loss.backward()

# 5. 根据梯度更新参数
optimizer.step()

print("损失：", loss.item())
print("更新后的权重：", model.weight)
print("更新后的偏置：", model.bias)
```

训练流程：

```
预测 → 计算损失 → 计算梯度 → 更新参数
```

| 步骤            | 代码实现                              | 作用                                    |
| ------------- | --------------------------------- | --------------------------------------- |
| 预测（前向传播）      | `output = model(input)`           | 根据当前参数计算输出                            |
| 计算损失          | `loss = loss_fn(output, target)`  | 衡量预测值与真实值的差距（得到一个标量）                  |
| 计算梯度（反向传播）    | `loss.backward()`                 | 自动求导，计算每个参数相对于损失的梯度，存入 `.grad`        |
| 更新参数          | `optimizer.step()`                | 利用梯度（如 `param -= lr * param.grad`）更新参数 |

### 为什么要 `zero_grad()`

PyTorch 默认**累加**梯度，所以每次训练前通常都要写：

```python
optimizer.zero_grad()
```

> [!danger] 常见陷阱
> - **忘记 `zero_grad()`** —— 梯度会累加，导致参数更新异常
> - **多个优化器或同一优化器用了多次** —— 记得在每个 `backward()` 前都调用 `zero_grad()`
> - 示例：
> ```python
> # ❌ 错误：第一轮梯度会保留到第二轮
> for epoch in range(10):
>     output = model(x)
>     loss = loss_fn(output, y)
>     loss.backward()  # 梯度累加！
>     optimizer.step()
> 
> # ✅ 正确
> for epoch in range(10):
>     output = model(x)
>     loss = loss_fn(output, y)
>     optimizer.zero_grad()  # 清空旧梯度
>     loss.backward()
>     optimizer.step()
> ```

### 动手试试

```python
import torch
from torch import nn

# 创建一个简单的线性模型
model = nn.Linear(1, 1)

# 创建数据：y = 2*x + 1
x = torch.tensor([[1.0], [2.0], [3.0], [4.0]])
y_true = 2 * x + 1

loss_fn = nn.MSELoss()
optimizer = torch.optim.SGD(model.parameters(), lr=0.01)

# 训练 5 个 epoch
for epoch in range(5):
    # 前向传播
    y_pred = model(x)
    loss = loss_fn(y_pred, y_true)
    
    # 反向传播
    optimizer.zero_grad()
    loss.backward()
    optimizer.step()
    
    print(f"Epoch {epoch+1}, Loss: {loss.item():.4f}")

# 最终预测
print("\n最终预测:")
with torch.no_grad():  # 推理时不需要梯度
    print("输入:", x.squeeze())
    print("预测:", model(x).squeeze())
    print("真实:", y_true.squeeze())
```

---

## 五、Epoch / Batch / DataLoader

- **Epoch**：模型完整看完一次全部训练数据
- **Batch**：一次处理多少条数据
- **Dataset**：保存和读取数据，把数据与标签对应起来
- **DataLoader**：按批次读取数据，按一定方式（如 `shuffle`）加载 `Dataset`

```python
import torch
from torch import nn
from torch.utils.data import TensorDataset, DataLoader

model = nn.Linear(2, 1)

x = torch.tensor([
    [1.0, 2.0],
    [2.0, 3.0],
    [3.0, 4.0],
    [4.0, 5.0],
    [5.0, 6.0],
    [6.0, 7.0]
])

y_true = torch.tensor([
    [3.0],
    [5.0],
    [7.0],
    [9.0],
    [11.0],
    [13.0]
])

dataset = TensorDataset(x, y_true)

data_loader = DataLoader(
    dataset,
    batch_size=2,
    shuffle=True
)

loss_function = nn.MSELoss()
optimizer = torch.optim.SGD(
    model.parameters(),
    lr=0.01
)

for epoch in range(1000):
    for batch_x, batch_y in data_loader:
        y_pred = model(batch_x)
        loss = loss_function(y_pred, batch_y)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

    if epoch % 100 == 0:
        print("第", epoch, "轮，损失：", loss.item())

print("最终预测：")
print(model(x))
```

### 重点小结

| 概念 | 含义 |
|---|---|
| Dataset | 数据集 |
| DataLoader | 按批次提供数据 |
| batch | 一次处理的一小批数据 |
| batch_size | 每个 batch 的数据数量 |
| epoch | 完整学习一次全部数据 |
| shuffle | 是否打乱数据顺序 |

> [!tip] YOLO 视角
> YOLO 训练同样用 `Dataset` + `DataLoader` 组织图片和标注（边界框），只是标签从一个数字变成"每张图若干个 `[类别, x, y, w, h]`"，`collate_fn` 需要自定义来处理每张图目标数量不一致的问题。这是后面自定义 Dataset 课的重点。

> [!danger] 常见陷阱
> 1. **batch_size 太大** —— 显存爆炸。YOLO 训练中如果显存不足，第一时间减小 `batch_size`
> 2. **忘记 `shuffle=True`** —— 数据顺序固定会导致模型泛化性差，训练时务必打乱
> 3. **Dataset 中的数据没有转 tensor** —— `DataLoader` 加载时会报错，记得在 `__getitem__` 里返回 tensor

### 动手试试

```python
import torch
from torch.utils.data import Dataset, DataLoader

# 自定义一个简单的数据集
class SimpleDataset(Dataset):
    def __init__(self, x, y):
        self.x = x
        self.y = y
    
    def __len__(self):
        return len(self.x)
    
    def __getitem__(self, idx):
        return self.x[idx], self.y[idx]

# 创建数据
x = torch.randn(100, 10)  # 100 条数据，每条 10 维特征
y = torch.randint(0, 3, (100,))  # 100 个标签，3 类

# 创建 Dataset 和 DataLoader
dataset = SimpleDataset(x, y)
loader = DataLoader(dataset, batch_size=16, shuffle=True)

# 遍历数据
for batch_idx, (batch_x, batch_y) in enumerate(loader):
    if batch_idx == 0:
        print(f"Batch 0 - 输入形状: {batch_x.shape}, 标签形状: {batch_y.shape}")
    if batch_idx < 3:
        print(f"Batch {batch_idx}: {batch_x.shape}")
```

---

## 六、训练模式 / 预测模式 / no_grad

训练模式用于训练（需要计算梯度），预测模式用于推理（不需要计算梯度）。`no_grad` 用来关闭梯度计算，节省显存/内存并加速推理。

```python
import torch
from torch import nn
from torch.utils.data import TensorDataset, DataLoader

model = nn.Linear(2, 1)

x = torch.tensor([
    [1.0, 2.0],
    [2.0, 3.0],
    [3.0, 4.0],
    [4.0, 5.0],
    [5.0, 6.0],
    [6.0, 7.0]
])

y_true = torch.tensor([
    [3.0],
    [5.0],
    [7.0],
    [9.0],
    [11.0],
    [13.0]
])

dataset = TensorDataset(x, y_true)

data_loader = DataLoader(
    dataset,
    batch_size=2,
    shuffle=True
)

loss_function = nn.MSELoss()
optimizer = torch.optim.SGD(
    model.parameters(),
    lr=0.01
)

for epoch in range(1000):
    model.train()  # 训练模式
    total_loss = 0.0
    for batch_x, batch_y in data_loader:
        y_pred = model(batch_x)
        loss = loss_function(y_pred, batch_y)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

        total_loss += loss.item()

    if epoch % 100 == 0:
        print("第", epoch, "轮，平均损失：", total_loss / len(data_loader))

# 预测模式
model.eval()
with torch.no_grad():
    prediction = model(x)

print("最终预测：")
print(prediction)
```

> [!warning] 重要补充：`model.eval()` 不等于 `torch.no_grad()`
> 二者作用不同，YOLO 推理时通常两个都要用：
> - `model.eval()`：切换 `BatchNorm`/`Dropout` 等层的行为（用训练时累积的统计量，而不是当前 batch 的统计量），**不会**关闭梯度计算。
> - `torch.no_grad()`：关闭梯度记录，省显存、加速，但不改变层的行为。
> 只调用其中一个在简单线性模型里看不出差别（因为没有 BN/Dropout），但换成 CNN/YOLO 后必须两者都加，否则推理结果会不稳定。

> [!danger] 常见陷阱
> 1. **推理时忘记 `model.eval()`** —— 导致 BatchNorm 统计量错误
> 2. **推理时忘记 `torch.no_grad()`** —— 显存占用翻倍，速度变慢
> 3. **训练时写了 `model.eval()`** —— 模型不会更新参数
> 4. **在 `torch.no_grad()` 块内调用 `loss.backward()`** —— 会报错，梯度无法计算

### 动手试试

```python
import torch
from torch import nn

model = nn.Sequential(
    nn.Linear(10, 5),
    nn.BatchNorm1d(5),  # 有 BatchNorm，训练/推理行为不同
    nn.ReLU(),
    nn.Linear(5, 3)
)

x = torch.randn(4, 10)
y_true = torch.randint(0, 3, (4,))

# ✅ 训练模式
model.train()
output = model(x)
print(f"训练模式输出: {output.shape}")

# ✅ 推理模式（完整写法）
model.eval()
with torch.no_grad():
    output = model(x)
    print(f"推理模式输出: {output.shape}")
    
# ❌ 推理时漏掉一个：
model.eval()  # 有 eval，没有 no_grad
output = model(x)  # 会占用显存记录计算图，浪费资源
print(f"半完全推理（有梯度）: {x.requires_grad}")
```

---

## 七、激活函数

线性模型（如 `nn.Linear(2,1)`）只能表达线性关系，即使堆叠多层，多个线性层叠加后本质上仍等价于一个线性层，表达能力有限。**激活函数给神经网络引入非线性，让它能学习复杂规律。**

### ReLU

```python
import torch
from torch import nn

relu = nn.ReLU()

x = torch.tensor([-2.0, -1.0, 0.0, 1.0, 2.0])
y = relu(x)

print("输入:", x)
print("ReLU 输出:", y)
# 输出: tensor([0., 0., 0., 1., 2.])
```

$$
\text{ReLU}(x) = \max(0, x)
$$

```python
model = nn.Sequential(
    nn.Linear(2, 4),
    nn.ReLU(),
    nn.Linear(4, 1)
)
```

> [!tip] 补充：YOLO 常用的激活函数不是 ReLU
> YOLO 系列（尤其 YOLOv5/v8）更常用 **SiLU（`nn.SiLU`，又叫 Swish）**，即 $x \cdot \sigma(x)$，比 ReLU 更平滑、梯度不会在负半轴直接归零。看到 YOLO 源码里的 `nn.SiLU()` 不用意外，原理和 ReLU 一样是"引入非线性"，只是曲线形状不同。

> [!example] 对比不同激活函数
> ```python
> import torch.nn as nn
> 
> x = torch.linspace(-3, 3, 100)
> relu = nn.ReLU()(x)
> silu = nn.SiLU()(x)
> 
> # ReLU: x < 0 时为 0，x > 0 时为 x（斜坡形）
> # SiLU: 更平滑的 S 形曲线，梯度更温和
> ```

### 动手试试

```python
import torch
from torch import nn

# 对比：线性模型 vs 有激活函数的模型
class LinearModel(nn.Module):
    def __init__(self):
        super().__init__()
        self.fc1 = nn.Linear(2, 4)
        self.fc2 = nn.Linear(4, 1)
    
    def forward(self, x):
        return self.fc2(self.fc1(x))

class NonlinearModel(nn.Module):
    def __init__(self):
        super().__init__()
        self.fc1 = nn.Linear(2, 4)
        self.fc2 = nn.Linear(4, 1)
    
    def forward(self, x):
        x = self.fc1(x)
        x = nn.ReLU()(x)
        x = self.fc2(x)
        return x

x = torch.randn(10, 2)
linear_model = LinearModel()
nonlinear_model = NonlinearModel()

print("线性模型参数数量:", sum(p.numel() for p in linear_model.parameters()))
print("非线性模型参数数量:", sum(p.numel() for p in nonlinear_model.parameters()))
# 参数数量相同，但非线性模型表达能力强
```

---

## 八、卷积层 Conv2d

卷积层用来提取图片中的特征，例如边缘、线条、形状等。**CNN 的核心就是卷积层**。

### 图片 Tensor 的形状

PyTorch 中图片的标准形状是：

```python
[B, C, H, W]
```

- `B`：batch（一批多少张图）
- `C`：channels（通道数，如 RGB=3，灰度=1）
- `H`：height
- `W`：width

> [!warning] OpenCV vs PyTorch 的通道顺序
> - **OpenCV**：`[H, W, C]` 且是 BGR 顺序
> - **PyTorch**：`[B, C, H, W]` 且通常转为 RGB 顺序
> 两者互转时需要：`permute()` 调整维度顺序 + `[..., ::-1]` 调整 BGR↔RGB

### 用法 1：`padding=1`

```python
import torch
from torch import nn

images = torch.rand(4, 3, 640, 640)

conv = nn.Conv2d(
    in_channels=3,
    out_channels=16,  # 输出16张特征图，可理解为16种不同的特征表示
    kernel_size=3,
    padding=1  # 补一圈像素，使宽高尽量不变
)

features = conv(images)

print("输入形状：", images.shape)   # torch.Size([4, 3, 640, 640])
print("输出形状：", features.shape) # torch.Size([4, 16, 640, 640])
```

### 用法 2：`stride=2`

```python
conv = nn.Conv2d(
    in_channels=3,
    out_channels=16,
    kernel_size=3,
    stride=2,
    padding=1
)

features = conv(images)
print("输出形状：", features.shape)  # torch.Size([4, 16, 320, 320])
```

`stride=2` 表示卷积核每次移动2个像素，图片高宽会缩小约一半，可以减少计算量，同时提取更高级的特征。

### 卷积 + ReLU

```python
import torch
from torch import nn

images = torch.rand(4, 3, 640, 640)

model = nn.Sequential(
    nn.Conv2d(
        in_channels=3,
        out_channels=16,
        kernel_size=3,
        stride=2,
        padding=1
    ),
    nn.ReLU()
)

features = model(images)

print("输入形状：", images.shape)   # torch.Size([4, 3, 640, 640])
print("输出形状：", features.shape) # torch.Size([4, 16, 320, 320])
```

### 重点

- `Conv2d`：提取图片特征
- `ReLU`：增加非线性
- `stride=2`：通常让图片尺寸缩小一半
- `[B, C, H, W]`：图片 Tensor 的标准形状

> [!danger] 常见陷阱
> 1. **输入通道数不对** —— RGB 图片应该用 `in_channels=3`，灰度图用 `in_channels=1`
> 2. **padding 设置不当** —— `padding=0` 会导致输出尺寸快速变小，通常用 `padding=kernel_size//2` 保持尺寸
> 3. **stride 太大** —— `stride=2` 会丢掉信息，不要盲目增大
> 4. **没有激活函数** —— 卷积后通常要接激活函数（ReLU 或 SiLU），否则还是线性的

### 动手试试

```python
import torch
from torch import nn

# 创建一个简单的卷积块
conv_block = nn.Sequential(
    nn.Conv2d(3, 32, kernel_size=3, padding=1, stride=1),
    nn.ReLU(),
    nn.Conv2d(32, 64, kernel_size=3, padding=1, stride=2),  # 缩小一半
    nn.ReLU()
)

# 创建 4 张 256×256 的 RGB 图片
images = torch.randn(4, 3, 256, 256)

output = conv_block(images)
print(f"输入形状: {images.shape}")
print(f"输出形状: {output.shape}")  # [4, 64, 128, 128]

# 查看参数数量
total_params = sum(p.numel() for p in conv_block.parameters())
print(f"总参数数: {total_params}")
```

---

## 九、批归一化 BatchNorm2d

卷积网络中常见的结构为：

```
Conv2d → BatchNorm2d → ReLU
```

BatchNorm2d 对卷积层的输出（形状 `[N, C, H, W]`）进行归一化，可以帮助：
- 稳定特征分布
- 加快训练
- 允许使用较大的学习率
- 减少训练过程中的不稳定

> [!tip] 注意
> 在卷积后紧接着 BatchNorm2d，这时候卷积通常不需要偏置 bias：
> ```python
> nn.Conv2d(..., bias=False)
> nn.BatchNorm2d(...)
> ```

### BatchNorm 的基本概念

```python
from torch import nn

bn = nn.BatchNorm2d(32)  # 32 是通道数

# BatchNorm 的参数
for name, param in bn.named_parameters():
    print(f"{name}: {param.shape}")
# weight (γ), bias (β)

# BatchNorm 的缓冲区（推理时使用）
for name, buffer in bn.named_buffers():
    print(f"{name}: {buffer.shape}")
# running_mean, running_var
```

### 训练 vs 推理的区别

```python
import torch
from torch import nn

bn = nn.BatchNorm2d(3)
x = torch.randn(4, 3, 32, 32)

# 训练：使用当前 batch 统计量
bn.train()
y_train = bn(x)

# 推理：使用累积统计量
bn.eval()
y_eval = bn(x)

# 输出不同！
print(torch.allclose(y_train, y_eval))  # False
```

### YOLO 风格的 CNN（Conv + BN + ReLU）

```python
import torch
from torch import nn

class YOLOStyleCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.layer1 = nn.Sequential(
            nn.Conv2d(3, 32, kernel_size=3, padding=1),
            nn.BatchNorm2d(32),
            nn.ReLU(inplace=True),  # inplace=True 节省显存
        )
        self.layer2 = nn.Sequential(
            nn.Conv2d(32, 64, kernel_size=3, padding=1, stride=2),
            nn.BatchNorm2d(64),
            nn.ReLU(inplace=True),
        )
        # ...其他层...
```

> [!danger] 常见陷阱
> 1. **推理时忘记 `model.eval()`** —— BatchNorm 用错统计量，结果不稳定
> 2. **训练时用了 `model.eval()`** —— 参数不更新
> 3. **多个 batch 推理时没 `eval()`** —— 运行统计量继续更新，导致结果漂移

---

## 十、完整 CNN：卷积 → 池化 → 展平 → 全连接

### 池化层

池化层在卷积层提取特征后使用，作用是：

1. 缩小图片的高宽
2. 减少计算量
3. 保留较明显的特征
4. 让模型对物体的小幅位置变化更稳定

常见池化层：

```python
import torch
from torch import nn

# MaxPool2d：取最大值
pool = nn.MaxPool2d(kernel_size=2, stride=2)

x = torch.rand(1, 3, 64, 64)
y = pool(x)

print("输入:", x.shape)   # [1, 3, 64, 64]
print("输出:", y.shape)   # [1, 3, 32, 32]

# AvgPool2d：取平均值
avg_pool = nn.AvgPool2d(kernel_size=2, stride=2)
y_avg = avg_pool(x)
print("平均池化输出:", y_avg.shape)  # [1, 3, 32, 32]
```

### 卷积、池化、ReLU 的区别

| 组件 | 作用 |
| ---- | ------------- |
| 卷积层  | 提取特征、改变通道数    |
| 池化层  | 压缩空间尺寸、保留重要信息 |
| ReLU / SiLU | 增加非线性能力       |

### 展平 Flatten

卷积层输出是四维数据 `[B, C, H, W]`，而全连接层 `nn.Linear` 通常需要二维输入 `[B, 特征数量]`，所以要把每张图展平成一行：

```python
[B, 特征数量] = [B, C*H*W]
例：输入 [2, 8, 32, 32] → 展平为 [2, 8192]  （8*32*32 = 8192）
```

```python
import torch
from torch import nn

# 展平层
flatten = nn.Flatten()

# 卷积输出
x = torch.randn(4, 16, 32, 32)
y = flatten(x)

print("输入:", x.shape)   # [4, 16, 32, 32]
print("输出:", y.shape)   # [4, 16384]  <- 16*32*32 = 16384
```

> [!danger] 常见陷阱
> 1. **计算展平后的特征数时算错** —— 必须是 `C * H * W`，常见错误是忘乘以通道数 `C`
> 2. **池化后忘记计算尺寸变化** —— 每次 `stride=2` 的池化会把高宽减半，要跟踪完整尺寸变化
> 3. 示例：
> ```python
> # 输入: [batch, 3, 224, 224]
> # Conv + stride=1, padding=1  -> [batch, 32, 224, 224]
> # MaxPool stride=2             -> [batch, 32, 112, 112]
> # Conv + stride=1, padding=1  -> [batch, 64, 112, 112]
> # MaxPool stride=2             -> [batch, 64, 56, 56]
> # Flatten                      -> [batch, 64*56*56] = [batch, 200704]
> ```

### CNN 的三大组件

| 组件   | 作用                                   | 类比            |
| ---- | ------------------------------------ | ------------- |
| 卷积层  | 用可学习的滤波器（卷积核）扫描输入，提取局部特征（边缘、纹理、颜色等） | 人眼视网膜细胞检测线条、色块 |
| 池化层  | 对特征图下采样，降低维度、增大感受野、引入平移不变性           | 压缩图像，保留重要信息    |
| 全连接层 | 将展平后的特征映射到最终的分类/回归输出                 | 大脑皮层综合判断      |

> [!tip] YOLO 视角
> YOLO 的 backbone（如 CSPDarknet）本质上就是大量"卷积+激活+归一化"堆叠，去掉了传统 CNN 分类网络末尾的全连接层，改用卷积直接输出特征图，再接检测头。理解本节的卷积/池化/展平即可看懂 backbone 结构图。

### 动手试试

```python
import torch
from torch import nn

# 完整的 CNN 网络
class SimpleCNN(nn.Module):
    def __init__(self):
        super().__init__()
        # Backbone：提取特征
        self.conv1 = nn.Conv2d(3, 32, kernel_size=3, padding=1)
        self.pool1 = nn.MaxPool2d(kernel_size=2, stride=2)
        
        self.conv2 = nn.Conv2d(32, 64, kernel_size=3, padding=1)
        self.pool2 = nn.MaxPool2d(kernel_size=2, stride=2)
        
        # Head：分类
        self.flatten = nn.Flatten()
        self.fc = nn.Linear(64 * 56 * 56, 10)  # 224/4 = 56
    
    def forward(self, x):
        # Conv -> ReLU -> Pool
        x = self.conv1(x)
        x = torch.relu(x)
        x = self.pool1(x)  # 224 -> 112
        
        x = self.conv2(x)
        x = torch.relu(x)
        x = self.pool2(x)  # 112 -> 56
        
        x = self.flatten(x)
        x = self.fc(x)
        return x

# 创建随机输入（4 张 224×224 的 RGB 图片）
images = torch.randn(4, 3, 224, 224)

model = SimpleCNN()
output = model(images)

print(f"输入: {images.shape}")
print(f"输出: {output.shape}")  # [4, 10] <- 4 张图，10 个类别分数

# 统计参数数
total_params = sum(p.numel() for p in model.parameters())
print(f"总参数数: {total_params:,}")

# 查看每层输出形状
x = images
print("\n--- 每层形状变化 ---")
print(f"输入: {x.shape}")
x = model.conv1(x); x = torch.relu(x)
print(f"Conv1: {x.shape}")
x = model.pool1(x)
print(f"Pool1: {x.shape}")
x = model.conv2(x); x = torch.relu(x)
print(f"Conv2: {x.shape}")
x = model.pool2(x)
print(f"Pool2: {x.shape}")
x = model.flatten(x)
print(f"Flatten: {x.shape}")
x = model.fc(x)
print(f"FC: {x.shape}")
```

---

## 十一、分类模型完整训练流程

这一节把前面内容串起来：图片 → 模型预测 → 类别分数 → 损失函数比较预测和标签 → 反向传播 → 优化器更新参数。

### 1. 分类任务是什么

假设有 3 个类别：

```text
0：猫
1：狗
2：汽车
```

一张狗的图片，标签是整数 `1`，**不是** one-hot 向量 `[0, 1, 0]`。使用 `CrossEntropyLoss` 时标签直接写整数类别号。

### 2. 模型输出是什么

假设模型最后一层是：

```python
nn.Linear(16 * 32 * 32, 3)
```

对每张图输出 3 个类别分数，例如 `[1.2, 4.5, 0.8]`（猫、狗、汽车），最高分对应预测类别。

```python
prediction = output.argmax(dim=1)
```

`argmax(dim=1)` 表示：在类别这一维中找分数最大的位置。

```python
output = torch.tensor([
    [1.2, 4.5, 0.8],
    [3.2, 1.1, 2.0]
])

prediction = output.argmax(dim=1)
print(prediction)
# tensor([1, 0])
```

即第1条数据预测为类别1，第2条数据预测为类别0。

### 3. `CrossEntropyLoss` 是什么

```python
loss_function = nn.CrossEntropyLoss()

output = torch.tensor([[1.2, 4.5, 0.8]])
label = torch.tensor([1])

loss = loss_function(output, label)
print(loss)
```

- `output.shape == [1, 3]`：1条数据，3个类别分数
- `label.shape == [1]`：1个标签，值为1（正确类别是第2类，从0开始数）

> [!warning] 重要注意点
> 使用 `CrossEntropyLoss` 时，模型最后**不需要**加 `Softmax`：
> ```python
> # 推荐
> nn.Linear(16 * 32 * 32, 3)
> ```
> 不要写成 `nn.Sequential(nn.Linear(...), nn.Softmax(dim=1))`，因为 `CrossEntropyLoss` 内部已经包含了 `log_softmax` 处理，重复加会导致损失计算错误、训练效果异常。

### 4. 构建分类模型

以 `128×128` 图片、3分类为例：

```python
import torch
from torch import nn

model = nn.Sequential(
    nn.Conv2d(in_channels=3, out_channels=8, kernel_size=3, padding=1),
    nn.ReLU(),
    nn.MaxPool2d(kernel_size=2, stride=2),

    nn.Conv2d(in_channels=8, out_channels=16, kernel_size=3, padding=1),
    nn.ReLU(),
    nn.MaxPool2d(kernel_size=2, stride=2),

    nn.Flatten(),
    nn.Linear(16 * 32 * 32, 3)
)

images = torch.rand(4, 3, 128, 128)
output = model(images)

print("图片形状：", images.shape)
print("输出形状：", output.shape)
```

形状变化：

```text
[4, 3, 128, 128]
↓ 第一层卷积
[4, 8, 128, 128]
↓ 第一次池化
[4, 8, 64, 64]
↓ 第二层卷积
[4, 16, 64, 64]
↓ 第二次池化
[4, 16, 32, 32]
↓ Flatten
[4, 16384]
↓ Linear
[4, 3]
```

`output.shape == torch.Size([4, 3])`：4张图片，每张输出3个类别分数。

### 5. 准备标签

```python
labels = torch.tensor([0, 1, 2, 1])
```

标签必须是整数类型（`torch.int64`，也叫 `torch.long`）：

```python
labels = torch.tensor([0, 1, 2, 1], dtype=torch.long)
```

### 6. 完整训练一次

```python
import torch
from torch import nn

model = nn.Sequential(
    nn.Conv2d(in_channels=3, out_channels=8, kernel_size=3, padding=1),
    nn.ReLU(),
    nn.MaxPool2d(kernel_size=2, stride=2),

    nn.Conv2d(in_channels=8, out_channels=16, kernel_size=3, padding=1),
    nn.ReLU(),
    nn.MaxPool2d(kernel_size=2, stride=2),

    nn.Flatten(),
    nn.Linear(16 * 32 * 32, 3)
)

images = torch.rand(4, 3, 128, 128)
labels = torch.tensor([0, 1, 2, 1], dtype=torch.long)

loss_function = nn.CrossEntropyLoss()
optimizer = torch.optim.Adam(model.parameters(), lr=0.001)

model.train()

output = model(images)
loss = loss_function(output, labels)

optimizer.zero_grad()
loss.backward()
optimizer.step()

prediction = output.argmax(dim=1)

print("输出形状：", output.shape)
print("预测类别：", prediction)
print("真实类别：", labels)
print("损失：", loss.item())
```

六个步骤：

| 步骤 | 代码 | 说明 |
|---|---|---|
| 1. 模型预测 | `output = model(images)` | 每行对应一张图，每列对应一个类别分数 |
| 2. 计算损失 | `loss = loss_function(output, labels)` | 比较最高分类别与真实标签 |
| 3. 清空梯度 | `optimizer.zero_grad()` | 清理上一次残留的梯度 |
| 4. 反向传播 | `loss.backward()` | 计算损失对参数的梯度 |
| 5. 更新参数 | `optimizer.step()` | 根据梯度修改卷积层/全连接层参数 |
| 6. 得到预测类别 | `prediction = output.argmax(dim=1)` | 注意这是**更新参数前**的预测结果 |

### 7. 循环训练多个 Epoch

```python
for epoch in range(10):
    model.train()

    output = model(images)
    loss = loss_function(output, labels)

    optimizer.zero_grad()
    loss.backward()
    optimizer.step()

    prediction = output.argmax(dim=1)
    accuracy = (prediction == labels).float().mean()

    print("第", epoch + 1, "轮", "损失：", loss.item(), "准确率：", accuracy.item())
```

准确率计算：`prediction == labels` → `[True, False, True, True]` → `.float()` → `[1.0, 0.0, 1.0, 1.0]` → `.mean()` → `0.75`（75%）。

### 8. 分类和 YOLO 的区别

分类模型输出 `[类别分数]`，例如 `[猫分数, 狗分数, 汽车分数]`。

YOLO 输出更复杂，因为它不仅要判断类别，还要预测物体位置：

```text
边界框位置（bounding box）
目标置信度（objectness）
类别分数（class scores）
```

但训练流程本质相同：`模型预测 → 计算损失 → 反向传播 → 更新参数`。

### 本课重点

- `nn.CrossEntropyLoss()`：用于多分类任务，内部已含 softmax，模型最后一层不要再加 Softmax
- `output.shape == [批次大小, 类别数量]`，例如 `[4, 3]`
- `labels.shape == [批次大小]`，例如 `[0, 1, 2, 1]`
- `output.argmax(dim=1)`：得到模型认为最可能的类别

---

## 十二、自定义 Dataset 与图片加载

### Dataset 的作用
Dataset 负责两个问题:
1. 数据集一共有多少条数据
2. 给定编号后，怎么取出一条数据

自定义 Dataset 通常需要实现三个方法：
```python
__init__()
__len__()
__getitem__()
```
示例
```python
import torch
from torch.utils.data import Dataset, DataLoader

class MyDataset(Dataset):
    def __init__(self):
        self.x = torch.tensor([
            [1.0, 2.0],
            [2.0, 3.0],
            [3.0, 4.0]
        ])

        self.y = torch.tensor([
            [3.0],
            [5.0],
            [7.0]
        ])

    def __len__(self):
        return len(self.x)
	# DataLoader依靠getitem方法批量读取数据
    def __getitem__(self, index):
        return self.x[index], self.y[index]

dataset = MyDataset()

print("数据数量：", len(dataset))
print("第0条数据：", dataset[0])
print("第1条数据：", dataset[1])
```

### 读取图片

#### 标准流程
```
图片文件
↓
读取图片
↓
调整大小
↓
转换为 Tensor
↓
调整维度顺序
↓
送入模型
```

#### 标准结构
```
dataset/
├── cat/
│   ├── cat1.jpg
│   └── cat2.jpg
├── dog/
│   ├── dog1.jpg
│   └── dog2.jpg
└── car/
    ├── car1.jpg
    └── car2.jpg
```
这时候PyTorch可以使用ImageFolder自动读取该结构

#### 使用ImageFolder
```python
from torchvision import datasets, transforms
from torch.utils.data import DataLoader

transform = transforms.Compose([
    transforms.Resize((128, 128)),
    transforms.ToTensor()
])

dataset = datasets.ImageFolder(
    root="dataset", # 文件夹
    transform=transform
)

data_loader = DataLoader(
    dataset,
    batch_size=4,
    shuffle=True
)
for images, labels in data_loader:
    print("图片形状：", images.shape)
    print("标签形状：", labels.shape)
    print("标签内容：", labels)
    break
print("类别名称：", dataset.classes)
print("类别编号：", dataset.class_to_idx)
print("图片数量：", len(dataset))
```
注意: 类别编号通常按照文件名称排序,不一定是直觉中的顺序

#### transforms的作用

```python
transform = transforms.Compose([
    transforms.Resize((128, 128)), # 神经网络需要相同大小的图片
    transforms.ToTensor() #将图片转为Tensor,调整为[C,H,W],将像素值从0~255缩放到0~1
])
```

---

## 十三、图片归一化与训练集/验证集

### 为什么要归一化

以下是一个归一化示例:
```python
transforms.Normalize(
    mean=[0.5, 0.5, 0.5],
    std=[0.5, 0.5, 0.5]
)
```
$$
x_{\text{new}}=\frac{x-\text{mean}}{\text{std}}
$$

### 归一化的作用

归一化是预处理，用来将像素值映射到一个合适的范围，让模型训练更加稳定，更快收敛，并提升最终性能。

### 训练集和验证集

训练集用于修改模型参数
验证集用于检查对没见过的图片是否有效

当训练集准确很高，但是新图片的准确率较低，这种只是记住图片而不是学会真正的规律的现象是**过拟合**

### 完整示例

使用 random_split 对数据集进行训练/验证集划分：

```python
from torch.utils.data import random_split

total_size = len(dataset)
train_size = int(total_size * 0.8) # 80%用来训练
val_size = total_size - train_size # 剩下的用来验证

# 随机划分数据集
train_dataset, val_dataset = random_split(
    dataset,
    [train_size, val_size]
)

# 加载训练集
train_loader = DataLoader(
    train_dataset,
    batch_size=4,
    shuffle=True
)

# 加载验证集
val_loader = DataLoader(
    val_dataset,
    batch_size=4,
    shuffle=False
)

# 训练阶段
for epoch in range(1000):
	model.train()
	train_correct = 0
	train_total = 0 
	train_loss_total = 0.0
	for images, labels in train_loader:
		output = model(images) # 前向传播
		loss = loss_function(output, labels) # 损失函数
		
		optimizer.zero_grad() # 清除梯度
		loss.backward() # 反向传播
		optimizer.step() # 更新
		
		train_loss_total += loss.item()
		
		prediction = output.argmax(dim=1)
		train_correct += (prediction == labels).sum().item()
		train_total += labels.size(0);
	train_loss = train_loss_total / len(train_loader)
	train_accuracy = train_correct / train_total
	
	# 验证阶段
	model.eval()
	
	val_correct = 0
	val_total = 0

	with torch.no_grad():
	    for images, labels in val_loader:
	        output = model(images)
	        prediction = output.argmax(dim=1)
	
	        val_correct += (prediction == labels).sum().item() # 计算正确数量
	        val_total += labels.size(0)
	
	accuracy = val_correct / val_total
	print("验证集准确率：", accuracy)
```

### 如何判断模型训练情况

#### 1. 训练准确率和验证准确率都很低
可能原因有:
1. 模型训练不够
2. 学习率不合适
3. 模型过于简单
4. 数据本身有问题

#### 2. 训练准确率高，验证准确率低
出现**过拟合**

---

## 十四、GPU 支持与设备管理

**YOLO 训练必须用 GPU**。

### 检查 GPU 并选择设备

```python
import torch

device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
print(f"使用设备: {device}")

# 查看 GPU 信息
if torch.cuda.is_available():
    print(f"GPU 名称: {torch.cuda.get_device_name(0)}")
    print(f"显存总量: {torch.cuda.get_device_properties(0).total_memory / 1e9:.2f} GB")
```

### 将模型和数据移到设备

```python
model = model.to(device)  # 移到 device（GPU 或 CPU）

for batch_x, batch_y in loader:
    batch_x = batch_x.to(device)  # ← 关键：每个 batch 都要移到 device
    batch_y = batch_y.to(device)
    
    output = model(batch_x)
    loss = criterion(output, batch_y)
    # ...
```

> [!danger] 常见陷阱
> 1. **模型在 GPU，数据在 CPU** —— `RuntimeError: Expected all tensors to be on the same device`
> 2. **GPU 显存溢出** —— 减小 `batch_size`
> 3. **忘记每个 batch 都 `.to(device)`**

### 训练循环中的典型写法

```python
model = MyModel().to(device)
optimizer = torch.optim.Adam(model.parameters())

for images, targets in loader:
    images = images.to(device)
    targets = targets.to(device)  # 如果需要

    optimizer.zero_grad()

    outputs = model(images)

    loss = compute_loss(outputs, targets)

    loss.backward()
    optimizer.step()
```

---

## 十五、模型保存、加载与 State Dict

**YOLO 权重文件（`.pt`）的核心就是 `state_dict`**。

### 保存和加载

```python
# 保存（推荐方式）
torch.save(model.state_dict(), 'model.pth')

# 加载
model = MyModel()
model.load_state_dict(torch.load('model.pth', map_location=device))
model.eval()
```

### 完整的 checkpoint（包含更多信息）

```python
checkpoint = {
    'epoch': 10,
    'model_state_dict': model.state_dict(),
    'optimizer_state_dict': optimizer.state_dict(),
    'loss': 0.123,
}
torch.save(checkpoint, 'checkpoint.pth')

# 加载
checkpoint = torch.load('checkpoint.pth', map_location=device)
model.load_state_dict(checkpoint['model_state_dict'])
optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
start_epoch = checkpoint['epoch']
```

> [!danger] 常见陷阱
> 1. **加载前忘记定义模型架构** —— 需要先创建相同的模型
> 2. **模型架构不匹配** —— `RuntimeError: size mismatch`
> 3. **忘记 `model.eval()`** —— 推理前必须调用
> 4. **跨设备加载没用 `map_location`** —— 必须指定目标设备

---

## 十六、YOLO 目标检测数据格式和边界框

前面的分类任务是一张图片一个类别，YOLO 的目标检测任务是一张图片对应一个或多个目标的位置和类别

> [!tip] 对应 YOLO 笔记
> 本节内容在 YOLO 视角下的整理与"为什么检测模型不能只改最后一层"的讨论，见 [[YOLO学习#第一课：从图像分类进入目标检测|YOLO 第一课]]。

### 1. 什么是边界框
```
左上角   ┌──────────┐
        │   目标    │
        └──────────┘  右下角
一般有: x_min,y_min,x_max,y_max
```

### 2. YOLO 边界框格式

YOLO 通常使用 `class_id x_center y_center width height` 表示边界框

例如
```
0 0.5 0.4 0.3 0.5 后四个数据已归一化
```

| 数值    | 含义       |
| ----- | -------- |
| `0`   | 类别编号     |
| `0.5` | 框中心点的横坐标 |
| `0.4` | 框中心点的纵坐标 |
| `0.3` | 框的宽度     |
| `0.5` | 框的高度     |

坐标归一化便于适应不同大小图片

### 文件结构

假设一张图片有两个目标，标签文件中写两行：
```
0 0.3125 0.375 0.3125 0.5 # 目标1
1 0.7 0.6 0.4 0.3 # 目标2
```

```
dataset/
├── images/
│   ├── train/
│   │   ├── image001.jpg
│   │   └── image002.jpg
│   └── val/
│       └── image003.jpg
└── labels/
    ├── train/
    │   ├── image001.txt
    │   └── image002.txt
    └── val/
        └── image003.txt
```

一个图片对应一个.txt文件，一个目标对应标签文件中的一行

### 用 PyTorch 表示一个目标框

PyTorch 中，一个目标可以表示为:
```python
targets = torch.tensor([
    [0, 0.3125, 0.375, 0.3125, 0.5],
    [1, 0.7, 0.6, 0.4, 0.3]
])
```

形状为
```
[2, 5]
2个目标，每个目标存在五个值，分别对应类别、中心x、中心y、宽、高
```

### 边界框格式转换为代码

```python
# 转换为 YOLO 边界框格式
def xyxy_to_yolo(x_min, y_min, x_max, y_max, image_width, image_height):
    x_center = (x_min + x_max) / 2
    y_center = (y_min + y_max) / 2

    box_width = x_max - x_min
    box_height = y_max - y_min
	# 返回归一化结果
    return [
        x_center / image_width,
        y_center / image_height,
        box_width / image_width,
        box_height / image_height
    ]

# 转换为标准边界框格式
def yolo_to_xyxy(
    x_center,
    y_center,
    box_width,
    box_height,
    image_width,
    image_height
):
    x_center *= image_width
    y_center *= image_height
    box_width *= image_width
    box_height *= image_height

    x_min = x_center - box_width / 2
    y_min = y_center - box_height / 2
    x_max = x_center + box_width / 2
    y_max = y_center + box_height / 2

    return [x_min, y_min, x_max, y_max]

result = yolo_to_xyxy(
    x_center=0.3125,
    y_center=0.375,
    box_width=0.3125,
    box_height=0.5,
    image_width=640,
    image_height=480
)
print(result)
```

---

## 十七、在图片上绘制和检查边界框

使用 OpenCV 来绘制边界框

> [!tip] 相关笔记
> - OpenCV 侧的绘图/坐标操作见 [[视觉学习/OpenCV学习笔记/_MOC|OpenCV 知识库]]（`cv2.rectangle` / `cv2.putText`）
> - YOLO 侧对可视化结果的解读见 [[YOLO学习#第二课：IoU、Precision、Recall、AP、mAP|YOLO 第二课]]

```python
BGR -> RGB cv2.cvtColor()
调整大小 cv2.resize()
numpy -> Tensor torch.from_numpy(image) 
0~255 -> 0~1 归一化
HWC -> CHW (OpenCV图片格式为[H,W,C],Pytorch为[C,H,W],需要permute())
增加批次维度 unsqueeze()
绘制边界框: cv2.rectangle()
绘制文字: cv2.putText()
```

> [!warning] 注意坐标顺序
> 用 OpenCV 画框时坐标是**像素值** `[x_min, y_min, x_max, y_max]`，而模型标签里存的是**归一化**的 `[x_center, y_center, w, h]`。画框前必须先用第 16 章的 `yolo_to_xyxy()` 转换，否则框会画到错误位置。

---

## 十八、IoU、NMS 与检测指标

这一章把"怎么判断一个预测框好不好"讲清楚：先用 IoU 衡量框的重叠程度，再用 Precision / Recall / AP / mAP 汇总成指标，最后用 NMS 去掉重复框。

> [!tip] 对应 YOLO 笔记
> 本章的直觉解释与图示见 [[YOLO学习#第二课：IoU、Precision、Recall、AP、mAP|YOLO 第二课：IoU、Precision、Recall、AP、mAP]]。

### 1. IoU 交并比

IoU 是两个框的交并比：
$$IoU = \frac{\text{交集面积}}{\text{并集面积}}$$

用于判断:
- 预测框和真实框是否匹配
- 计算目标检测指标
- 参与 YOLO 损失函数

> [!example] 用 PyTorch 计算两个框的 IoU
> ```python
> import torch
>
> def box_iou(box_a, box_b):
>     """box 格式: [x_min, y_min, x_max, y_max]"""
>     # 交集区域的左上角和右下角
>     inter_x1 = torch.max(box_a[0], box_b[0])
>     inter_y1 = torch.max(box_a[1], box_b[1])
>     inter_x2 = torch.min(box_a[2], box_b[2])
>     inter_y2 = torch.min(box_a[3], box_b[3])
>
>     # clamp(min=0) 处理两个框不相交的情况（宽高会出现负数）
>     inter_w = (inter_x2 - inter_x1).clamp(min=0)
>     inter_h = (inter_y2 - inter_y1).clamp(min=0)
>     inter_area = inter_w * inter_h
>
>     area_a = (box_a[2] - box_a[0]) * (box_a[3] - box_a[1])
>     area_b = (box_b[2] - box_b[0]) * (box_b[3] - box_b[1])
>     union = area_a + area_b - inter_area
>
>     return inter_area / union
>
> a = torch.tensor([0.0, 0.0, 10.0, 10.0])   # 面积 100
> b = torch.tensor([5.0, 5.0, 15.0, 15.0])   # 面积 100，交集 25
> print(box_iou(a, b))   # tensor(0.1429) -> 25 / 175
> ```

### 2. TP / FP / FN

| 缩写 | 全称 | 含义 |
|---|---|---|
| **TP** | True Positive | 真实存在且被正确检测（类别正确 **且** IoU 高于阈值） |
| **FP** | False Positive | 误检：不存在的目标被检测出来，或类别/IoU 不达标 |
| **FN** | False Negative | 漏检：真实目标没有被检测到 |

> [!warning] 只对类别不查 IoU 是常见错误
> 即使类别预测正确，如果预测框与真实框的 IoU 低于阈值（如 `0.5`），这条预测**不能**算 TP，通常计为 FP，同时对应的真实目标会形成 FN。

### 3. Precision 与 Recall

$$
Precision = \frac{TP}{TP + FP}
\qquad\qquad
Recall = \frac{TP}{TP + FN}
$$

- **Precision**（精确率）回答："检测出来的目标里，有多少是真的？" —— 反映**误报**多少
- **Recall**（召回率）回答："真实存在的目标里，有多少被找到了？" —— 反映**漏检**多少

> [!example] 举例
> 检测出 10 个目标，其中 2 个是误报：`TP=8, FP=2` → `Precision = 8/10 = 80%`
> 真实存在 10 个目标，只找到 8 个：`TP=8, FN=2` → `Recall = 8/10 = 80%`
> 类比：抓嫌疑人时，Precision 是"抓的人里真凶占比"，Recall 是"真凶里被抓到的占比"。

### 4. 置信度阈值

YOLO 会为每个预测结果给出置信度，例如：

```
猫：0.95
狗：0.82
汽车：0.61
人：0.38
```

- 阈值设为 `0.8` → 只保留猫和狗，检测更严谨（少误报）
- 阈值设为 `0.5` → 猫、狗、汽车都保留，检测更广泛（多目标）

> [!tip] 阈值与 Precision / Recall 的关系
> 降低置信度阈值会保留更多预测：Recall 可能提高，但误报也会增加、Precision 可能下降。反之提高阈值更严谨，却容易漏检。这个阈值就是把两者的权衡调到合适位置的旋钮。

### 5. AP 与 mAP

**AP（平均精度）** 是某个类别由 Precision-Recall 曲线计算出来的面积：

- 按置信度从高到低排列所有预测框
- 逐步增加保留的预测框，每次重新计算一次 Precision 和 Recall
- 得到一条 Precision-Recall 曲线
- 计算曲线下方的面积，即为 AP

```
AP_person = 0.80
AP_car    = 0.92
AP_dog    = 0.65
```

**mAP（平均精度均值）** 是所有类别 AP 的平均值：

```
mAP = (0.80 + 0.92 + 0.65) / 3 = 0.79
```

| 指标 | 含义 |
|---|---|
| `mAP@0.5` | IoU 阈值固定为 0.5 时的 mAP（较宽松） |
| `mAP@0.5:0.95` | 取 IoU 阈值 0.5 到 0.95 各档分别算 mAP 再取均值（更严格） |

> [!warning] 两个 mAP 差距大是什么意思
> 例如 `mAP@0.5 = 0.9` 但 `mAP@0.5:0.95 = 0.6`：说明模型**认得出**目标，但**框不准**（边界框回归能力弱），在更严格的 IoU 阈值下表现明显下降。

### 6. 指标速查

| 指标 | 主要反映 |
|---|---|
| Precision | 预测结果是否容易误报 |
| Recall | 是否容易漏检 |
| AP | 某一个类别的综合检测表现 |
| mAP | 所有类别的平均检测表现 |
| IoU | 预测框和真实框的位置重叠程度 |
| confidence | 模型对单个预测结果的信心 |

### 7. NMS 非极大值抑制

一张图片中，同一个目标可能产生多个重叠的预测框：

```
框 A：置信度 0.95
框 B：置信度 0.87
框 C：置信度 0.62
```

NMS 的基本步骤：

1. 选择置信度最高的框（此处是 A）
2. 删除与它 IoU 过高的其他框（即和 A 高度重叠的）
3. 在剩余框中继续选择置信度最高的框
4. 重复执行，直到没有可删除的框

> [!note] 待深入
> 本节的 NMS 只讲直觉与步骤。**NMS 的实现原理**（含 `torchvision.ops.nms` 的用法与 YOLO 后处理中的完整解码流程）会在 [[YOLO学习|YOLO 学习笔记]] 的第四课展开（待完成）。

### 本课重点

- `IoU` 衡量两个框的重叠程度，是判断预测是否命中的基础
- `Precision` 管误报，`Recall` 管漏检，置信度阈值在两者之间做权衡
- `AP` 是单类别的 P-R 曲线面积，`mAP` 是所有类别 AP 的均值
- `mAP@0.5:0.95` 比 `mAP@0.5` 严格，两者差距大说明**框不准**而非**认不出**
- `NMS` 用"置信度排序 + IoU 去重"输出最终检测结果

---

## 总结：从 PyTorch 基础到 YOLO 的完整路径

✅ **现在你已掌握**：
- 张量操作和自动求导（第 1-2 章）
- 模型定义和基本训练循环（第 3-7 章）
- CNN 完整架构（第 8-11 章）
- **从文件加载数据**（第 12 章）
- **数据预处理和集合划分**（第 13 章）
- **GPU 训练**（第 14 章）
- **模型保存/加载**（第 15 章）
- **YOLO 数据格式基础**（第 16-18 章）
- **检测评价指标**：IoU、TP/FP/FN、Precision、Recall、AP、mAP、NMS（第 18 章）

⚠️ **转向 YOLO 后还需学**（在 [[YOLO学习|YOLO 学习笔记]] 中继续）：
- YOLO 的整体网络结构：Backbone / Neck / Head（对应 [[YOLO学习#第三课：YOLO 整体网络结构|YOLO 第三课]]）
- 检测输出的解码与边界框预测参数（对应 YOLO 第四课，待完成）
- 数据格式转换（标注文件）（对应 YOLO 第五课，待完成）
- YOLO 损失函数设计
- YOLO 官方模型架构（YOLOv8）

🚀 **现在可以开始学 YOLO 了！** → [[YOLO学习|YOLO 学习笔记]]

---

## 速查表 & 常用代码片段

### 概念速查

| 概念 | 定义 | 对应 YOLO 用途 |
|---|---|---|
| **Tensor** | PyTorch 的基础数据结构，n 维数组 | 图片、权重、梯度都是张量 |
| **Autograd** | 自动求导，计算梯度 | 反向传播的核心，参数更新 |
| **nn.Module** | 神经网络模块基类 | 所有模型（YOLO 模型）都是继承它 |
| **Conv2d** | 二维卷积层 | YOLO backbone 由大量卷积层组成 |
| **BatchNorm2d** | 批归一化层 | 卷积后稳定训练 |
| **MaxPool2d** | 最大池化 | 降低空间维度，加速计算 |
| **Flatten** | 展平层 | 卷积输出转全连接输入 |
| **CrossEntropyLoss** | 多分类损失函数 | YOLO 分类头用的损失 |
| **DataLoader** | 数据加载器 | 组织图片 batch、打乱数据 |
| **Optimizer** | 优化器（SGD/Adam）| 根据梯度更新参数 |
| **model.train()** | 训练模式 | BatchNorm 用当前 batch 统计量 |
| **model.eval()** | 推理模式 | BatchNorm 用累积的统计量 |
| **torch.no_grad()** | 关闭梯度计算 | 推理时省显存、加速 |

### 常用代码片段

#### 1. 导入和基础设置

```python
import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader
from torch.optim import Adam, SGD

device = 'cuda' if torch.cuda.is_available() else 'cpu'
print(f"使用设备: {device}")
```

#### 2. 创建简单模型

```python
class MyModel(nn.Module):
    def __init__(self, input_size, output_size):
        super().__init__()
        self.fc1 = nn.Linear(input_size, 128)
        self.relu = nn.ReLU()
        self.fc2 = nn.Linear(128, output_size)
    
    def forward(self, x):
        x = self.fc1(x)
        x = self.relu(x)
        x = self.fc2(x)
        return x

model = MyModel(input_size=10, output_size=3).to(device)
```

#### 3. 标准训练循环

```python
def train_epoch(model, dataloader, criterion, optimizer, device):
    model.train()
    total_loss = 0.0
    
    for batch_idx, (x, y) in enumerate(dataloader):
        x, y = x.to(device), y.to(device)
        
        # 前向传播
        output = model(x)
        loss = criterion(output, y)
        
        # 反向传播
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        
        total_loss += loss.item()
        
        if (batch_idx + 1) % 10 == 0:
            print(f"Batch [{batch_idx+1}/{len(dataloader)}], Loss: {loss.item():.4f}")
    
    return total_loss / len(dataloader)

# 使用
criterion = nn.CrossEntropyLoss()
optimizer = Adam(model.parameters(), lr=0.001)

for epoch in range(10):
    avg_loss = train_epoch(model, train_loader, criterion, optimizer, device)
    print(f"Epoch {epoch+1}: Avg Loss = {avg_loss:.4f}")
```

#### 4. 推理

```python
def predict(model, x, device):
    model.eval()
    with torch.no_grad():
        x = x.to(device)
        output = model(x)
        predictions = output.argmax(dim=1)
    return predictions

# 使用
test_data = torch.randn(4, 10).to(device)
preds = predict(model, test_data, device)
print(f"预测结果: {preds}")
```

#### 5. 自定义 Dataset（YOLO 会用）

```python
class CustomDataset(Dataset):
    def __init__(self, x, y):
        self.x = torch.tensor(x, dtype=torch.float32)
        self.y = torch.tensor(y, dtype=torch.long)
    
    def __len__(self):
        return len(self.x)
    
    def __getitem__(self, idx):
        return self.x[idx], self.y[idx]

# 使用
dataset = CustomDataset(x_data, y_data)
loader = DataLoader(dataset, batch_size=32, shuffle=True, num_workers=4)
```

#### 6. 保存和加载模型

```python
# 保存
torch.save(model.state_dict(), 'model.pth')

# 加载
model = MyModel(input_size=10, output_size=3)
model.load_state_dict(torch.load('model.pth', map_location=device))
model.to(device)
```

#### 7. 卷积网络实例（接近 YOLO backbone 风格）

```python
class SimpleCNNBackbone(nn.Module):
    def __init__(self):
        super().__init__()
        self.conv1 = nn.Conv2d(3, 32, kernel_size=3, padding=1, stride=1)
        self.bn1 = nn.BatchNorm2d(32)
        self.relu = nn.ReLU()
        self.pool = nn.MaxPool2d(kernel_size=2, stride=2)
        
        self.conv2 = nn.Conv2d(32, 64, kernel_size=3, padding=1, stride=1)
        self.bn2 = nn.BatchNorm2d(64)
        
    def forward(self, x):
        x = self.conv1(x)
        x = self.bn1(x)
        x = self.relu(x)
        x = self.pool(x)  # 224 -> 112
        
        x = self.conv2(x)
        x = self.bn2(x)
        x = self.relu(x)
        x = self.pool(x)  # 112 -> 56
        
        return x

backbone = SimpleCNNBackbone().to(device)
x = torch.randn(4, 3, 224, 224).to(device)
features = backbone(x)  # [4, 64, 56, 56]
```

### 常见错误排查

| 错误 | 原因 | 解决方案 |
|---|---|---|
| `RuntimeError: Expected 2D input` | 输入维度不对 | 检查 `x.shape`，用 `.reshape()` / `.unsqueeze()` 调整 |
| `RuntimeError: grad can be implicitly created only for scalar outputs` | 在向量上调用 `backward()` | 先 `loss = output.sum()` 或 `loss = loss_fn(output, target)` |
| `CUDA out of memory` | 显存不足 | 减小 `batch_size` 或 `model` 参数数量 |
| `模型参数不更新` | 忘记 `optimizer.step()` 或 `loss.backward()` | 检查训练循环的 5 个步骤是否都有 |
| `推理结果不稳定` | 推理时忘记 `model.eval()` | 必须同时调用 `model.eval()` 和 `torch.no_grad()` |
| `梯度累加` | 忘记 `optimizer.zero_grad()` | 每个 epoch/batch 开始调用 `zero_grad()` |

---

## 学完这篇笔记，下一步是什么？

✅ **完成本笔记后应该掌握**：
- PyTorch 张量基本操作（shape、reshape、permute）
- 自动求导和反向传播
- 模型定义、训练、推理的完整流程
- CNN 的基本原理（卷积、池化、展平、全连接）
- 分类任务的标准实现
- 数据加载和预处理
- GPU 训练和模型保存
- 检测指标：IoU、Precision、Recall、AP、mAP、NMS

**下一个目标**：进入 [[YOLO学习|YOLO 目标检测学习笔记]]，继续学习目标检测；全景路线见 [[视觉学习/_MOC|视觉学习知识库总览]]。

---

> [!success] 恭喜！
> 
> 学完这篇笔记，你已经掌握了 PyTorch 的核心概念和标准用法，这是入门深度学习和目标检测的坚实基础。
> 
> 关键记住：
> - **代码优于理论** —— 每节都要跑一遍，修改参数看效果
> - **YOLO 只是应用** —— 本笔记的所有概念都会在 YOLO 中直接用到
> - **调试是常态** —— 报错很正常，看错误信息、查对应章节、再试一遍，这就是学习过程
