---
title: cv::Mat内存模型
type: 课程笔记
课程: 2
created: 2026-09-01
updated: 2026-09-02
tags:
  - opencv
  - 课程笔记
  - cv_Mat
status: 学习中
---

# cv::Mat内存模型（课程笔记）

> **本课目标**：深入理解 `cv::Mat` 的内存模型（矩阵头 + 数据 + 引用计数），掌握图像尺寸/通道/类型、浅拷贝与深拷贝的区别、ROI、内存连续性以及三种像素访问方式。
> **涉及主题**：[[OpenCV学习笔记/主题模块/cv_Mat核心]] · [[OpenCV学习笔记/主题模块/OpenCV与Qt集成]]

---

## 一、cv::Mat 的内存模型

### 概念

`cv::Mat` 本质上是**「矩阵头 + 像素数据内存」**的封装。矩阵头记录了图像的元信息，数据指针指向真正存像素的内存区。

### 核心要点

```
cv::Mat 对象
│
├─ rows：行数 / 图像高度
├─ cols：列数 / 图像宽度
├─ type：像素的数据类型与通道数
├─ step：每一行占用多少字节
├─ data：指向像素数据区的指针
└─ 引用计数：有多少个 Mat 对象共享同一份数据
```

---

## 二、图像的尺寸、通道与类型

### 概念

- `image.rows` → 行数（高度方向）；`image.cols` → 列数（宽度方向）。
- 访问像素用 `image.at<类型>(y, x)`，注意顺序是 **`(y, x) = (行, 列) = (高度方向, 宽度方向)`**。

#### channels() —— 通道数

| 图像格式 | 通道数 | 常见用途 |
| --- | --- | --- |
| 灰度图 | 1 | 二值化、边缘检测、传统视觉算法 |
| BGR 彩色图 | 3 | OpenCV 默认彩色图 |
| BGRA 彩色图 | 4 | 包含透明度的 PNG 等图片 |

#### type() —— 返回 OpenCV 宏的整数二进制位数值

| OpenCV 宏 | 常见 `type()` 输出 |
| --- | --- |
| `CV_8UC1` | `0` |
| `CV_8UC2` | `8` |
| `CV_8UC3` | `16` |
| `CV_8UC4` | `24` |

### 核心要点

#### 图像数据类型：CV_8UCn 到底是什么

以 `CV_8UC3` 为例：

- `CV_8U` → 每个通道是 **8 位无符号整数**，范围 `0 ~ 255`。
- `C3` → **3 个通道** `[B][G][R]`，一个通道一个字节；对应方法 `elemSize1()`，返回**单个通道**的大小。
- 一个 `CV_8UC3` 占 `3 × 1 = 3` 字节；对应方法 `elemSize()`，返回**整个元素**（所有通道）的大小。

#### 常见 OpenCV 类型表

| 类型 | 含义 | 每通道范围 / 说明 |
| --- | --- | --- |
| `CV_8UC1` | 8 位无符号，单通道 | `0 ~ 255` |
| `CV_8UC3` | 8 位无符号，3 通道 | 常见 BGR 彩图 |
| `CV_8UC4` | 8 位无符号，4 通道 | 常见 BGRA 图片 |
| `CV_16UC1` | 16 位无符号，单通道 | 高精度灰度、工业相机数据 |
| `CV_32FC1` | 32 位浮点，单通道 | 算法计算中间结果 |
| `CV_64FC1` | 64 位浮点，单通道 | 高精度矩阵计算 |

---

## 三、拷贝与引用计数（浅拷贝 vs 深拷贝）

### 概念

理解 `cv::Mat` 的**引用计数**机制，以及**浅拷贝 / 深拷贝**的区别，是正确使用 Mat 的关键。

### 核心要点

#### 1. 浅拷贝的副作用

```cpp
cv::Mat image = cv::imread("test.jpg");

cv::Mat copyImage = image;
```

> **浅拷贝**后，`copyImage` 与 `image` 的 `data` 指针指向**同一块内存**，因此只要修改其中一个的数据，另一个的数据也会被修改！

```cpp
// 修改 copyImage 的左上角 100 × 100 区域为红色。
cv::rectangle(
    copyImage,
    cv::Rect(0, 0, 100, 100),   // 左上角坐标为 (0,0) 的 100*100 矩形
    cv::Scalar(0, 0, 255),      // 红色
    cv::FILLED                  // 填充
);

cv::imshow("image", image);
cv::imshow("copyImage", copyImage);
// 这时候会发现 image 和 copyImage 的图片左上角都为红色！！！
```

#### 2. 深拷贝：clone() 与 copyTo()

**使用 `clone()` 进行深拷贝**

```cpp
cv::Mat deepCopy = image.clone(); // 深拷贝
```

此时两个对象的 `data` 指向**不同的内存**：

```
image    ──> 图像数据 A
deepCopy ──> 图像数据 B
```

**使用 `copyTo()`（可带掩膜复制）**

```cpp
cv::Mat output;
image.copyTo(output);            // 相当于 output = input.clone()

image.copyTo(result, mask);      // 只复制 mask 中满足条件的区域
```

#### 3. cv::Mat 的引用计数

```cpp
cv::Mat image = cv::imread("test.jpg");
cv::Mat image2 = image;
cv::Mat image3 = image;
```

此时三个 `image` 共用一块图像数据内存；当**最后一个引用对象销毁**时，会自动释放图像数据内存。因此**不需要手动 delete 图像数据内存**！

#### 4. 拷贝行为对照表

| 操作 | 是否复制像素数据 | 是否共享底层内存 |
| --- | --- | --- |
| `cv::Mat b = a;` | 否 | 是 |
| `cv::Mat b(a);` | 否 | 是 |
| `cv::Mat b = a.clone();` | 是 | 否 |
| `a.copyTo(b);` | 是 | 否 |
| `cv::Mat roi = a(rect);` | 通常否 | 是 |
| `cv::Mat roi = a(rect).clone();` | 是 | 否 |

---

## 四、ROI 与内存连续性

### 概念

ROI（Region of Interest，感兴趣区域）用于只关注图像中的某一块区域；`data`、`step`、`isContinuous()` 与图像在内存中的存储方式有关。

### 核心要点

#### 图像 ROI

```cpp
cv::Mat image = cv::imread("test.jpg");

cv::Rect rect(100, 50, 300, 200); // 定义一个矩形 (x, y, w, h)

cv::Mat roi = image(rect);        // 获取 image 的指定矩形区域
```

> ⚠️ **注意**：ROI 同样是**浅拷贝**！如果要独立保存，请使用 `copyTo()` 或 `clone()`。

#### data

- `data` 是一个指针，指向**图像数据第一行第一个像素**的内存。
- 不要直接大范围使用 `data[index]`，因为图像**不一定是连续的**（尤其是 ROI）。

#### step

- `step` 表示从一行图像数据跳到下一行，需要**跨过多少字节**。
- 通常等于 `image.cols * image.elemSize()`。
- 但有些图像为了**内存对齐**进行填充字节，会使 `step >= cols * elemSize()`。

#### isContinuous()

用于判断图像数据是否在内存中**连续存储**。

---

## 五、三种访问像素的方法

### 概念

`cv::Mat` 提供三种像素访问方式：`at<>`、`ptr<>`、直接访问 `data`。三者性能与适用场景不同。

### 核心要点

#### 1. 使用 `at<>` 访问（性能不如 ptr）

```cpp
uchar value = gray.at<uchar>(y, x); // 灰度图

// 彩色图
cv::Vec3b pixel = color.at<cv::Vec3b>(y, x);
uchar b = pixel[0];
uchar g = pixel[1];
uchar r = pixel[2];
```

#### 2. 使用 `ptr<>`（推荐用于批量处理）

```cpp
// 灰度图
for (int y = 0; y < gray.rows; ++y)
{
    uchar* row = gray.ptr<uchar>(y); // 获取行首指针

    for (int x = 0; x < gray.cols; ++x)
    {
        row[x] = 255 - row[x]; // 反色处理
    }
}
```

```cpp
// BGR 图像
for (int y = 0; y < image.rows; ++y)
{
    cv::Vec3b* row = image.ptr<cv::Vec3b>(y);

    for (int x = 0; x < image.cols; ++x)
    {
        row[x][0] = 255 - row[x][0]; // B
        row[x][1] = 255 - row[x][1]; // G
        row[x][2] = 255 - row[x][2]; // R
    }
}
```

#### 3. 直接访问 `data`（仅当内存连续时才能使用）

```cpp
// 灰度图
if (gray.isContinuous())
{
    const int total = gray.rows * gray.cols;

    for (int i = 0; i < total; ++i)
    {
        gray.data[i] = 255 - gray.data[i];
    }
}
```

---

## 六、Qt + OpenCV 中浅拷贝的危险

### 概念

如果直接把 `cv::Mat` 的原始数据交给 `QImage`，`QImage` 可能只是在**引用** Mat 的数据，并未真正复制像素，存在花屏/崩溃等风险。

### 核心要点

```cpp
cv::Mat frame;
camera.read(frame);

QImage qImage(
    frame.data,
    frame.cols,
    frame.rows,
    static_cast<int>(frame.step),
    QImage::Format_RGB888
);
```

此时 `qImage` 只是在引用 `frame` 的 `data`，**并不一定复制了像素**。如果随后 `frame` 很快被下一帧覆盖/释放/离开作用域，很可能出现**花屏、闪烁、崩溃、显示上一帧或错误帧**。

因此后面需要明确判断：

1. 当前的 `QImage` 是否有独立的数据；
2. `cv::Mat` 在 Qt 使用期间是否仍有效；
3. 图像是否会跨线程传递；
4. 图像是否正确地处于 BGR / RGB 格式；
5. 是否需要深拷贝。

---

## 七、代码示例

### C++ 示例：深拷贝与 ROI

```cpp
#include <opencv2/opencv.hpp>

int main()
{
    cv::Mat image = cv::imread("test.jpg");

    cv::Mat shallow = image;            // 浅拷贝：共享内存
    cv::Mat deep    = image.clone();    // 深拷贝：独立内存

    cv::Rect rect(100, 50, 300, 200);
    cv::Mat roi = image(rect);          // ROI：默认浅拷贝
    cv::Mat roiCopy = image(rect).clone(); // 独立保存

    // 像素访问（灰度图）
    cv::Mat gray = cv::imread("test.jpg", cv::IMREAD_GRAYSCALE);
    for (int y = 0; y < gray.rows; ++y)
    {
        uchar* row = gray.ptr<uchar>(y);
        for (int x = 0; x < gray.cols; ++x)
            row[x] = 255 - row[x];      // 反色
    }

    cv::imshow("image", image);
    cv::waitKey(0);
    return 0;
}
```

> 说明：本课内容偏 C++ 内存语义；Python 中 `numpy` 数组的切片赋值同样属于**共享内存**（浅拷贝），如需独立数据应使用 `.copy()`。

---

## 八、本节小结

- `cv::Mat` 本质上是「矩阵头 + 像素数据」的对象。
- `Mat a = b;` 通常是**浅拷贝**，两个对象共享一块图像内存。
- `Mat a = b.clone();` 或 `b.copyTo(a);` 才会创建**独立的像素数据**。
- `Mat` 使用**引用计数**自动管理内存释放，无需手动 `delete`。
- ROI 默认共享原始图像的数据，且 ROI 通常**不是连续存储**的。
- 图像坐标访问顺序是 `(y, x) = (行, 列) = (高度方向, 宽度方向)`。
- 像素访问三方式：`at<>`（灵活）、`ptr<>`（批量推荐）、`data`（需 `isContinuous()`）。
- 与 Qt 集成时，`QImage` 可能只是引用 Mat 的 `data`，需注意生命周期与深拷贝。

---

## 相关链接

- 上一课：[[OpenCV第一课]]
- 下一课：（待添加）
- 主题归纳：[[OpenCV学习笔记/主题模块/cv_Mat核心]] · [[OpenCV学习笔记/主题模块/OpenCV与Qt集成]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
