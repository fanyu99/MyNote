---
title: OpenCV 第八课 Canny边缘检测
type: 课程笔记
课程: 8
created: 2026-09-06
updated: 2026-09-06
tags:
  - opencv
  - 课程笔记
  - 边缘检测
  - canny
status: 学习中
---

# OpenCV第八课 Canny边缘检测

> **本课目标**：掌握图像边缘与梯度的概念，学会使用 Sobel / Scharr / Laplacian / Canny 四种算子进行边缘检测，理解 Canny 的多阶段流程与**双阈值**原理。
> **涉及主题**：[[OpenCV学习笔记/主题模块/特征检测与匹配]]

---

## 一、什么是图像边缘

### 概念

图像边缘就是像素灰度变化较快的位置
例如:
```
黑色和白色的交界处
物体和背景的交界处
文字的轮廓
零件的边界
人脸五官的边界
```
### 图像梯度

梯度强度可以近似表示为
```
G = sqrt(Gx^2 + Gy^2)   // L2 范数
或 G = |Gx| + |Gy|      // L1 范数（速度更快）
```

### Sobel算子(一阶)
![[Pasted image 20260907152116.png]]
#### 作用
```C++
cv::Sobel(
	src,
	dst,
	ddepth, // 输出数据类型
	dx, // x方向导数阶数
	dy, 
	ksize // 卷积核大小
);
cv::Sobel(
	gray,
	sobelX,
	CV_16S, // 输出类型为CV_16S
	1, // x一阶
	0,
	3 
);
```

#### 方向梯度阶数检测什么?
例如dx=1,dy=0表示计算X方向上的变化;
注意: **如果X方向的变化较强,通常意味着存在垂直边缘**
然后dy=0,dy=1表示计算Y方向上的变化;
注意: **如果Y方向上的变化较强,通常意味着存在水平边缘**
例如:
```
// X方向
黑黑黑 | 白白白

// Y方向
黑黑黑
-----
白白白
```

### 合并边缘
```C++
cv::AddWeighted(
	sobelX,
	fx,
	sobelY,
	fy,
	0, // 添加到结果的变量值用于亮度调节
	sobelCombined // 可选: 输出图像
	dtype // 可选: 输出图像深度
);
```

### 为什么使用CV_165S?
灰度图类型一般为: CV_8U
但是梯度的计算结果可能会为负数
所以通常先使用CV_16S保存负数后再转换为8位图
```C++
cv::convertScaleAbs(sobelX16,sobelX); // 从16位转为8位
```

### Scharr 算子(一阶)
![[Pasted image 20260907152239.png]]
专为3 * 3 卷积核设计,相比Sobel,计算更加精准
一般如果需要更加精准的3 * 3一阶导数时使用Scharr
```C++
cv::Scharr(
	src,
	dst,
	ddepth,
	dx,
	dy,
);
```

### Laplacian 算子(二阶)
![[Pasted image 20260907152320.png]]
```C++
cv::Laplacian(
	InputArray src,
	outputArray dst,
	int ddepth, // 输出图像深度:CV_16S.CV_64F等
	int ksize = 1, // 卷积核大小
	double scale = 1, // 缩放因子
	double delta = 0, // 偏移量
	int boderType = BORDER_DEFAULT 
);
```

#### Laplacian的特点
1. 可以检测各个方向的边缘
2. 不需要计算x和y
3. 对噪声比较敏感
4. 使用前需要先高斯滤波

### Canny边缘检测
#### Canny是什么?
Canny是一种边缘检测算法,
通常包含一下步骤:
1. 高斯滤波去噪
2. 计算图像梯度
3. 非极大值抑制,使边缘变细
4. 双阈值检测
5. 滞后边缘连接
```C++
cv::Canny(
	inputArray image,
	outputArray edges,
	double threshold1, // 低阈值
	double threshold2, // 高阈值
	int apertureSize = 3 , // Sobel算子的卷积核大小
	bool L2Gradiant = false // 是否使用L2范数计算梯度幅值(更加精确但是速度较慢)
	
);
// 选择L2范数时会使用G = sqrt(Gx^2 + Gy^2);不使用时则G = |Gx| + |Gy|
```

#### 非极大值抑制
线性插值法:
![[Pasted image 20260907154345.png]]
一般简化为八方向模型,不使用线性插值法

#### 高低阈值
大于高阈值:认为是强边缘
小于低阈值:认为不是边缘
两者之间:如果与强边缘连接,则保留,否则舍弃
高阈值较大时,边界标准越高,所得图像的边缘信息更少
高阈值较小时,边界标准越低,候选的边界越多,能得到的边缘信息更多但是可能会选取噪声作为边界


### 三个算子区别
|算子|导数类型|主要特点|常见问题|
|---|---|---|---|
|Sobel|一阶导数|可以分别检测 x、y 方向|结果可能较粗|
|Scharr|一阶导数|`3×3` 情况下通常更精确|不能像 Sobel 一样自由选择核大小|
|Laplacian|二阶导数|同时检测多个方向|对噪声敏感|
|Canny|多阶段边缘检测|边缘细、效果稳定|需要调整两个阈值|
### 核心要点

---

## 二、边缘检测与图像处理流程的关系

### 概念

Canny 等边缘检测**很少单独使用**，通常是图像处理流程中的**中间步骤**，为后续的**轮廓检测、特征提取**做准备：

```
读取图像 → 灰度化 → 滤波去噪 → 边缘检测 → 轮廓检测 / 特征提取
```

- **高斯滤波（[[OpenCV第五课 图像平滑与滤波]]）**：Canny 内部的第一步就是高斯去噪，也是常见的预处理。
- **轮廓检测（[[OpenCV第七课 轮廓检测与目标分析]]）**：`findContours` 查找轮廓前，常用 Canny 生成**二值边缘图**作为输入（白边、黑底）。
- **特征检测与匹配**：边缘检测结果是角点、特征描述子等特征提取的基础。

### 核心要点

- Canny 输出的是**二值边缘图**（单通道 0/255），可直接作为二值化输入给 `findContours`。
- 高斯滤波做预处理时，效果会明显影响边缘检测的噪声表现。

---

## 三、代码示例

```cpp
// C++ 示例
#include <opencv2/opencv.hpp>
#include <QDebug>

int main()
{
    const std::string imagePath =
        R"(D:\OneDrive\图片\Screenshots\屏幕截图 2026-08-20 100303.png)";

    // 1. 读取图像
    cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);

    if (image.empty()) {
        qDebug() << "图像读取失败";
        qDebug() << "请检查图片路径是否正确";
        return 1;
    }

    // 2. 转换为灰度图
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // 3. 高斯滤波，减少噪声
    cv::Mat blurred;
    cv::GaussianBlur(
        gray,
        blurred,
        cv::Size(5, 5),
        0
    );

    // =========================
    // Sobel
    // =========================

    cv::Mat sobelX16;
    cv::Mat sobelY16;

    // x 方向梯度
    cv::Sobel(
        blurred,
        sobelX16,
        CV_16S,
        1,
        0,
        3
    );

    // y 方向梯度
    cv::Sobel(
        blurred,
        sobelY16,
        CV_16S,
        0,
        1,
        3
    );

    // 转换为可以显示的 8 位图
    cv::Mat sobelX;
    cv::Mat sobelY;

    cv::convertScaleAbs(sobelX16, sobelX);
    cv::convertScaleAbs(sobelY16, sobelY);

    // 合并 x、y 两个方向的梯度
    cv::Mat sobelCombined;

    cv::addWeighted(
        sobelX,
        0.5,
        sobelY,
        0.5,
        0,
        sobelCombined
    );

    // =========================
    // Scharr
    // =========================

    cv::Mat scharrX16;
    cv::Mat scharrY16;

    cv::Scharr(
        blurred,
        scharrX16,
        CV_16S,
        1,
        0
    );

    cv::Scharr(
        blurred,
        scharrY16,
        CV_16S,
        0,
        1
    );

    cv::Mat scharrX;
    cv::Mat scharrY;

    cv::convertScaleAbs(scharrX16, scharrX);
    cv::convertScaleAbs(scharrY16, scharrY);

    cv::Mat scharrCombined;

    cv::addWeighted(
        scharrX,
        0.5,
        scharrY,
        0.5,
        0,
        scharrCombined
    );

    // =========================
    // Laplacian
    // =========================

    cv::Mat laplacian16;
    cv::Mat laplacian;

    cv::Laplacian(
        blurred,
        laplacian16,
        CV_16S,
        3
    );

    cv::convertScaleAbs(
        laplacian16,
        laplacian
    );

    // =========================
    // Canny
    // =========================

    cv::Mat edges;

    cv::Canny(
        blurred,
        edges,
        50,
        150,
        3,
        true
    );

    // 输出图像尺寸和类型信息
    qDebug() << "原图尺寸:"
             << image.cols
             << "x"
             << image.rows;

    qDebug() << "灰度图通道数:"
             << gray.channels();

    qDebug() << "Canny 图像通道数:"
             << edges.channels();

    // 显示结果
    cv::imshow("Original", image);
    cv::imshow("Gray", gray);
    cv::imshow("Blurred", blurred);

    cv::imshow("Sobel X", sobelX);
    cv::imshow("Sobel Y", sobelY);
    cv::imshow("Sobel Combined", sobelCombined);

    cv::imshow("Scharr Combined", scharrCombined);
    cv::imshow("Laplacian", laplacian);
    cv::imshow("Canny Edges", edges);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}
```

```python
# Python 示例
import cv2 as cv

image_path = r"D:\OneDrive\图片\Screenshots\屏幕截图 2026-08-20 100303.png"

image = cv.imread(image_path, cv.IMREAD_COLOR)

if image is None:
    print("图像读取失败")
    exit()

# 灰度图
gray = cv.cvtColor(image, cv.COLOR_BGR2GRAY)

# 高斯滤波
blurred = cv.GaussianBlur(
    gray,
    (5, 5),
    0
)

# Sobel X
sobel_x16 = cv.Sobel(
    blurred,
    cv.CV_16S,
    1,
    0,
    ksize=3
)

# Sobel Y
sobel_y16 = cv.Sobel(
    blurred,
    cv.CV_16S,
    0,
    1,
    ksize=3
)

sobel_x = cv.convertScaleAbs(sobel_x16)
sobel_y = cv.convertScaleAbs(sobel_y16)

sobel_combined = cv.addWeighted(
    sobel_x,
    0.5,
    sobel_y,
    0.5,
    0
)

# Scharr
scharr_x16 = cv.Scharr(
    blurred,
    cv.CV_16S,
    1,
    0
)

scharr_y16 = cv.Scharr(
    blurred,
    cv.CV_16S,
    0,
    1
)

scharr_x = cv.convertScaleAbs(scharr_x16)
scharr_y = cv.convertScaleAbs(scharr_y16)

scharr_combined = cv.addWeighted(
    scharr_x,
    0.5,
    scharr_y,
    0.5,
    0
)

# Laplacian
laplacian16 = cv.Laplacian(
    blurred,
    cv.CV_16S,
    ksize=3
)

laplacian = cv.convertScaleAbs(laplacian16)

# Canny
edges = cv.Canny(
    blurred,
    50,
    150,
    apertureSize=3,
    L2gradient=True
)

cv.imshow("Original", image)
cv.imshow("Gray", gray)
cv.imshow("Blurred", blurred)
cv.imshow("Sobel X", sobel_x)
cv.imshow("Sobel Y", sobel_y)
cv.imshow("Sobel Combined", sobel_combined)
cv.imshow("Scharr Combined", scharr_combined)
cv.imshow("Laplacian", laplacian)
cv.imshow("Canny Edges", edges)

cv.waitKey(0)
cv.destroyAllWindows()
```

---

## 四、本节小结

- **图像边缘** = 像素灰度变化较快的位置，可用**梯度强度**衡量：`G = sqrt(Gx²+Gy²)`（L2）或 `G = |Gx|+|Gy|`（L1）。
- **Sobel**（一阶）：可分 x / y 方向检测，`dx` 方向的强变化对应**垂直边缘**，`dy` 方向对应**水平边缘**；结果可能为负，需用 `CV_16S` 保存后再 `convertScaleAbs` 转 8 位。
- **Scharr**（一阶）：专为 `3×3` 核设计，比 Sobel 更精确，但不能自由选核大小。
- **Laplacian**（二阶）：一次检测多个方向、无需分 x/y，但对**噪声敏感**，使用前需先高斯滤波。
- **Canny**（多阶段）：高斯去噪 → 计算梯度 → 非极大值抑制（细化边缘）→ 双阈值检测 → 滞后边缘连接；`threshold1` 低 / `threshold2` 高，介于两者之间且与强边缘相连才保留。
- **选型**：Sobel 方向可控但边缘粗；Laplacian 检测全方向但怕噪；Canny 边缘细、效果稳定，但需调两个阈值。

---

## 相关链接

- 上一课：[[OpenCV第七课 轮廓检测与目标分析]]
- 下一课：[[OpenCV第九课 霍夫变换(检测直线或圆)]]（霍夫直线变换以 Canny 输出的二值边缘图为输入）
- 相关课程：[[OpenCV第五课 图像平滑与滤波]]（高斯滤波是 Canny 的预处理步骤）
- 主题归纳：[[OpenCV学习笔记/主题模块/特征检测与匹配]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
