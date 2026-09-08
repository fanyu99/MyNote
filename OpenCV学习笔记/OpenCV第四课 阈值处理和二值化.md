---
title: OpenCV 第四课 阈值处理与二值化
type: 课程笔记
课程: 4
created: 2026-09-02
updated: 2026-09-02
tags:
  - opencv
  - 课程笔记
  - 阈值处理
status: 学习中
---

# OpenCV第四课 阈值处理与二值化

> **本课目标**：掌握 OpenCV 的**阈值处理与二值化**——理解阈值与二值化的概念，掌握固定阈值 `cv::threshold` 的 5 种阈值类型、自适应阈值 `cv::adaptiveThreshold`（均值/高斯）与 Otsu 自动阈值，并能在 C++/Python 中熟练运用。
> **涉及主题**：[[OpenCV学习笔记/主题模块/图像处理]]

---

## 一、什么是阈值，什么是二值化

### 概念

#### 阈值

对于**灰度图**来说，选择的一个特定的边界值，用于筛选每个像素。灰度值范围 0~255，**255 白，0 黑**。

#### 二值化

将**灰度图**图像依照阈值进行处理，最后仅剩黑白两种颜色。具体用途：
- 提取物体的轮廓
- 做文字识别的预处理
- 分离前景和背景

### 核心要点

#### 全局固定阈值（通常用于灰度图）：`cv::threshold`

一般用法：

```C++
double cv::threshold(           // 返回实际使用/计算出的阈值
    InputArray src,             // 输入图，通常为灰度图
    OutputArray dst,            // 输出图
    double thresh,              // 阈值
    double maxval,              // 最大值，通常为 255
    int type                    // 阈值类型
);
```

**Python 对照**（多返回一个阈值 `ret`）：
```python
# cv2.threshold(src, thresh, maxval, type) -> (ret: float, dst: np.ndarray)
#   src: np.ndarray（灰度图）, thresh/maxval: float, type: int
ret, dst = cv2.threshold(src, thresh, maxval, type)   # src 通常为灰度图
```

#### 阈值类型

1. `cv::THRESH_BINARY`：`src(x,y) > thresh` → `dst = maxval`，否则 `dst = 0`。较亮的地方更白，较暗的地方更黑（提取**白色目标**）。
2. `cv::THRESH_BINARY_INV`：反转，`src(x,y) <= thresh` → `dst = maxval`，否则 `dst = 0`。较暗的地方更白，较亮的地方更黑（提取**黑色目标**）。
3. `cv::THRESH_TRUNC`：`src(x,y) > thresh` → `dst = thresh`，否则 `dst = src(x,y)`。把超过阈值的部分截断为阈值。
4. `cv::THRESH_TOZERO`：`src(x,y) > thresh` → `dst = src(x,y)`，否则 `dst = 0`。把小于等于阈值的变为 0。
5. `cv::THRESH_TOZERO_INV`：反转，把大于阈值的变为 0。

#### 自适应阈值：`cv::adaptiveThreshold`

**自定义阈值**：根据每个像素附近的小区域分别计算局部的阈值，更适合**光照不均匀**的图像。

```C++
void cv::adaptiveThreshold(
    InputArray src,         // 输入图，通常为灰度图
    OutputArray dst,        // 输出图
    double maxValue,        // 最大值，通常为 255
    int adaptiveMethod,     // 局部的阈值计算方法（MEAN_C / GAUSSIAN_C）
    int thresholdType,      // THRESH_BINARY / THRESH_BINARY_INV
    int blockSize,          // 邻域大小，必须为奇数
    double C                // 从计算结果中减去的常量
);
```

##### 均值自适应

```C++
cv::Mat adaptiveMean;

cv::adaptiveThreshold(
    gray,
    adaptiveMean,
    255,
    cv::ADAPTIVE_THRESH_MEAN_C, // 自适应均值 - C 作为阈值
    cv::THRESH_BINARY,
    11, // 邻域大小为 11×11
    2   // 统计值减去 2 作为最后的阈值
);
```

**Python 对照**：
```python
adaptive_mean = cv2.adaptiveThreshold(
    gray, 255, cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY, 11, 2)  # -> np.ndarray
```

##### 高斯自适应阈值

```C++
cv::Mat adaptiveGaussian;

cv::adaptiveThreshold(
    gray,
    adaptiveGaussian,
    255,
    cv::ADAPTIVE_THRESH_GAUSSIAN_C, // 高斯自适应阈值 - C 作为阈值
    cv::THRESH_BINARY,
    11,
    2
);
```

**Python 对照**：
```python
adaptive_gaussian = cv2.adaptiveThreshold(
    gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 11, 2)  # -> np.ndarray
```

**均值法 vs 高斯法**：
- 均值法：邻域内像素的权重相对平均
- 高斯法：中心附近的像素权重更高

---

## 二、Otsu 自动阈值

### 概念

Otsu 方法会根据图像的**直方图**自动寻找一个较为合适的全局阈值，尤其适合前景和背景具有较为明显的**双峰分布**的图像。

```C++
cv::Mat otsu;

double otsuThreshold = cv::threshold(
    gray,
    otsu,
    0,     // 一般设置为 0，采用 Otsu 自动计算的阈值
    255,
    cv::THRESH_BINARY | cv::THRESH_OTSU // 组合 Otsu 自动计算阈值和普通二值化
);

std::cout << "Otsu 自动计算的阈值："
          << otsuThreshold
          << std::endl;
```

**Python 对照**（Otsu 会返回自动计算的阈值）：
```python
otsu_threshold, otsu = cv2.threshold(
    gray, 0, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)  # -> (float, np.ndarray)
print("Otsu 自动计算的阈值：", otsu_threshold)
```

### 高斯滤波预处理

如果图像的噪声比较多，可以先对图像进行**高斯滤波**以提高二值化的准确性，避免噪声被误判为前景或背景。

```C++
// 高斯滤波
cv::Mat blurred;
cv::GaussianBlur(
    gray,
    blurred,
    cv::Size(5, 5),
    0
);

// 再进行 Otsu
cv::Mat otsuAfterBlur;

double otsuThresholdAfterBlur = cv::threshold(
    blurred,
    otsuAfterBlur,
    0,
    255,
    cv::THRESH_BINARY | cv::THRESH_OTSU
);
```

**Python 对照**：
```python
blurred = cv2.GaussianBlur(gray, (5, 5), 0)  # -> np.ndarray
otsu_threshold, otsu_after_blur = cv2.threshold(
    blurred, 0, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)  # -> (float, np.ndarray)
```

---

## 三、代码示例

```cpp
// C++ 示例
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>

int main()
{
    const std::string imagePath =
        R"(D:\OneDrive\图片\Screenshots\屏幕截图 2026-07-19 173438.png)";

    cv::Mat image = cv::imread(
        imagePath,
        cv::IMREAD_COLOR
    );

    if (image.empty()) {
        std::cerr << "读取图片失败："
                  << imagePath
                  << std::endl;

        return 1;
    }

    // --------------------------------------------------
    // 1. 转换为灰度图
    // --------------------------------------------------
    cv::Mat gray;

    cv::cvtColor(
        image,
        gray,
        cv::COLOR_BGR2GRAY
    );

    // --------------------------------------------------
    // 2. 固定阈值：普通二值化
    // --------------------------------------------------
    cv::Mat binary;

    double fixedThreshold = cv::threshold(
        gray,
        binary,
        127,
        255,
        cv::THRESH_BINARY
    );

    // --------------------------------------------------
    // 3. 固定阈值：反二值化
    // --------------------------------------------------
    cv::Mat binaryInv;

    cv::threshold(
        gray,
        binaryInv,
        127,
        255,
        cv::THRESH_BINARY_INV
    );

    // --------------------------------------------------
    // 4. 截断阈值
    // --------------------------------------------------
    cv::Mat trunc;

    cv::threshold(
        gray,
        trunc,
        127,
        255,
        cv::THRESH_TRUNC
    );

    // --------------------------------------------------
    // 5. TOZERO
    // --------------------------------------------------
    cv::Mat toZero;

    cv::threshold(
        gray,
        toZero,
        127,
        255,
        cv::THRESH_TOZERO
    );

    // --------------------------------------------------
    // 6. 自适应均值阈值
    // --------------------------------------------------
    cv::Mat adaptiveMean;

    cv::adaptiveThreshold(
        gray,
        adaptiveMean,
        255,
        cv::ADAPTIVE_THRESH_MEAN_C,
        cv::THRESH_BINARY,
        11,
        2
    );

    // --------------------------------------------------
    // 7. 自适应高斯阈值
    // --------------------------------------------------
    cv::Mat adaptiveGaussian;

    cv::adaptiveThreshold(
        gray,
        adaptiveGaussian,
        255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY,
        11,
        2
    );

    // --------------------------------------------------
    // 8. Otsu 自动阈值
    // --------------------------------------------------
    cv::Mat otsu;

    double otsuThreshold = cv::threshold(
        gray,
        otsu,
        0,
        255,
        cv::THRESH_BINARY | cv::THRESH_OTSU
    );

    // --------------------------------------------------
    // 9. 高斯滤波后再使用 Otsu
    // --------------------------------------------------
    cv::Mat blurred;

    cv::GaussianBlur(
        gray,
        blurred,
        cv::Size(5, 5),
        0
    );

    cv::Mat otsuAfterBlur;

    double otsuThresholdAfterBlur = cv::threshold(
        blurred,
        otsuAfterBlur,
        0,
        255,
        cv::THRESH_BINARY | cv::THRESH_OTSU
    );

    // --------------------------------------------------
    // 输出阈值信息
    // --------------------------------------------------
    std::cout << "固定阈值："
              << fixedThreshold
              << std::endl;

    std::cout << "Otsu 阈值："
              << otsuThreshold
              << std::endl;

    std::cout << "滤波后 Otsu 阈值："
              << otsuThresholdAfterBlur
              << std::endl;

    // --------------------------------------------------
    // 显示图像
    // --------------------------------------------------
    cv::imshow("原图", image);
    cv::imshow("灰度图", gray);
    cv::imshow("固定阈值二值化", binary);
    cv::imshow("反二值化", binaryInv);
    cv::imshow("截断阈值", trunc);
    cv::imshow("TOZERO", toZero);
    cv::imshow("自适应均值阈值", adaptiveMean);
    cv::imshow("自适应高斯阈值", adaptiveGaussian);
    cv::imshow("Otsu", otsu);
    cv::imshow("高斯滤波后 Otsu", otsuAfterBlur);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}
```

```python
# Python 示例
import cv2

image_path = r"D:\OneDrive\图片\Screenshots\屏幕截图 2026-07-19 173438.png"

image = cv2.imread(image_path)  # -> np.ndarray / None

if image is None:
    print("读取图片失败：", image_path)
    exit()

# 1. 转灰度图
gray = cv2.cvtColor(
    image,
    cv2.COLOR_BGR2GRAY
)  # -> np.ndarray

# 2. 固定阈值
fixed_threshold, binary = cv2.threshold(
    gray,
    127,
    255,
    cv2.THRESH_BINARY
)  # -> (float, np.ndarray)

# 3. 反二值化
_, binary_inv = cv2.threshold(
    gray,
    127,
    255,
    cv2.THRESH_BINARY_INV
)  # -> (float, np.ndarray)

# 4. 截断阈值
_, trunc = cv2.threshold(
    gray,
    127,
    255,
    cv2.THRESH_TRUNC
)  # -> (float, np.ndarray)

# 5. TOZERO
_, to_zero = cv2.threshold(
    gray,
    127,
    255,
    cv2.THRESH_TOZERO
)  # -> (float, np.ndarray)

# 6. 自适应均值阈值
adaptive_mean = cv2.adaptiveThreshold(
    gray,
    255,
    cv2.ADAPTIVE_THRESH_MEAN_C,
    cv2.THRESH_BINARY,
    11,
    2
)  # -> np.ndarray

# 7. 自适应高斯阈值
adaptive_gaussian = cv2.adaptiveThreshold(
    gray,
    255,
    cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
    cv2.THRESH_BINARY,
    11,
    2
)  # -> np.ndarray

# 8. Otsu 自动阈值
otsu_threshold, otsu = cv2.threshold(
    gray,
    0,
    255,
    cv2.THRESH_BINARY | cv2.THRESH_OTSU
)  # -> (float, np.ndarray)

# 9. 高斯滤波后使用 Otsu
blurred = cv2.GaussianBlur(
    gray,
    (5, 5),
    0
)  # -> np.ndarray

otsu_blur_threshold, otsu_after_blur = cv2.threshold(
    blurred,
    0,
    255,
    cv2.THRESH_BINARY | cv2.THRESH_OTSU
)  # -> (float, np.ndarray)

print("固定阈值：", fixed_threshold)
print("Otsu 阈值：", otsu_threshold)
print("滤波后 Otsu 阈值：", otsu_blur_threshold)

cv2.imshow("原图", image)
cv2.imshow("灰度图", gray)
cv2.imshow("固定阈值二值化", binary)
cv2.imshow("反二值化", binary_inv)
cv2.imshow("截断阈值", trunc)
cv2.imshow("TOZERO", to_zero)
cv2.imshow("自适应均值阈值", adaptive_mean)
cv2.imshow("自适应高斯阈值", adaptive_gaussian)
cv2.imshow("Otsu", otsu)
cv2.imshow("高斯滤波后 Otsu", otsu_after_blur)

cv2.waitKey(0)
cv2.destroyAllWindows()
```

### 二值图像的本质

二值图像通常为 `CV_8UC1`（单通道 8 位无符号整数，0~255），**0 表示黑色，255 表示白色**。

后续的轮廓提取需要基于二值化后的二值图（因为具有清晰的前景和背景分离）来处理，典型的处理流程：

```
读取图像
  ↓
转灰度
  ↓
滤波去噪
  ↓
阈值二值化
  ↓
查找轮廓
  ↓
计算面积、矩形、圆形
```

---

## 四、本节小结

| 图像情况 | 推荐方法 |
|---|---|
| 光照均匀、背景简单 | 固定阈值 |
| 不知道合适阈值 | Otsu |
| 图像有噪声 | 高斯滤波 + Otsu |
| 光照不均匀 | 自适应阈值 |
| 黑色目标、白色背景 | `THRESH_BINARY_INV` |
| 白色目标、黑色背景 | `THRESH_BINARY` |

三种重要的方法：

```
固定阈值    ：适合光照均匀的图像
自适应阈值  ：适合光照不均匀的图像
Otsu        ：适合不知道阈值、且前景背景分布较明显的图像
```

重点 API：

```C++
cv::threshold()       // 全局固定阈值处理
cv::adaptiveThreshold() // 自适应阈值处理
cv::THRESH_BINARY     // 一般二值化，白更白、黑更黑，用于提取白色目标
cv::THRESH_BINARY_INV // 反二值化，白变黑、黑变白，用于提取黑色目标
cv::THRESH_OTSU       // 自动计算阈值，通常组合前两者
```

---

## 相关链接

- 上一课：[[OpenCV第三课 图像几何操作]]
- 下一课：[[OpenCV第五课 图像平滑与滤波]]
- 主题归纳：[[OpenCV学习笔记/主题模块/图像处理]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
