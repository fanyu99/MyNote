---
title: OpenCV 第九课 霍夫变换（检测直线或圆）
type: 课程笔记
课程: 9
created: 2026-09-07
updated: 2026-09-09
tags:
  - opencv
  - 课程笔记
status: 学习中
---

# OpenCV第九课 霍夫变换（课程笔记）

> **本课目标**：理解霍夫变换的核心原理（极坐标表示直线、累加器投票），学会用 `cv::HoughLines()` / `cv::HoughLinesP()` 检测直线，用 `cv::HoughCircles()` 检测圆，并能绘制检测结果。
> **涉及主题**：[[OpenCV学习笔记/主题模块/特征检测与匹配]]

---

## 一、霍夫变换检测直线

### 概念

霍夫变换一般用于处理CV_8UC1类型的二值边缘图(也就是Canny的输出结果)

霍夫变换使用极坐标表示直线:
```
ρ = x cosθ + y sinθ
```
其中:
- `x`、`y`：图像中的像素坐标
- `ρ`：坐标原点到直线的垂直距离
- `θ`：垂线与 x 轴的夹角
例如:
rho = 100,theta = 0时,垂线水平,该直线就是一条竖直的直线,位于x=100的位置

### 核心原理

一个点会被很多条直线穿过,对于每一条可能的直线都可以得出(rho,theta)组合
如果很多边缘像素同属一条实际直线,那么他们在参数空间中会相交于相近的位置
使用一个累加器记录每个(rho,theta)组合的票数
票数越高,越可能是一条真实的直线

### cv::HoughLines()
```C++
void cv::HoughLines(
	InputArray image,
	OutputArray lines, 
	// lines 通常使用std::vector<cv::Vec2f>
	// cv::Vec2f:(rho,theta)组合
	double rho,  // 划分距离,单位是像素
	double theta, // 划分角度,弧度制
	int threshold, // 累加器的票数阈值
	double srn = 0 , 
	double stn = 0 ,
	double min_theta = 0, 
	double max_theta = CV_PI
);
// 例如
cv::HoughLines(
	edges,
	lines,
	1, // 每隔1像素划分一个区间
	CV_PI/180,  // 每隔1度划分一个区间
	100 // 100票认为是一条直线
);
```

### 如何绘制HoughLines()检测到的直线?

HoughLines()得到的是std::vector<cv::Vec2f>,需要通过rho,theta计算两个足够远的点
```C++
// 计算垂点坐标
double a = std::cos(theta);
double b = std::sin(theta);
double x0 = a * rho; // 获取x
double y0 = b * rho; // 获取y
// 随便获取两个直线上的远点
Point pt1(
    cvRound(x0 + 1000 * (-b)),
    cvRound(y0 + 1000 * a)
);

Point pt2(
    cvRound(x0 - 1000 * (-b)),
    cvRound(y0 - 1000 * a)
);

// 绘制直线
cv::line(
	InputOutputArray image, // 图像
	Point pt1, // 点1
	Point pt2, // 点2
	const Scalar& color,  // 颜色
	int thickness =1 , // 线宽(像素)
	int lineType = LINE_8, // 线型
	int shift = 0 // 点坐标的小数位数
);
```

### cv::HoughLinesP()
```C++
void cv::HoughLinesP(
	InputArray image,
	OutputArray lines, 
	// 输出线段,std::vector<cv::Vec4i>
	// 包含四个整数: 起点(x1,y1),终点(x2,y2)
	double rho, // 划分距离
	double theta, // 划分角度
	int threshold, // 票数阈值
	double minLineLength=0, // 最小线段长度(低于该长度的线段会被舍弃)
	double maxLineGap =0  // 同一直线上允许连接的最大间隙(连接可能是同一直线的断开的两条线段)
);

// 例如
cv::HoughLinesP(
	image,
	lines,
	1, // 每隔1像素划分一个区域
	cv_PI/180, // 每隔1度划分一个区域
	100, // 票数阈值
	50, // 最短线段长度
	20 // 最大间隙
);
```

### 标准霍夫变换和概率霍夫变换的区别

| 对比项      | `HoughLines()` | `HoughLinesP()`  |
| -------- | -------------- | ---------------- |
| 输出结果     | `rho, theta`   | `x1, y1, x2, y2` |
| 表示内容     | 无限延伸的直线        | 实际线段             |
| 是否直接得到端点 | 否              | 是                |
| 使用难度     | 稍高             | 较简单              |
| 是否适合长度分析 | 不方便            | 方便               |
| 计算效率     | 通常较慢           | 通常更快             |
| 工程中常用程度  | 理论分析较多         | 实际项目常用           |

### 计算线段长度和角度

dx = x1-x2;
dy = y1-y2;
使用HoughLinesP()得到的结果的长度公式:
length = sqrt( dx* dx + dy * dy );
角度公式:
angle = std::atan2(dy,dx)* 180/CV_PI;

### 核心要点

- 霍夫变换输入是**二值边缘图**（通常为 Canny 输出，见 [[OpenCV第八课 Canny边缘检测]]），直线用极坐标 `ρ = xcosθ + ysinθ` 表示。
- `HoughLines()` 输出 `std::vector<cv::Vec2f>`（`rho, theta`，表示**无限直线**，绘制需自行换算两个远点）。
- `HoughLinesP()` 输出 `std::vector<cv::Vec4i>`（`x1,y1,x2,y2`，表示**真实线段**），直接得端点，实际项目更常用、更快。
- 线段长度 `sqrt(dx²+dy²)`，角度 `atan2(dy,dx) * 180/CV_PI`。

---

## 二、霍夫变换圆检测(自动调用Canny边缘检测)
```C++
void cv::HoughCircles(
    cv::InputArray image,
    cv::OutputArray circles,
    int method, // 检测方法,通常使用cv::HOUGH_GRADIENT
    double dp, // 累加器分辨率与原图的分辨率的反比,dp越小,检测越精细
    double minDist, // 圆心的最小距离,用于分别两个不同的圆
    double param1 = 100, //Canny边缘检测的高阈值,低阈值为其一半
    double param2 = 100, // 圆检测的累加器阈值
    int minRadius = 0, // 认为是圆的最小半径
    int maxRadius = 0 // 认为是圆的最大半径
);
```

### 霍夫圆检测和轮廓检测的区别
|对比项|霍夫圆检测|轮廓检测|
|---|---|---|
|主要目标|检测圆|检测任意封闭边界|
|输入重点|灰度图和边缘信息|二值图|
|是否必须闭合|不一定完全闭合|通常需要较完整轮廓|
|输出|圆心和半径|点集轮廓|
|对遮挡的容忍度|相对较强|通常较弱|
|计算量|较大|通常较小|
|是否适合检测圆|适合|也可以，但需要后续拟合|
|是否能分析复杂形状|不适合|适合|

### 代码示例

```C++
#include <algorithm>
#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

int main()
{
    cv::Mat image = cv::imread(
        R"(C:\Users\fanyu\Videos\NVIDIA\Wuthering Waves\Wuthering Waves Screenshot 2026.06.08 - 22.21.55.10.png)");

    if (image.empty()) {
        std::cerr << "图像读取失败" << std::endl;
        return -1;
    }

    // BGR 图像转换为灰度图
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // 中值滤波，减少噪声并保留边缘
    cv::medianBlur(gray, gray, 5);

    std::vector<cv::Vec3f> circles;

    const double minDist = std::max(20.0, gray.rows / 16.0);

    cv::HoughCircles(
        gray,
        circles,
        cv::HOUGH_GRADIENT,
        1.0,       // dp
        minDist,   // minDist
        100,       // param1
        30,        // param2
        10,        // minRadius
        300        // maxRadius
    );

    cv::Mat result = image.clone();

    for (const cv::Vec3f& circle : circles) {
	    // 获取圆心
        cv::Point center(
            cvRound(circle[0]),
            cvRound(circle[1])
        );

        int radius = cvRound(circle[2]);

        // 绘制圆心
        cv::circle(
            result,
            center,
            3,
            cv::Scalar(0, 0, 255),
            -1, // 实心
            cv::LINE_AA
        );

        // 绘制圆周
        cv::circle(
            result,
            center,
            radius,
            cv::Scalar(0, 255, 0),
            2,
            cv::LINE_AA
        );

        std::cout
            << "圆心：("
            << center.x
            << ", "
            << center.y
            << ")，半径："
            << radius
            << std::endl;
    }

    std::cout << "检测到的圆数量：" << circles.size() << std::endl;

    cv::imshow("原图", image);
    cv::imshow("灰度图", gray);
    cv::imshow("霍夫圆检测结果", result);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}

```


### 概念

`HoughCircles()` 内部**自动调用 Canny 边缘检测**，输入灰度图即可，无需手动准备二值图。

### 核心要点

- 常用方法 `cv::HOUGH_GRADIENT`；输出 `std::vector<cv::Vec3f>`（`cx, cy, radius`）。
- 关键参数：`dp` 越小累加器分辨率越高越精细；`minDist` 用于分离相邻圆；`param1` 是 Canny 高阈值（低阈值为其一半）；`param2` 是圆检测累加器阈值；`minRadius / maxRadius` 限制半径范围。
- 圆检测对**遮挡容忍度相对较强**，但计算量较大；相比轮廓检测更专注"检测圆"这一任务（圆检测 vs 轮廓检测的对比见上文表格）。

---

## 三、代码示例

```cpp
// C++ 示例
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// 使用cv::HoughLines()

int main()
{
    cv::Mat image = cv::imread(
        "D:/Desktop/OpenCV学习/test.jpg"
    );

    if (image.empty())
    {
        std::cerr << "图像读取失败" << std::endl;
        return -1;
    }

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat blurred;
    cv::GaussianBlur(
        gray,
        blurred,
        cv::Size(5, 5),
        0
    );

    cv::Mat edges;
    cv::Canny(
        blurred,
        edges,
        50,
        150
    );

    std::vector<cv::Vec2f> lines;

    cv::HoughLines(
        edges,
        lines,
        1,
        CV_PI / 180,
        100
    );

    cv::Mat result = image.clone();

    for (const cv::Vec2f& lineData : lines)
    {
        float rho = lineData[0];
        float theta = lineData[1];

        double a = std::cos(theta);
        double b = std::sin(theta);

        double x0 = a * rho;
        double y0 = b * rho;

        cv::Point pt1(
            cvRound(x0 + 1000 * (-b)),
            cvRound(y0 + 1000 * a)
        );

        cv::Point pt2(
            cvRound(x0 - 1000 * (-b)),
            cvRound(y0 - 1000 * a)
        );

        cv::line(
            result,
            pt1,
            pt2,
            cv::Scalar(0, 0, 255),
            2,
            cv::LINE_AA
        );
    }

    cv::imshow("原图", image);
    cv::imshow("Canny边缘", edges);
    cv::imshow("HoughLines直线", result);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}

// 使用cv::HoughLinesP()

int main()
{
    cv::Mat image = cv::imread(
        "D:/Desktop/OpenCV学习/test.jpg"
    );

    if (image.empty())
    {
        std::cerr << "图像读取失败" << std::endl;
        return -1;
    }

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat blurred;
    cv::GaussianBlur(
        gray,
        blurred,
        cv::Size(5, 5),
        0
    );

    cv::Mat edges;
    cv::Canny(
        blurred,
        edges,
        50,
        150
    );

    std::vector<cv::Vec4i> lines;

    cv::HoughLinesP(
        edges,
        lines,
        1,
        CV_PI / 180,
        80,
        50,
        10
    );

    cv::Mat result = image.clone();

    for (const cv::Vec4i& lineData : lines)
    {
        int x1 = lineData[0];
        int y1 = lineData[1];
        int x2 = lineData[2];
        int y2 = lineData[3];

        cv::line(
            result,
            cv::Point(x1, y1),
            cv::Point(x2, y2),
            cv::Scalar(0, 0, 255),
            2,
            cv::LINE_AA
        );
    }

    cv::imshow("原图", image);
    cv::imshow("Canny边缘", edges);
    cv::imshow("HoughLinesP线段", result);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}
```

```python
# Python 示例
```

---

## 四、本节小结

- **核心思想**：把"图像空间找直线/圆"转化为"参数空间累加器投票"，票数超过阈值即判定存在目标。
- **直线检测**：`HoughLines()` 输出无限直线（`rho, theta`），`HoughLinesP()` 输出真实线段（`x1,y1,x2,y2`），后者工程中更常用。
- **圆检测**：`HoughCircles()` 自动调用 Canny，输出圆心 + 半径（`cx, cy, radius`）。
- **输入**：直线检测需**二值边缘图**（Canny 输出）；圆检测内部自动做 Canny，给灰度图即可。

---

## 相关链接

- 上一课：[[OpenCV第八课 Canny边缘检测]]（提供霍夫直线变换所需的二值边缘图）
- 下一课：[[OpenCV第十课 图像几何变换]]
- 相关课程：[[OpenCV第七课 轮廓检测与目标分析]]（另一种形状检测思路，可与圆检测对比）
- 相关课程：[[OpenCV第五课 图像平滑与滤波]]（检测前的去噪预处理）
- 主题归纳：[[OpenCV学习笔记/主题模块/特征检测与匹配]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
