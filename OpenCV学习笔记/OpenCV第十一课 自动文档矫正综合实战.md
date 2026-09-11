---
title:
type: 课程笔记
课程:
created: 2026-09-09
updated: 2026-09-09
tags:
  - opencv
  - 课程笔记
status: 学习中
---

# OpenCV第十一课 自动文档矫正综合实战

> **本课目标**：
> **涉及主题**：

---

## 一、综合知识,做一个文档矫正项目
```
灰度转换
    ↓
高斯滤波
    ↓
Canny 边缘检测
    ↓
形态学闭运算
    ↓
轮廓检测
    ↓
轮廓近似
    ↓
筛选四边形
    ↓
透视变换
    ↓
得到矫正后的文档
```

```C++
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <opencv2/opencv.hpp>
#include <vector>
// 获取透视变换需要的四个角点
std::array<cv::Point2f, 4> orderPoints(
    const std::vector<cv::Point>& points
)
{
    std::array<cv::Point2f, 4> ordered{};

    float minSum = std::numeric_limits<float>::max();
    float maxSum = std::numeric_limits<float>::lowest();

    float minDiff = std::numeric_limits<float>::max();
    float maxDiff = std::numeric_limits<float>::lowest();

    cv::Point2f topLeft;
    cv::Point2f topRight;
    cv::Point2f bottomRight;
    cv::Point2f bottomLeft;

    for (const cv::Point& point : points) {
        float sum = static_cast<float>(point.x + point.y);
        float diff = static_cast<float>(point.x - point.y);

        if (sum < minSum) {
            minSum = sum;
            topLeft = point;
        }

        if (sum > maxSum) {
            maxSum = sum;
            bottomRight = point;
        }

        if (diff > maxDiff) {
            maxDiff = diff;
            topRight = point;
        }

        if (diff < minDiff) {
            minDiff = diff;
            bottomLeft = point;
        }
    }

    ordered[0] = topLeft;
    ordered[1] = topRight;
    ordered[2] = bottomRight;
    ordered[3] = bottomLeft;

    return ordered;
}

int main()
{
    cv::Mat image = cv::imread(
        R"(D:\OneDrive\图片\Screenshots\屏幕截图 2026-09-07 001128.png)"
    );

    if (image.empty()) {
        std::cerr << "图像读取失败" << std::endl;
        return -1;
    }

    // 1. 转换为灰度图
    cv::Mat gray;
    cv::cvtColor(
        image,
        gray,
        cv::COLOR_BGR2GRAY
    );

    // 2. 高斯滤波
    cv::Mat blurred;
    cv::GaussianBlur(
        gray,
        blurred,
        cv::Size(5, 5),
        0
    );

    // 3. Canny 边缘检测
    cv::Mat edges;
    cv::Canny(
        blurred,
        edges,
        50,
        150
    );

    // 4. 闭运算，连接断裂边缘
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(5, 5)
    );

    cv::Mat closedEdges;

	// 闭运算
    cv::morphologyEx(
        edges,
        closedEdges,
        cv::MORPH_CLOSE,
        kernel
    );

    // 5. 查找外部轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(
        closedEdges,
        contours,
        hierarchy,
        cv::RETR_EXTERNAL, // 外部轮廓
        cv::CHAIN_APPROX_SIMPLE
    );

    // 6. 查找面积最大的凸四边形
    double imageArea =
        static_cast<double>(image.cols * image.rows);

    double minArea = imageArea * 0.10;
    double largestArea = 0.0;

    std::vector<cv::Point> documentContour;
	// 查找最大的凸四边形
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);

        if (area < minArea) {
            continue;
        }

        double perimeter = cv::arcLength(
            contour,
            true
        );
	
		// 对轮廓作近似处理
        std::vector<cv::Point> approximation;

        cv::approxPolyDP(
            contour,
            approximation,
            0.02 * perimeter,
            true
        );
		// 不是四边形舍弃
        if (approximation.size() != 4) {
            continue;
        }

        if (!cv::isContourConvex(approximation)) {
            continue;
        }

        if (area > largestArea) {
            largestArea = area;
            documentContour = approximation;
        }
    }

    if (documentContour.empty()) {
        std::cerr << "没有检测到合适的文档四边形"
                  << std::endl;

        cv::imshow("边缘图", edges);
        cv::imshow("闭运算结果", closedEdges);
        cv::waitKey(0);

        return -1;
    }

    // 7. 给四个角点排序
    std::array<cv::Point2f, 4> ordered =
        orderPoints(documentContour);

    // 8. 自动计算输出尺寸
    double topWidth = cv::norm(
        ordered[1] - ordered[0]
    );

    double bottomWidth = cv::norm(
        ordered[2] - ordered[3]
    );

    double leftHeight = cv::norm(
        ordered[3] - ordered[0]
    );

    double rightHeight = cv::norm(
        ordered[2] - ordered[1]
    );

    int outputWidth = static_cast<int>(
        std::max(topWidth, bottomWidth)
    );

    int outputHeight = static_cast<int>(
        std::max(leftHeight, rightHeight)
    );

    if (outputWidth <= 0 || outputHeight <= 0) {
        std::cerr << "输出尺寸无效" << std::endl;
        return -1;
    }

    // 9. 构造目标矩形的四个角点
    std::vector<cv::Point2f> destinationPoints = {
        cv::Point2f(0, 0),
        cv::Point2f(
            outputWidth - 1.0F,
            0
        ),
        cv::Point2f(
            outputWidth - 1.0F,
            outputHeight - 1.0F
        ),
        cv::Point2f(
            0,
            outputHeight - 1.0F
        )
    };

    // 10. 计算透视变换矩阵
    std::vector<cv::Point2f> sourcePoints(
        ordered.begin(),
        ordered.end()
    );

    cv::Mat perspectiveMatrix =
        cv::getPerspectiveTransform(
            sourcePoints,
            destinationPoints
        );

    // 11. 执行透视矫正
    cv::Mat corrected;

    cv::warpPerspective(
        image,
        corrected,
        perspectiveMatrix,
        cv::Size(outputWidth, outputHeight)
    );

    // 12. 在原图上绘制检测到的四边形
    cv::Mat marked = image.clone();

    for (int i = 0; i < 4; ++i) {
        cv::line(
            marked,
            documentContour[i],
            documentContour[(i + 1) % 4],
            cv::Scalar(0, 0, 255),
            3,
            cv::LINE_AA
        );

        cv::circle(
            marked,
            documentContour[i],
            8,
            cv::Scalar(0, 255, 0),
            -1
        );
    }

    std::cout << "文档轮廓面积："
              << largestArea
              << std::endl;

    std::cout << "矫正后尺寸："
              << outputWidth
              << " x "
              << outputHeight
              << std::endl;

    // 13. 显示结果
    cv::imshow("原图", image);
    cv::imshow("Canny边缘图", edges);
    cv::imshow("检测到的文档边界", marked);
    cv::imshow("透视矫正结果", corrected);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}
```

### 常见问题

#### 1. 为什么检测不到文档?
1. 文档和背景颜色接近,例如白色纸张放在白色桌面
		导致Canny不容易检测到边缘
		可以尝试:调整Canny阈值,增强对比度,使用阴影消除,使用彩色通道分析
2. 文档边界断裂
		可以适当增大卷积核的大小
3. 文档不是最大的四边形,可以增加更多的条件,例如宽高比例,角度接近直角,进行颜色和纹理判断
4. 四个角点顺序出错:需要使用保证:
		左上 → 右上 → 右下 → 左下
	
### 概念

### 核心要点

---

## 二、{{本节主题2}}

### 概念

### 核心要点

---

## 三、代码示例

```cpp
// C++ 示例
```

```python
# Python 示例
```

---

## 四、本节小结

- 
- 

---

## 相关链接

- 上一课：[[OpenCV第一课 基础入门]]
- 下一课：（待添加）
- 主题归纳：[[OpenCV学习笔记/主题模块/图像基础与色彩空间]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
