---
title: OpenCV 第六课 形态学处理
type: 课程笔记
课程: 6
created: 2026-09-03
updated: 2026-09-03
tags:
  - opencv
  - 课程笔记
  - 形态学
status: 学习中
---

# OpenCV第六课 形态学处理

> **本课目标**：掌握形态学处理的基础——**结构元素（Kernel）**，以及**腐蚀、膨胀**两大基本运算；进而理解并运用**开运算、闭运算、形态学梯度、顶帽、黑帽**等组合运算，学会用它们处理二值图像（去噪、补洞、提取轮廓与亮/暗区域）。
> **涉及主题**：[[OpenCV学习笔记/主题模块/图像处理]]

---

## 一、结构元素（Kernel）

### 概念

结构元素是形态学处理的"形状模板"，常见形状有：

```C++
cv::MORPH_RECT      // 矩形
cv::MORPH_ELLIPSE   // 椭圆
cv::MORPH_CROSS     // 十字形
```

创建结构元素（以 5×5 矩形为例）：

```c++
// Mat getStructuringElement(int shape, Size ksize, Point anchor = Point(-1,-1))
cv::Mat kernel = cv::getStructuringElement(
    cv::MORPH_RECT,      // 形状：RECT / ELLIPSE / CROSS
    cv::Size(5, 5)       // 尺寸，一般为奇数
);
```

**Python 对照**：
```python
kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))  # -> np.ndarray
```

### 前景与背景

形态学处理主要作用于**二值图像**，约定：

- **前景目标**：白色，像素值为 255
- **背景**：黑色，像素值为 0

---

## 二、基本运算：腐蚀与膨胀

### 腐蚀 Erode

使用结构元素在图像上滑动，若结构元素**不能完全放入**白色区域，就把当前位置变成黑色。本质为取**局部区域中的最小值**。

效果：
```
让白色目标的边界向内收缩
目标变细
目标变小
细小白色噪声可能消失
相邻目标可能被分离
```

```c++
cv::Mat eroded;
cv::erode(binary, eroded, kernel);   // void erode(...)：结果写入 dst

// 完整写法
void cv::erode(
    InputArray src,                 // 输入图
    OutputArray dst,                // 输出图
    InputArray kernel,              // 结构元素
    Point anchor = Point(-1, -1),   // 锚点，(-1,-1) 表示中心
    int iterations = 1,             // 操作次数
    int borderType = BORDER_CONSTANT,
    const Scalar& borderValue = morphologyDefaultBorderValue()
);
```

**Python 对照**：
```python
eroded = cv2.erode(binary, kernel)  # -> np.ndarray
# 完整写法
eroded = cv2.erode(src, kernel, anchor=(-1, -1), iterations=1)  # -> np.ndarray
```

**腐蚀的作用**：
1. 去除小白点
2. 去除目标边缘的毛刺
3. 让目标变细
4. 分离轻微粘连的目标
5. 减少白色前景区域

### 膨胀 Dilate

膨胀是腐蚀的**反操作**，本质为取**局部区域中的最大值**。

效果：
```
目标变粗
目标变大
填补小缝隙
连接相邻的白色区域
```

```c++
cv::Mat dilated;
cv::dilate(binary, dilated, kernel);   // void dilate(...)：结果写入 dst

// 完整写法
void cv::dilate(
    InputArray src,                 // 输入图
    OutputArray dst,                // 输出图
    InputArray kernel,              // 结构元素
    Point anchor = Point(-1, -1),   // 锚点，(-1,-1) 表示中心
    int iterations = 1,             // 操作次数
    int borderType = BORDER_CONSTANT,
    const Scalar& borderValue = morphologyDefaultBorderValue()
);
```

**Python 对照**：
```python
dilated = cv2.dilate(binary, kernel)  # -> np.ndarray
dilated = cv2.dilate(binary, kernel, anchor=(-1, -1), iterations=1)  # -> np.ndarray
```

**膨胀的作用**：
1. 填补小间隙
2. 连接断裂的目标
3. 让目标区域变粗
4. 填充小孔洞
5. 连接相邻的白色区域

### 腐蚀与膨胀对比

| 操作 | 白色前景的变化 | 常见用途 |
| --- | --- | --- |
| 腐蚀 | 变小、变细 | 去除小白点、分离目标 |
| 膨胀 | 变大、变粗 | 填补间隙、连接区域 |

---

## 三、组合运算

### 开运算：先腐蚀后膨胀

```c++
// void morphologyEx(InputArray src, OutputArray dst, int op, InputArray kernel,
//                   Point anchor = Point(-1,-1), int iterations = 1, ...)
cv::Mat opened;
cv::morphologyEx(
    binary,
    opened,
    cv::MORPH_OPEN,   // 形态学中的开运算
    kernel
);
```

**Python 对照**：
```python
opened = cv2.morphologyEx(binary, cv2.MORPH_OPEN, kernel)  # -> np.ndarray
```

- 常用于清除**小白点 / 粘连物**
- ⚠️ 注意结构元素的尺寸：若结构元素比处理目标更大，目标很可能被直接腐蚀掉！

### 闭运算：先膨胀后腐蚀

```c++
cv::Mat closed;
cv::morphologyEx(
    binary,
    closed,
    cv::MORPH_CLOSE,  // 形态学中的闭运算
    kernel
);
```

**Python 对照**：
```python
closed = cv2.morphologyEx(binary, cv2.MORPH_CLOSE, kernel)  # -> np.ndarray
```

- 常用于**填充小黑洞 / 连接断裂的白色目标**

### 形态学梯度：膨胀结果 − 腐蚀结果

`Gradient = Dilate(src) - Erode(src)`

```c++
cv::Mat gradient;
cv::morphologyEx(
    binary,
    gradient,
    cv::MORPH_GRADIENT,  // 梯度计算
    kernel
);
```

**Python 对照**：
```python
gradient = cv2.morphologyEx(binary, cv2.MORPH_GRADIENT, kernel)  # -> np.ndarray
```

- 作用：获取目标的**轮廓**，辅助边缘分析

### 顶帽 TopHat

`TopHat = src - Open`（原图 − 原图开运算）

```c++
cv::Mat topHat;
cv::morphologyEx(
    gray,
    topHat,
    cv::MORPH_TOPHAT,  // 顶帽
    kernel
);
```

**Python 对照**：
```python
top_hat = cv2.morphologyEx(gray, cv2.MORPH_TOPHAT, kernel)  # -> np.ndarray
```

- 作用：提取**比周围更亮**的小区域，如灰色背景中的小白点、明亮的小缺陷、局部的亮纹理、粘连的物体等

### 黑帽 BlackHat

`BlackHat = Close - src`（原图闭运算 − 原图）

```c++
cv::Mat blackHat;
cv::morphologyEx(
    gray,
    blackHat,
    cv::MORPH_BLACKHAT,  // 黑帽
    kernel
);
```

**Python 对照**：
```python
black_hat = cv2.morphologyEx(gray, cv2.MORPH_BLACKHAT, kernel)  # -> np.ndarray
```

- 作用：提取**比周围更暗**的小目标，如浅色背景中的黑色文字、图像中的暗色裂纹、暗色缺陷等

### 核心要点

1. **结构元素（Kernel）**：形态学的基本形状（矩形/椭圆/十字形），尺寸一般取奇数，**越大处理强度越大**。
2. **腐蚀 = 局部最小**（白色收缩），**膨胀 = 局部最大**（白色扩张），二者互为反操作。
3. **开运算 = 先腐后膨**：去小白点、去粘连物；**闭运算 = 先膨后腐**：填黑洞、连断裂目标。
4. **梯度 = 膨胀 − 腐蚀**：提取目标轮廓。
5. **顶帽 = 原图 − 开**：提取亮区域；**黑帽 = 闭 − 原图**：提取暗区域。

---

## 四、代码示例

### C++ 示例

```cpp
// C++ 示例
#include <opencv2/opencv.hpp>

int main()
{
    // 读取灰度图像
    cv::Mat src = cv::imread("path/to/image.jpg", cv::IMREAD_GRAYSCALE);
    if (src.empty()) {
        std::cout << "图像读取失败" << std::endl;
        return 1;
    }

    // 二值化（形态学处理一般在二值图像上进行）
    cv::Mat binary;
    cv::threshold(src, binary, 127, 255, cv::THRESH_BINARY);

    // 结构元素
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(5, 5)
    );

    cv::Mat eroded, dilated, opened, closed, gradient, topHat, blackHat;

    cv::erode(binary, eroded, kernel);                                  // 腐蚀
    cv::dilate(binary, dilated, kernel);                                // 膨胀
    cv::morphologyEx(binary, opened, cv::MORPH_OPEN, kernel);           // 开运算
    cv::morphologyEx(binary, closed, cv::MORPH_CLOSE, kernel);          // 闭运算
    cv::morphologyEx(binary, gradient, cv::MORPH_GRADIENT, kernel);     // 形态学梯度
    cv::morphologyEx(binary, topHat, cv::MORPH_TOPHAT, kernel);         // 顶帽
    cv::morphologyEx(binary, blackHat, cv::MORPH_BLACKHAT, kernel);     // 黑帽

    cv::imshow("Binary", binary);
    cv::imshow("Erode", eroded);
    cv::imshow("Dilate", dilated);
    cv::imshow("Open", opened);
    cv::imshow("Close", closed);
    cv::imshow("Gradient", gradient);
    cv::imshow("TopHat", topHat);
    cv::imshow("BlackHat", blackHat);

    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;
}
```

### Python 示例

```python
# Python 示例
import cv2

src = cv2.imread("path/to/image.jpg", cv2.IMREAD_GRAYSCALE)  # -> np.ndarray / None
if src is None:
    print("图像读取失败")
    exit(1)

# 二值化
_, binary = cv2.threshold(src, 127, 255, cv2.THRESH_BINARY)  # -> (float, np.ndarray)

# 结构元素
kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))  # -> np.ndarray

eroded    = cv2.erode(binary, kernel)                          # 腐蚀    -> np.ndarray
dilated   = cv2.dilate(binary, kernel)                         # 膨胀    -> np.ndarray
opened    = cv2.morphologyEx(binary, cv2.MORPH_OPEN, kernel)   # 开运算  -> np.ndarray
closed    = cv2.morphologyEx(binary, cv2.MORPH_CLOSE, kernel)  # 闭运算  -> np.ndarray
gradient  = cv2.morphologyEx(binary, cv2.MORPH_GRADIENT, kernel)  # -> np.ndarray
tophat    = cv2.morphologyEx(binary, cv2.MORPH_TOPHAT, kernel)    # -> np.ndarray
blackhat  = cv2.morphologyEx(binary, cv2.MORPH_BLACKHAT, kernel)  # -> np.ndarray

for name, img in [("Binary", binary), ("Erode", eroded), ("Dilate", dilated),
                  ("Open", opened), ("Close", closed), ("Gradient", gradient),
                  ("TopHat", tophat), ("BlackHat", blackhat)]:
    cv2.imshow(name, img)

cv2.waitKey(0)
cv2.destroyAllWindows()
```

---

## 五、本节小结

- **结构元素（Kernel）**：矩形 / 椭圆 / 十字形三种形状，尺寸一般取奇数，越大处理强度越大。
- **腐蚀** = 取局部最小，白色收缩——去小白点、分离粘连目标。
- **膨胀** = 取局部最大，白色扩张——填补间隙、连接断裂目标。
- **开运算**（先腐后膨）去小白点 / 粘连物；**闭运算**（先膨后腐）填黑洞 / 连断裂。
- **形态学梯度**（膨胀 − 腐蚀）提取轮廓；**顶帽**（原图 − 开）提取亮区；**黑帽**（闭 − 原图）提取暗区。
- 形态学运算一般作用于**二值图像**，注意**结构元素尺寸不要大于处理目标**。

---

## 相关链接

- 上一课：[[OpenCV第五课 图像平滑与滤波]]
- 下一课：[[OpenCV第七课 轮廓检测与目标分析]]
- 主题归纳：[[OpenCV学习笔记/主题模块/图像处理]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
