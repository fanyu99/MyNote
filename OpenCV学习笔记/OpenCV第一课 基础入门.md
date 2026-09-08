---
title: OpenCV 第一课
type: 课程笔记
课程: 1
created: 2026-09-01
updated: 2026-09-02
tags:
  - opencv
  - 课程笔记
  - BGR
  - cv_Mat
status: 学习中
---

# OpenCV 第一课

> **本课目标**：正确认识图像的本质、掌握 OpenCV 的 BGR 颜色顺序、理解核心类 `cv::Mat`、认识常用数据类型，并跑通"读取→获取信息→显示→保存"的完整流程。
> **涉及主题**：[[OpenCV学习笔记/主题模块/图像基础与色彩空间]] · [[OpenCV学习笔记/主题模块/cv_Mat核心]] · [[OpenCV学习笔记/主题模块/OpenCV与Qt集成]]

---

## 一、图像的本质与颜色顺序

### 1. 图像的本质

一张图片本质上就是**一块内存**：

- 用 **宽度 × 高度** 表示图像的大小；
- 其中的数字表示**像素值**（灰度图像存的是灰度值，彩色图像存的是各通道的亮度值）。

### 2. OpenCV 的颜色顺序：BGR

OpenCV 内部使用 **BGR**（蓝、绿、红）顺序，与常见的 **RGB**（红、绿、蓝）不同。

> 例如：纯红色的一般表示为 `(255, 0, 0)`，但在 OpenCV 中要写成 `cv::Scalar(0, 0, 255)`。

---

## 二、核心类：cv::Mat

### 什么是 cv::Mat

`cv::Mat` 表示一个**矩阵**，可以承载：

- 一张图片、一帧视频
- 灰度数据、彩色图像、二值图像、深度图
- 算法中间结果

它比一般的二维数组更强大，额外管理了：

| 管理项 | 说明 |
| --- | --- |
| 数据类型 | 元素类型（如 `uchar`、`float`） |
| 通道数量 | 1（灰度）、3（BGR）、4（BGRA）等 |
| 行数 / 列数 | 图像尺寸 |
| 内存空间 | 数据的存储分配 |
| 引用计数 | 自动管理内存释放 |
| 数据共享 | 多个 Mat 可共享同一块数据 |
| ROI 区域 | 感兴趣区域（Region of Interest） |

---

## 三、OpenCV 的数据类型

### 常用类型：CV_8UCn

经常能看到类似 `CV_8UC1`、`CV_8UC3` 这样的类型，格式为 `CV_<位数><类型><通道数>`：

- `CV_8U` 表示 **8 位无符号整数**，取值范围 **0 ~ 255**，是常见的图像数据类型。

| 类型 | 含义 | 对应关系 |
| --- | --- | --- |
| `CV_8UC1` | 8 位无符号整数，1 通道 | 灰度图 |
| `CV_8UC3` | 8 位无符号整数，3 通道 | BGR 彩图 |
| `CV_8UC4` | 8 位无符号整数，4 通道 | BGRA 彩图 |
| `CV_32FC1` | 32 位浮点数，1 通道 | — |

---

## 四、完整代码示例

### 一个常见的工作流程：读取 → 获取信息 → 显示 → 保存

#### C++ 版本

```cpp
#include <iostream>
#include <opencv2/opencv.hpp>

int main()
{
    // 读取图片（请将路径替换为你自己的图片）
    cv::Mat image = cv::imread("test.jpg");
    if (image.empty()) {
        std::cerr << "读取图片失败" << std::endl;
        return -1;
    }

    std::cout << "图片读取成功" << std::endl;
    std::cout << "宽度: " << image.cols << std::endl;
    std::cout << "高度: " << image.rows << std::endl;
    std::cout << "通道数: " << image.channels() << std::endl;

    cv::imshow("原始图片", image);   // 创建窗口并提交显示请求

    cv::imwrite("output.jpg", image); // 保存图片

    cv::waitKey(0);                   // 无限等待键盘输入
    return 0;
}
```

#### Python 版本

```python
import cv2

image = cv2.imread("test.jpg")   # 请替换为你自己的图片路径   -> np.ndarray / None

if image is None:
    print("读取图片失败")
    exit()

print("图片读取成功")
print("宽度:", image.shape[1])
print("高度:", image.shape[0])
print("通道数:", image.shape[2])
print(image[0, 0])   # 获取 Numpy 数组（返回像素值，灰度: np.uint8 标量 / 彩色: 长度 3 的数组）
cv2.imshow("原始图片", image)   # -> None

cv2.waitKey(0)                  # -> int
cv2.destroyAllWindows()         # -> None
```

> **注意**：Python 中获取宽高的顺序与 C++ 不同：
> - `image.shape[0]` → 高度
> - `image.shape[1]` → 宽度
> - `image.shape[2]` → 通道

### 用 Numpy 数组获取像素（Python）

注意是 `image[y, x]` 而不是 `image[x, y]`，即 **`image[行坐标, 列坐标]`**。

---

## 五、常用函数速查

### cv::imread —— 读取图片

```cpp
// 以彩色方式读取（默认）：cv::IMREAD_COLOR（可省略）
// 以灰度图读取：cv::IMREAD_GRAYSCALE
// 保留原始通道：cv::IMREAD_UNCHANGED
cv::Mat image = cv::imread("test.jpg", cv::IMREAD_COLOR);
```

**Python 对照**：
```python
image = cv2.imread("test.jpg", cv2.IMREAD_COLOR)      # 彩色（默认）   -> np.ndarray / None
gray  = cv2.imread("test.jpg", cv2.IMREAD_GRAYSCALE)  # 灰度           -> np.ndarray / None
raw   = cv2.imread("test.jpg", cv2.IMREAD_UNCHANGED)  # 保留原通道     -> np.ndarray / None
```

### cv::imshow / cv::waitKey —— 显示图片

```cpp
void cv::imshow(const std::string& winname, cv::InputArray mat); // 创建窗口并提交显示请求，不一定一直等待用户操作
int  cv::waitKey(int delay = 0);                                 // 无限等待键盘输入，返回值为用户输入的键值
```

**Python 对照**：
```python
cv2.imshow("原始图片", image)   # -> None
cv2.waitKey(0)                  # -> int：用户按下的键值
cv2.destroyAllWindows()         # -> None
```

### 访问像素

#### 灰度读取

```cpp
cv::Mat image = cv::imread("test.jpg", cv::IMREAD_GRAYSCALE); // 灰度读取
if (image.empty()) {
    std::cerr << "读取图片失败" << std::endl;
    return -1;
}

// image.at<uchar>(y, x) 访问第 y 行第 x 列的像素
uchar value = image.at<uchar>(0, 0); // 获取第 0 行 0 列的像素
std::cout << "左上角灰度值: " << static_cast<int>(value) << "\n";
```

**Python 对照**（numpy 下标，顺序同为 `[行, 列]`）：
```python
value = image[0, 0]     # 第 0 行 0 列像素（numpy 标量）
print("左上角灰度值:", value)
```

#### 三通道彩色读取

```cpp
cv::Mat image = cv::imread("test.jpg");

cv::Vec3b pixel = image.at<cv::Vec3b>(0, 0); // Vec3b 由 3 个 uchar 组成的像素

std::cout << "B = " << static_cast<int>(pixel[0]) << std::endl;
std::cout << "G = " << static_cast<int>(pixel[1]) << std::endl;
std::cout << "R = " << static_cast<int>(pixel[2]) << std::endl;
```

**Python 对照**（返回长度为 3 的数组，顺序为 BGR）：
```python
b, g, r = image[0, 0]
print("B =", b)
print("G =", g)
print("R =", r)
```

---

## 六、OpenCV 与 Qt 集成

### 最重要的问题之一：红蓝互换

OpenCV 读出的普通彩色图片通常是 **BGR**，而 Qt 常用格式是 **RGB**。直接把 OpenCV 的数据交给 Qt 会发生**红蓝互换**的错误！

### 转换方法

```cpp
// OpenCV 通常使用转换：
void cv::cvtColor(
    InputArray src,          // 输入图
    OutputArray dst,         // 输出图（RGB）
    int code,                // 转换类型，如 COLOR_BGR2RGB
    int dstCn = 0            // 目标通道数，0 表示自动
);
cv::Mat rgbImage;
cv::cvtColor(image, rgbImage, cv::COLOR_BGR2RGB);

// Qt 显示通常使用：
QImage qImage(
    rgbImage.data,                     // const uchar*
    rgbImage.cols,                     // int
    rgbImage.rows,                     // int
    static_cast<int>(rgbImage.step),   // int（行步长）
    QImage::Format_RGB888              // Format
);
```

**Python 对照**（Qt 显示部分为 C++/Qt 专用；Python 通常用 `matplotlib` 或 OpenCV 直接显示）：
```python
rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)   # BGR -> RGB   -> np.ndarray
# matplotlib 显示 RGB：
# import matplotlib.pyplot as plt
# plt.imshow(rgb_image); plt.show()
```

### 知识重点

| 场景                    | 颜色顺序     |
| --------------------- | -------- |
| OpenCV 默认             | **BGR**  |
| Qt 常用显示格式             | **RGB**  |
| **函数**                | **作用**   |
| `cv::imread()`        | 读取图片     |
| `cv::imshow()`        | 显示图片     |
| `cv::imwrite()`       | 保存图片     |
| `cv::waitKey()`       | 等待键盘输入   |
| `cv::cvtColor()`      | 颜色空间转换   |
| `cv::Mat::empty()`    | 判断图像是否为空 |
| `cv::Mat::channels()` | 获取通道数    |
| `cv::Mat::rows`       | 获取高度     |
| `cv::Mat::cols`       | 获取宽度     |
| `cv::Mat::at()`       | 访问像素     |
两者结合时，通常需要 `cv::cvtColor()` 进行转换。

---

## 七、本节小结

- 图像本质是内存，大小为 **宽度 × 高度**，值为像素。
- OpenCV 使用 **BGR** 颜色顺序。
- `cv::Mat` 是 OpenCV 的核心类，管理类型/通道/尺寸/内存/引用计数/ROI。
- 常用数据类型 `CV_8UCn`，`CV_8U` 取值范围 **0 ~ 255**。
- 掌握 `imread / imshow / imwrite / waitKey / at<>` 的用法。
- 与 Qt 集成时需用 `cv::cvtColor` 做 BGR → RGB 转换。

---

## 相关链接

- **课程系列**：本课为第 1 课（后续：图像处理、特征检测…）
- 下一课: [[OpenCV第二课 cv_Mat模型]]
- **主题模块**：
  - [[OpenCV学习笔记/主题模块/图像基础与色彩空间]]
  - [[OpenCV学习笔记/主题模块/cv_Mat核心]]
  - [[OpenCV学习笔记/主题模块/OpenCV与Qt集成]]
- **知识库总览**：[[OpenCV学习笔记/_MOC]]
