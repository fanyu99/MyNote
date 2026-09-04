---
title: OpenCV 第八课 轮廓检测与目标分析
type: 课程笔记
课程: 8
created: 2026-09-03
updated: 2026-09-03
tags:
  - opencv
  - 课程笔记
  - 轮廓
status: 学习中
---

# OpenCV第八课 轮廓检测与目标分析

> **本课目标**：掌握用 OpenCV 查找图像轮廓的方法（`findContours`、层级关系、提取模式、点保存方式），并学会基于轮廓计算**面积、周长、外接矩形、中心点、最小外接圆、轮廓近似**等特征，从而实现目标检测、计数与形状分析。
> **涉及主题**：[[OpenCV学习笔记/主题模块/图像处理]]

---

## 一、轮廓检测

### 概念

轮廓常用于：
- 目标检测；
- 目标计数；
- 轮廓面积计算；
- 物体周长计算；
- 目标位置计算；
- 外接矩形检测；
- 形状识别；
- 缺陷检测。

> ⚠️ 在查找轮廓前，需要使用**二值化**或 **Canny 边缘检测**预处理，并且一般为**白色目标、黑色背景**。

### 轮廓检测的一般流程

```
读取彩色图像
    ↓
转换为灰度图
    ↓
滤波去噪
    ↓
二值化
    ↓
形态学处理
    ↓
查找轮廓
    ↓
计算轮廓特征
    ↓
绘制检测结果
```

### findContours() 函数

```C++
cv::findContours(
    image,
    contours,   // 轮廓集合(vector等)
    hierarchy,  // 轮廓的层级关系
    mode,       // 轮廓提取的模式
    method,     // 轮廓点保存方式
    offset      // 偏移量,一般不填
);
```

> ⚠️ `findContours` 会**修改传入的二值图像**，若后续仍需使用原图，请传入 `.clone()` 副本。

contours 类型：

```C++
std::vector<std::vector<cv::Point>> contours;
// 一般由二维集合表示：n 个轮廓，每个轮廓又有 m 个点
```

### 层级关系 hierarchy

当使用 `cv::RETR_TREE` / `cv::RETR_CCOMP` 时，会返回每个轮廓的层级信息：

```C++
std::vector<cv::Vec4i> hierarchy;
hierarchy[i][0]  // 下一个同级轮廓
hierarchy[i][1]  // 上一个同级轮廓
hierarchy[i][2]  // 第一个子轮廓
hierarchy[i][3]  // 父轮廓
```

#### 轮廓的提取模式

1. **RETR_EXTERNAL**：只提取最外层的轮廓。适合只关心物体的整体、目标计数以及简单的外部目标检测。

```C++
cv::findContours(
    image,
    contours,
    hierarchy,          // 层级信息
    cv::RETR_EXTERNAL,  // 仅关注外部的轮廓
    cv::CHAIN_APPROX_SIMPLE
);
```

2. **RETR_LIST**：提取所有的轮廓，但不建立父子层级关系。适合只想得到所有的轮廓、不关心轮廓之间关系的情况。

3. **RETR_CCOMP**：将轮廓分为两层结构——外部轮廓和内部轮廓。适合需要区分外部区域和孔洞、目标存在内部空洞的情况。

4. **RETR_TREE**：建立完整的轮廓层级关系。适合复杂的嵌套结构、需要分析轮廓的父子关系、处理多个层级孔洞的情况。

#### 轮廓点保存方式

1. **CHAIN_APPROX_NONE**：保存轮廓上的所有点。
2. **CHAIN_APPROX_SIMPLE**（常用）：压缩轮廓点，只保留关键拐点，节省内存。

### 绘制轮廓 drawContours()

```C++
cv::drawContours(
    drawing,
    contours,                  // 轮廓集合
    -1,                        // 绘制全部轮廓；0 表示第 0 个轮廓，-1 表示全部
    cv::Scalar(0, 255, 0),     // BGR 颜色，当前为绿色
    2                          // 线宽，-1 表示填充轮廓内部
);
```

### 轮廓数量 != 目标数量

目标可能含有多个轮廓，噪声可能被识别为轮廓，文字的每个笔画可能形成不同的轮廓，一个目标断裂会形成多个轮廓。通常需要结合：

```
RETR_EXTERNAL
面积过滤
形态学处理
```

### 核心要点

1. 查找轮廓前需先**二值化 / Canny 边缘检测**，一般为“白色目标、黑色背景”。
2. `findContours` 会**修改输入图像**，保护原图用 `.clone()`。
3. `RETR_EXTERNAL` 只取最外层、适合计数；`RETR_TREE` 建立完整层级；`CHAIN_APPROX_SIMPLE` 压缩轮廓点。
4. **轮廓数量 ≠ 目标数量**，需结合面积过滤、形态学处理。

---

## 二、目标分析（轮廓特征）

### 轮廓面积: contourArea()

```C++
double area = cv::contourArea(contours[i]);
```

### 面积过滤噪声

```C++
const double minArea = 500.0;

for (size_t i = 0; i < contours.size(); ++i) {
    double area = cv::contourArea(contours[i]);
    if (area < minArea) continue;
    // 仅处理面积较大的轮廓,忽略小面积的噪声
    // ...
}
```

### 轮廓周长(弧长): arcLength()

```C++
double perimeter = cv::arcLength(
    contour,
    true   // 轮廓是否闭合
);
```

例如计算闭合轮廓的周长：

```C++
for (const auto &contour : contours) {
    double perimeter = cv::arcLength(contour, true);
    qDebug() << "周长: " << perimeter;
}
```

### 外接矩形: boundingRect()

```C++
cv::Rect rect = cv::boundingRect(contour);  // 获取 contour 的水平外接矩形
// cv::Rect 包含左上角坐标及长宽
rect.x
rect.y
rect.width
rect.height
```

### 绘制外接矩形

```C++
cv::rectangle(
    drawing,
    rect,                 // 矩形
    cv::Scalar(0, 255, 0),// 颜色
    2                     // 线宽
);
```

### 计算目标轮廓填充程度 extent

```C++
std::vector<cv::Point> contour;
double contourArea = cv::contourArea(contour);
cv::Rect boundingBox = cv::boundingRect(contour);
double rectangleArea = boundingBox.width * boundingBox.height;
double extent = contourArea / rectangleArea;  // 越接近 1 说明轮廓越饱满
```

> `extent`（填充程度）接近 1 表示轮廓几乎填满外接矩形，适合判断目标形状是否规则、是否接近矩形。

### 轮廓中心点: moments()

图像矩用来计算：轮廓面积、重心、中心点、形状的相关特征。

```C++
cv::Moments moments = cv::moments(contour);
```

计算公式：

```C++
// centerX = m10 / m00;
// centerY = m01 / m00;
// 注意: 需要判断 m00 是否为零!!!
double centerX = moments.m10 / moments.m00;
double centerY = moments.m01 / moments.m00;
```

### 最小外接圆: minEnclosingCircle()

```C++
cv::Point2f center;   // 中心
float radius;         // 半径

cv::minEnclosingCircle(
    contour,   // 轮廓
    center,    // 中心
    radius     // 半径
);

// 绘制圆
cv::circle(
    drawing,
    center,
    static_cast<int>(radius),
    cv::Scalar(0, 0, 255),
    2
);
```

常用于：粗略表示目标范围、圆形目标检测、检测目标是否接近圆形、视觉测量。

### 轮廓近似: approxPolyDP()

```C++
cv::approxPolyDP(
    contour,      // 原始轮廓
    approximate,  // 近似后的轮廓
    epsilon,      // 最大距离偏差，越大，顶点越少（拐点越少）
    true          // 是否闭合
);
```

示例：

```C++
std::vector<cv::Point> approximate;

double perimeter = cv::arcLength(contour, true);
double epsilon = 0.02 * perimeter;

cv::approxPolyDP(
    contour,
    approximate,
    epsilon,
    true
);
```

> 某点距离大于 epsilon，被视为重要拐点保留；反之会被忽略。使用误差参数简化轮廓，**误差越大，保留的顶点通常越少**。

### 使用轮廓近似后的顶点数量进行简单的形状判断

```C++
std::vector<cv::Point> approximate;  // 近似的轮廓

double perimeter = cv::arcLength(contour, true);  // 获取周长
double epsilon = 0.02 * perimeter;                // 最大距离误差

// 获取近似的轮廓
cv::approxPolyDP(contour, approximate, epsilon, true);  // 闭合

// 获取轮廓的顶点数量
int vertexCount = static_cast<int>(approximate.size());

if (vertexCount == 3) {
    qDebug() << "可能是三角形";
} else if (vertexCount == 4) {
    qDebug() << "可能是四边形";
} else if (vertexCount > 4) {
    qDebug() << "可能是圆形或多边形";
}
```

> ⚠️ 仅仅根据顶点数量判断**不可靠**！例如：
> - 噪声可能改变顶点的数量；
> - epsilon 大小可能影响顶点数量；
> - 透视变形会影响轮廓；
> - 圆形可能会被近似为多个顶点的多边形。
>
> 通常需要**综合使用**：
> ```
> 面积
> 周长
> 宽高比    使用外接矩形的宽高比可做简单判断（注意高度不能为 0）
> 顶点数量
> 圆度
> 凸性
> 位置
> ```

### 基于轮廓的目标检测逻辑

1. 获取二值图
2. 查找轮廓
3. 遍历有效轮廓（使用面积过滤等）
4. 计算轮廓面积
5. 计算外接矩形
6. 绘制检测框
7. 统计有效目标的数量

### 核心要点

1. **面积** `contourArea`、**周长** `arcLength(contour, true)` 是最基本的轮廓特征。
2. **外接矩形** `boundingRect` 得到 `(x, y, w, h)`；`extent = 轮廓面积 / 矩形面积` 衡量填充程度。
3. **中心点**用 `moments`，计算 `m10/m00`、`m01/m00`，**必须判断 `m00` 是否为 0**。
4. **最小外接圆** `minEnclosingCircle` 适合圆形目标；**轮廓近似** `approxPolyDP` 用 `epsilon` 简化轮廓。
5. 顶点数量可辅助判断形状，但**不绝对可靠**，需综合面积、周长、宽高比、圆度、凸性等。

---

## 三、代码示例

```cpp
// C++ 示例
// 检测轮廓并绘制图形
#include <opencv2/opencv.hpp>

#include <QDebug>

#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    const std::string imagePath =
        R"(C:\Users\fanyu\Downloads\qq_pic_merged_1788356484263.jpg)";

    cv::Mat image = cv::imread(
        imagePath,
        cv::IMREAD_COLOR
    );

    if (image.empty()) {
        qDebug() << "图像读取失败";
        qDebug() << "请检查图片路径是否正确";
        return 1;
    }

    cv::Mat gray;
    cv::Mat filtered;
    cv::Mat binary;

    // 1. 彩色图像转换为灰度图
    cv::cvtColor(
        image,
        gray,
        cv::COLOR_BGR2GRAY
    );

    // 2. 高斯滤波，减少噪声
    cv::GaussianBlur(
        gray,
        filtered,
        cv::Size(5, 5),
        0
    );

    // 3. Otsu 自动二值化
    double otsuValue = cv::threshold(
        filtered,
        binary,
        0,
        255,
        cv::THRESH_BINARY | cv::THRESH_OTSU
    );

    qDebug() << "Otsu 阈值:"
             << otsuValue;

    // 4. 创建结构元素
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(3, 3)
    );

    // 5. 开运算，去除小白色噪声
    cv::Mat opened;
    cv::morphologyEx(
        binary,
        opened,
        cv::MORPH_OPEN,
        kernel
    );

    // 6. 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    // 使用 clone() 保留 opened 原图，便于后续继续显示或处理
    cv::findContours(
        opened.clone(),
        contours,
        hierarchy,
        cv::RETR_EXTERNAL,
        cv::CHAIN_APPROX_SIMPLE
    );

    qDebug() << "检测到的轮廓数量:"
             << static_cast<int>(contours.size());

    // 在彩色原图上绘制结果
    cv::Mat result = image.clone();

    const double minArea = 500.0;

    int validObjectCount = 0;

    for (size_t i = 0; i < contours.size(); ++i) {
        const auto &contour = contours[i];

        // 计算轮廓面积
        double area = cv::contourArea(contour);

        // 过滤小轮廓
        if (area < minArea) {
            continue;
        }

        ++validObjectCount;

        // 计算轮廓周长
        double perimeter = cv::arcLength(
            contour,
            true
        );

        // 计算水平外接矩形
        cv::Rect boundingBox =
            cv::boundingRect(contour);

        // 计算轮廓中心
        cv::Moments moments =
            cv::moments(contour);

        cv::Point center(0, 0);

        if (std::abs(moments.m00) > 1e-5) {
            center.x = static_cast<int>(
                moments.m10 / moments.m00
            );

            center.y = static_cast<int>(
                moments.m01 / moments.m00
            );
        }

        // 绘制轮廓
        cv::drawContours(
            result,
            contours,
            static_cast<int>(i),
            cv::Scalar(0, 255, 0),
            2
        );

        // 绘制外接矩形
        cv::rectangle(
            result,
            boundingBox,
            cv::Scalar(255, 0, 0),
            2
        );

        // 绘制中心点
        cv::circle(
            result,
            center,
            4,
            cv::Scalar(0, 0, 255),
            -1  // 填充中心点
        );

        // 显示轮廓编号
        cv::putText(
            result,
            "ID: " + std::to_string(validObjectCount),
            cv::Point(
                boundingBox.x,
                std::max(20, boundingBox.y - 8)
            ),
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            cv::Scalar(0, 255, 255),
            2
        );

        qDebug() << "有效轮廓:"
                 << validObjectCount
                 << "面积:"
                 << area
                 << "周长:"
                 << perimeter
                 << "外接矩形:"
                 << boundingBox.width
                 << "x"
                 << boundingBox.height
                 << "中心:"
                 << center.x
                 << ","
                 << center.y;
    }

    qDebug() << "有效目标数量:"
             << validObjectCount;

    cv::imshow("Original", image);
    cv::imshow("Gray", gray);
    cv::imshow("Binary", binary);
    cv::imshow("Opened", opened);
    cv::imshow("Contour Result", result);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}
```

```python
# Python 示例
import cv2

image_path = r"C:\Users\fanyu\Downloads\qq_pic_merged_1788356484263.jpg"

image = cv2.imread(
    image_path,
    cv2.IMREAD_COLOR
)

if image is None:
    print("图像读取失败")
    exit(1)

# 1. 转灰度
gray = cv2.cvtColor(
    image,
    cv2.COLOR_BGR2GRAY
)

# 2. 高斯滤波
filtered = cv2.GaussianBlur(
    gray,
    (5, 5),
    0
)

# 3. Otsu 二值化
otsu_value, binary = cv2.threshold(
    filtered,
    0,
    255,
    cv2.THRESH_BINARY | cv2.THRESH_OTSU
)

print("Otsu 阈值:", otsu_value)

# 4. 开运算去噪
kernel = cv2.getStructuringElement(
    cv2.MORPH_RECT,
    (3, 3)
)

opened = cv2.morphologyEx(
    binary,
    cv2.MORPH_OPEN,
    kernel
)

# 5. 查找外部轮廓
contours, hierarchy = cv2.findContours(
    opened.copy(),
    cv2.RETR_EXTERNAL,
    cv2.CHAIN_APPROX_SIMPLE
)

print("轮廓数量:", len(contours))

result = image.copy()

min_area = 500
valid_count = 0

for contour in contours:
    # 计算面积
    area = cv2.contourArea(contour)

    if area < min_area:
        continue

    valid_count += 1

    # 计算周长
    perimeter = cv2.arcLength(
        contour,
        True
    )

    # 外接矩形
    x, y, w, h = cv2.boundingRect(contour)

    # 计算中心点
    moments = cv2.moments(contour)

    if abs(moments["m00"]) > 1e-5:
        center_x = int(moments["m10"] / moments["m00"])
        center_y = int(moments["m01"] / moments["m00"])
    else:
        center_x = 0
        center_y = 0

    # 绘制轮廓
    cv2.drawContours(
        result,
        [contour],
        -1,
        (0, 255, 0),
        2
    )

    # 绘制外接矩形
    cv2.rectangle(
        result,
        (x, y),
        (x + w, y + h),
        (255, 0, 0),
        2
    )

    # 绘制中心点
    cv2.circle(
        result,
        (center_x, center_y),
        4,
        (0, 0, 255),
        -1
    )

    # 显示编号
    cv2.putText(
        result,
        f"ID: {valid_count}",
        (x, max(20, y - 8)),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (0, 255, 255),
        2
    )

    print(
        f"目标 {valid_count}: "
        f"面积={area:.2f}, "
        f"周长={perimeter:.2f}, "
        f"矩形={w}x{h}, "
        f"中心=({center_x}, {center_y})"
    )

print("有效目标数量:", valid_count)

cv2.imshow("Original", image)
cv2.imshow("Gray", gray)
cv2.imshow("Binary", binary)
cv2.imshow("Opened", opened)
cv2.imshow("Contour Result", result)

cv2.waitKey(0)
cv2.destroyAllWindows()
```

---

## 四、本节小结

- **轮廓检测流程**：读取彩色图 → 转灰度 → 滤波去噪 → 二值化 → 形态学处理 → `findContours` → 计算特征 → 绘制结果。
- **`findContours`** 会修改输入图像，用 `.clone()` 保护原图；常用 `RETR_EXTERNAL` + `CHAIN_APPROX_SIMPLE`。
- **轮廓数量 ≠ 目标数量**，需结合**面积过滤**与**形态学处理**去除噪声。
- **目标特征**：面积 `contourArea`、周长 `arcLength`、外接矩形 `boundingRect`、中心点 `moments`、最小外接圆 `minEnclosingCircle`、轮廓近似 `approxPolyDP`、填充程度 `extent`。
- **`moments` 计算中心点必须判断 `m00` 是否为 0**（避免除零）。
- `approxPolyDP` 的顶点数量可**辅助判断形状**，但不绝对可靠，需综合面积、周长、宽高比、圆度、凸性等特征。

---

## 相关链接

- 上一课：[[OpenCV第七课 形态学处理]]
- 下一课：（待添加）
- 主题归纳：[[OpenCV学习笔记/主题模块/图像处理]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
