---
title: OpenCV 第五课 图像平滑与滤波
type: 课程笔记
课程: 5
created: 2026-09-03
updated: 2026-09-03
tags:
  - opencv
  - 课程笔记
  - 平滑滤波
status: 学习中
---

# OpenCV第五课 图像平滑与滤波

> **本课目标**：掌握图像滤波的基本思想（卷积核），以及 OpenCV 的四种常用滤波——**均值、高斯、中值、双边滤波**。理解各自的原理、适用场景与优缺点，并能根据噪声类型和边缘保护需求选择合适的方法。
> **涉及主题**：[[OpenCV学习笔记/主题模块/图像处理]]

---

## 一、滤波基本思想：卷积核

### 概念

滤波可以理解为：使用一个小窗口，在图像上**逐像素移动**，根据窗口内的像素计算**中心位置**的新像素值。这个小窗口就是**卷积核（Kernel）**。

### 示例

一个 3×3 的均值滤波核：

```
1/9 ×
[ 1  1  1
  1  1  1
  1  1  1 ]
```

某个像素区域为：

```
[ 10  20  30
  20 100  30
  10  20  30 ]
```

中心像素值为 100，均值滤波会计算 9 个像素的平均值，然后将中心的像素值更改为该平均值，从而降低局部突变，使其平滑。

### 卷积核大小

卷积核大小一般为**奇数 × 奇数**，例如：

```cpp
cv::Size(3,3) // 平滑效果一般，但保留细节较多
cv::Size(5,5) // 平滑效果中
cv::Size(7,7) // 平滑效果强，细节损失多
```

### 核心要点

1. 滤波核心 = **卷积核**：小窗口在图像上逐像素滑动，用窗口内像素计算中心新值。
2. 卷积核大小通常为**奇数 × 奇数**（3×3、5×5、7×7）；**核越大，平滑越强、细节损失越多**。
3. 均值核 = 所有像素取平均（`1/9 ×` 全 1 矩阵），用于降低局部突变。

---

## 二、各种滤波

### 均值滤波

用窗口中所有像素的**平均值**替换中心像素。

#### 1. cv::blur()

```cpp
void cv::blur(
    InputArray src,               // 输入图
    OutputArray dst,              // 输出图
    Size ksize,                   // 卷积核大小
    Point anchor = Point(-1, -1), // 锚点，默认核中心
    int borderType = BORDER_DEFAULT
);
```

**Python 对照**：
```python
dst = cv2.blur(src, (5, 5))  # -> np.ndarray
```

- 即窗口中所有像素的平均值替换中心像素。
- 等同于使用**归一化**的方框滤波（`boxFilter()`）。
- 问题：同时模糊噪声和边缘，**不适合特别强调边缘**的场景。

#### 2. cv::boxFilter()

```cpp
void cv::boxFilter(
    InputArray src,               // 输入图
    OutputArray dst,              // 输出图
    int ddepth,                   // 输出深度，-1 表示与输入相同
    Size ksize,                   // 卷积核大小
    Point anchor = Point(-1, -1), // 锚点，默认核中心
    bool normalize = true,        // 是否归一化
    int borderType = BORDER_DEFAULT
);
```

**Python 对照**：
```python
dst = cv2.boxFilter(src, -1, (5, 5), normalize=True)  # -> np.ndarray
```

### 高斯滤波

特点：**距离中心越近的像素权重越大**，越远的像素权重越小。

```cpp
void cv::GaussianBlur(
    InputArray src,      // 输入图
    OutputArray dst,     // 输出图
    Size ksize,          // 卷积核大小
    double sigmaX,       // X 方向标准差（0 表示按核大小自动计算）
    double sigmaY = 0,   // Y 方向标准差（0 表示与 sigmaX 相同）
    int borderType = BORDER_DEFAULT
);
// 或者手动指定分布参数：
cv::GaussianBlur(
    src,
    dst,
    cv::Size(),  // 卷积核大小
    1.0,         // 用于控制高斯分布的范围
    1.0
);
```

**Python 对照**：
```python
dst = cv2.GaussianBlur(src, (5, 5), 0)        # sigmaX=0 自动按核大小计算   # -> np.ndarray
dst = cv2.GaussianBlur(src, (5, 5), 1.0, 1.0) # 手动指定 sigmaX/sigmaY     # -> np.ndarray
```

**常用场景**：
- 一般的图像去噪
- Canny 边缘检测前的预处理
- 减少细小纹理、平滑图像、降低高频噪声

### 中值滤波

取出邻域内的所有像素，取排序后的**中位数**作为中心像素。特别适合去除**椒盐噪声**（随机的黑点/白点）。

```cpp
void cv::medianBlur(
    InputArray src,   // 输入图
    OutputArray dst,  // 输出图
    int ksize         // 窗口大小，必须为奇数
);
```

**Python 对照**：
```python
dst = cv2.medianBlur(src, 5)   # 窗口大小必须为奇数   # -> np.ndarray
```

**常用场景**：
- 去除随机的黑白点
- 比均值滤波更能保护边缘
- 适合扫描图像和摄像头的噪声

### 双边滤波

目标：在**去除噪声的同时尽可能保留边缘**。

机制：在考虑空间距离的同时，也考虑两个像素的**像素值差异**——差异越大，相互影响越弱；差异越小，相互影响越大。

缺点：速度较慢，大尺寸图像会进行大量重复处理。

```cpp
void cv::bilateralFilter(
    InputArray src,        // 输入图
    OutputArray dst,       // 输出图
    int d,                 // 邻域的直径
    double sigmaColor,     // 颜色或灰度差异的影响范围：值越大，越容易把颜色差异大的混合，平滑效果越好
    double sigmaSpace,     // 空间距离的影响范围
    int borderType = BORDER_DEFAULT
);
```

**Python 对照**：
```python
dst = cv2.bilateralFilter(src, 9, 75, 75)   # (d, sigmaColor, sigmaSpace)   # -> np.ndarray
```

**常用场景**：
- 人像平滑
- 保留轮廓的降噪
- 保持边缘的彩色图像处理
- 纹理降低但轮廓保留的场景

### 四种滤波方法的对比

| 滤波方式 | 主要思想 | 去除椒盐噪声 | 边缘保护 | 速度 |
| --- | --- | --- | --- | --- |
| 均值滤波 | 邻域求平均 | 一般 | 较差 | 快 |
| 高斯滤波 | 加权平均 | 较好 | 一般 | 较快 |
| 中值滤波 | 邻域取中间值 | 很好 | 较好 | 中等 |
| 双边滤波 | 同时考虑空间和像素差异 | 较好 | 很好 | 较慢 |

### 核心要点

1. **均值滤波**（`blur` / `boxFilter`）：邻域求平均，简单快，但同时模糊噪声与边缘。
2. **高斯滤波**（`GaussianBlur`）：按距中心的距离**加权平均**，去噪更自然，是 Canny 前常用预处理。
3. **中值滤波**（`medianBlur`）：取邻域**中位数**，抗椒盐噪声最佳，且更保护边缘。
4. **双边滤波**（`bilateralFilter`）：同时考虑空间距离与像素值差异，**保边去噪最好**，但最慢，大图慎用。
5. **选型参考**：椒盐噪声 → 中值；普通去噪/预处理 → 高斯；人像保边 → 双边。

---

## 三、API 对照

| 功能 | C++ | Python |
| --- | --- | --- |
| 均值滤波 | `cv::blur` / `cv::boxFilter` | `cv2.blur` / `cv2.boxFilter` |
| 高斯滤波 | `cv::GaussianBlur` | `cv2.GaussianBlur` |
| 中值滤波 | `cv::medianBlur` | `cv2.medianBlur` |
| 双边滤波 | `cv::bilateralFilter` | `cv2.bilateralFilter` |

---

## 四、代码示例

### C++ 示例

```cpp
// C++ 示例
#include <opencv2/opencv.hpp>
#include <QDebug>

int main()
{
    const std::string imagePath =
        R"(C:\Users\fanyu\Downloads\qq_pic_merged_1788356484263.jpg)";

    // 读取彩色图像
    cv::Mat image = cv::imread(
        imagePath,
        cv::IMREAD_COLOR
    );

    // 必须在处理之前检查读取结果
    if (image.empty()) {
        qDebug() << "图像读取失败";
        qDebug() << "请检查路径是否正确";
        return 1;
    }

    cv::Mat meanResult;
    cv::Mat gaussianResult;
    cv::Mat medianResult;
    cv::Mat bilateralResult;

    // 1. 均值滤波
    cv::blur(
        image,
        meanResult,
        cv::Size(5, 5)
    );

    // 2. 高斯滤波
    cv::GaussianBlur(
        image,
        gaussianResult,
        cv::Size(5, 5),
        0
    );

    // 3. 中值滤波
    cv::medianBlur(
        image,
        medianResult,
        5
    );

    // 4. 双边滤波
    cv::bilateralFilter(
        image,
        bilateralResult,
        9,
        75,
        75
    );

    cv::imshow("Original", image);
    cv::imshow("Mean Blur", meanResult);
    cv::imshow("Gaussian Blur", gaussianResult);
    cv::imshow("Median Blur", medianResult);
    cv::imshow("Bilateral Filter", bilateralResult);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}
```

### Python 示例

```python
# Python 示例
import cv2

image_path = r"C:\Users\fanyu\Downloads\qq_pic_merged_1788356484263.jpg"

image = cv2.imread(
    image_path,
    cv2.IMREAD_COLOR
)  # -> np.ndarray / None

if image is None:
    print("图像读取失败")
    print("请检查图片路径是否正确")
    exit(1)

# 均值滤波
mean_result = cv2.blur(
    image,
    (5, 5)
)  # -> np.ndarray

# 高斯滤波
gaussian_result = cv2.GaussianBlur(
    image,
    (5, 5),
    0
)  # -> np.ndarray

# 中值滤波
median_result = cv2.medianBlur(
    image,
    5
)  # -> np.ndarray

# 双边滤波
bilateral_result = cv2.bilateralFilter(
    image,
    9,
    75,
    75
)  # -> np.ndarray

cv2.imshow("Original", image)
cv2.imshow("Mean Blur", mean_result)
cv2.imshow("Gaussian Blur", gaussian_result)
cv2.imshow("Median Blur", median_result)
cv2.imshow("Bilateral Filter", bilateral_result)

cv2.waitKey(0)
cv2.destroyAllWindows()
```

---

## 五、本节小结

- 滤波 = 用**卷积核**在图像上滑动，用窗口内像素计算中心新值；核通常为**奇数**大小。
- **均值滤波**（`blur` / `boxFilter`）简单快，但会同时模糊噪声与边缘。
- **高斯滤波**（`GaussianBlur`）按距离加权，去噪更自然，常用于 Canny 预处理。
- **中值滤波**（`medianBlur`）去除**椒盐噪声**最佳，且更保护边缘。
- **双边滤波**（`bilateralFilter`）保边去噪最强，但速度最慢，大图慎用。
- 选型口诀：椒盐 → 中值；普通去噪 → 高斯；保边人像 → 双边。

---

## 相关链接

- 上一课：[[OpenCV第四课 阈值处理和二值化]]
- 下一课：[[OpenCV第六课 形态学处理]]
- 主题归纳：[[OpenCV学习笔记/主题模块/图像处理]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
