---
title: OpenCV 第十课 图像几何变换
type: 课程笔记
课程: 10
created: 2026-09-09
updated: 2026-09-09
tags:
  - opencv
  - 课程笔记
  - 几何变换
  - 仿射变换
  - 透视变换
status: 学习中
---

# OpenCV第十课 图像几何变换

> **本课目标**：掌握图像**几何变换**的核心——**仿射变换**（平移、旋转、缩放、错切）与**透视变换**（四点矫正）。理解 `2×3` 与 `3×3` 变换矩阵的区别与插值方法，并能完成文档 / 证件透视矫正。
> **涉及主题**：[[OpenCV学习笔记/主题模块/图像处理]]

---

## 一、仿射变换

### 平移（仿射变换的特例）

图像平移的一般步骤就是:
1. 构造平移矩阵,平移矩阵形式一般为
```
x1 = a * x + tx
y1 = b * y + ty
[x1] = [1,0]  *  [x]  + [tx]
[y1]   [0,1]     [y]    [ty]
合成为齐次方程组为[1,0,tx]
			   [0,1,ty]
```
2. 进行仿射变换 

```cpp
// 设置平移矩阵
cv::Mat translationMatrix = (cv::Mat_<double>(2, 3) <<
    1, 0, 100,
    0, 1, 50
);

cv::Mat translated;
// 仿射变换
cv::warpAffine(
    image,
    translated,
    translationMatrix,
    image.size()
);
```


### 旋转

```c++
cv::Mat cv::getRotationMatrix2D(
	cv::Point2f center, // 旋转中心
	double angle, // 旋转角度(度),正为逆时针
	double scale // 缩放比例
);
```
### 旋转图像示例

```C++
// 获取旋转中心
cv::Point2f center(
    image.cols / 2.0F,
    image.rows / 2.0F
);
// 角度/缩放比例
double angle = 30.0;
double scale = 1.0;
// 获取仿射变换的旋转矩阵
cv::Mat rotationMatrix = cv::getRotationMatrix2D(
    center,
    angle,
    scale
);

cv::Mat rotated;

// 对原图进行仿射变换得到最终的旋转图像
cv::warpAffine(
    image,
    rotated,
    rotationMatrix,
    image.size()
);
```

### 什么是仿射变换
仿射变换是有线性变换和位移组成的一类几何变换
可以表示:
1. 平移
2. 旋转
3. 缩放
4. 错切
5. 上述操作集合
仿射变换通常使用2 * 3的矩阵:
```
[ x' ]   [ a00  a01  b0 ] [ x ]
[ y' ] = [ a10  a11  b1 ] [ y ]
                          [ 1 ]
```
### 仿射变换的特点:
1. 直线变换后仍然直线
2. 平行线变换后仍然保持平行
3. 同一条直线的比例关系不变
4. 一般不能将一个矩形变成任意的四边形
5. **仿射变换的三个点不能在一条直线上**

### 仿射矩阵的计算方法
```C++
// 返回2 * 3的仿射变换矩阵
cv::Mat cv::getAffineTransform(
	InputArray src, // 原图像的三个点
	InputArray dst  // 目标图像的三个对应的点
);

```
### 仿射变换的示例
```C++
cv::Mat affineInput = image.clone();

std::vector<cv::Point2f> srcPoints = {
    cv::Point2f(0, 0),
    cv::Point2f(image.cols - 1.0F, 0),
    cv::Point2f(0, image.rows - 1.0F)
};

std::vector<cv::Point2f> dstPoints = {
    cv::Point2f(50, 50),
    cv::Point2f(image.cols - 100.0F, 80),
    cv::Point2f(80, image.rows - 50.0F)
};

cv::Mat affineMatrix = cv::getAffineTransform(
    srcPoints,
    dstPoints
);

cv::Mat affineResult;
// 例如原三个点为(0,0),(399,0),(0,299)
// 变换的三个目标点(50,50),(300,80),(80,250)
// 最终结果为: 
// 原图左上角进行右移+下移
// 原图右上角进行左移+下移
// 原图左下角进行右移+上移
cv::warpAffine(
    affineInput,
    affineResult,
    affineMatrix,
    image.size()
);
```


## 二、透视变换

### 什么是透视变换?
```
现实中的纸张：       拍摄后的纸张：

┌──────────┐          ╱────────╲
│          │         ╱          ╲
│          │        ╱            ╲
└──────────┘       └──────────────┘
```
透视变换可以将拍摄透视的纸张进行拉正为矩形
常见应用:
1. 身份证矫正
2. 票据矫正
3. 书页矫正
4. 车牌矫正
5. 文档扫描
6. 俯视图转换
7. 透视畸变修正
### 函数原型及应用
```C++
cv::Mat cv::getPerspectiveTransform(
	InputArray src, // 原图像
	InputArray dst //  获取变换的矩阵 
);
void cv::warpPerspective(
	InputArray src,
	OutputAarray dst, // 输出图像
	InputArray M, // 变换矩阵
	cv::Size dsize, // 输出图像大小
	int flags = cv::INTER_LINEAR, // 插值方式
	int borderMode = cv::BORDER_CONSTANT, // 边界处理模式
	const cv::Scalar& borderValue = cv::Scalar() // 常量边界颜色
);
```

### 仿射变换和透视变换的区别
| 对比       | 仿射变换           | 透视变换                |
| -------- | -------------- | ------------------- |
| 矩阵       | `2×3`          | `3×3`               |
| 对应点数量    | 3 组            | 4 组                 |
| 直线       | 仍是直线           | 仍是直线                |
| 平行线      | 仍保持平行          | 可能变成相交线             |
| 能否处理梯形矫正 | 能力有限           | 非常适合                |
| 函数       | `warpAffine()` | `warpPerspective()` |

### 文档矫正完整示例

现在已经知道文档的四个角点:左上角,左下角,右上角,右下角;
现在需要将其变为规则的矩形
```C++
#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

int main()
{
    cv::Mat image = cv::imread(
        R"(D:\OneDrive\图片\Screenshots\屏幕截图 2026-09-07 001128.png)"
    );

    if (image.empty()) {
        std::cerr << "图像读取失败" << std::endl;
        return -1;
    }

    /*
        假设已经从图像中检测出了文档的四个角点。

        顺序必须统一：
        0：左上
        1：右上
        2：右下
        3：左下
    */
    std::vector<cv::Point2f> sourcePoints = {
        cv::Point2f(120, 80),
        cv::Point2f(650, 100),
        cv::Point2f(700, 520),
        cv::Point2f(90, 500)
    };
	// 指定输出的文档大小
    int outputWidth = 600;
    int outputHeight = 800;
	// 目标点必须完整对应
    std::vector<cv::Point2f> destinationPoints = {
        cv::Point2f(0, 0),
        cv::Point2f(outputWidth - 1.0F, 0),
        cv::Point2f(outputWidth - 1.0F, outputHeight - 1.0F),
        cv::Point2f(0, outputHeight - 1.0F)
    };
	// 获取变换矩阵
    cv::Mat perspectiveMatrix =
        cv::getPerspectiveTransform(
            sourcePoints,
            destinationPoints
        );

    cv::Mat corrected;
	// 获取修正后的文档
    cv::warpPerspective(
        image,
        corrected,
        perspectiveMatrix,
        cv::Size(outputWidth, outputHeight)
    );

    // 在原图上标记四个角点
    cv::Mat marked = image.clone();

    for (const cv::Point2f& point : sourcePoints) {
        cv::circle(
            marked,
            point,
            8,
            cv::Scalar(0, 0, 255),
            -1
        );
    }

    cv::imshow("原图与角点", marked);
    cv::imshow("透视矫正结果", corrected);

    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}
```

### 如何自动计算输出的尺寸?
```C++
// 获取上下的宽度以及左右的高度
// norm用于计算两点之间的欧几里得距离,即distance = ((x1-x2)^2+(y1-y2)^2)^0.5
double topWidth = cv::norm(
    sourcePoints[1] - sourcePoints[0]
);

double bottomWidth = cv::norm(
    sourcePoints[2] - sourcePoints[3]
);

double leftHeight = cv::norm(
    sourcePoints[3] - sourcePoints[0]
);

double rightHeight = cv::norm(
    sourcePoints[2] - sourcePoints[1]
);

// 得出最后的输出尺寸
int outputWidth = static_cast<int>(
    std::max(topWidth, bottomWidth)
);

int outputHeight = static_cast<int>(
    std::max(leftHeight, rightHeight)
);
```

### warpAffine() 和 warpPerspective()的使用流程
#### 仿射变换warpAffine()
```
三个源点
   +
三个目标点
   ↓
getAffineTransform()
   ↓
2×3 矩阵
   ↓
warpAffine()
   ↓
仿射变换结果
```
#### 透视变换
```
四个源点
   +
四个目标点
   ↓
getPerspectiveTransform()
   ↓
3×3 矩阵
   ↓
warpPerspective()
   ↓
透视变换结果
```
## 三、插值方法
几何变换后,目标图像的像素坐标不一定正好对应原图像的整数坐标,可能得到例如目标图像像素对应原图的像素位置(100.3,50.7),这时候需要进行插值估算像素颜色
常见插值方式

| 插值方式            | 特点           |
| --------------- | ------------ |
| `INTER_NEAREST` | 最近邻，速度快，质量较低 |
| `INTER_LINEAR`  | 双线性插值，常用     |
| `INTER_CUBIC`   | 三次插值，质量较好但较慢 |
| `INTER_AREA`    | 缩小时效果较好      |

## 四、核心要点
| 任务   | 常用函数                                                  | 矩阵/参数         |
| ---- | ----------------------------------------------------- | ------------- |
| 缩放   | `cv::resize()`                                        | 目标尺寸或缩放比例     |
| 平移   | `cv::warpAffine()`                                    | 手动创建 `2×3` 矩阵 |
| 旋转   | `cv::getRotationMatrix2D()` + `warpAffine()`          | `2×3`         |
| 三点仿射 | `cv::getAffineTransform()` + `warpAffine()`           | 三组对应点，`2×3`   |
| 四点透视 | `cv::getPerspectiveTransform()` + `warpPerspective()` | 四组对应点，`3×3`   |

---

## 五、本节小结

- **仿射变换**（`warpAffine`，`2×3` 矩阵）：线性变换 + 位移，保持直线与平行性，含平移、旋转、缩放、错切。旋转用 `getRotationMatrix2D`，三点定矩阵用 `getAffineTransform`。
- **透视变换**（`warpPerspective`，`3×3` 矩阵）：四组对应点，平行线可能不再平行，适合**文档 / 证件 / 票据矫正**；`getPerspectiveTransform` 求矩阵。
- **矩阵区别**：仿射 `2×3`（3 组点）、透视 `3×3`（4 组点）；透视可处理梯形矫正，仿射能力有限。
- **插值方法**：`INTER_NEAREST` 最快、`INTER_LINEAR` 常用、`INTER_CUBIC` 质量高、`INTER_AREA` 缩小时更优。
- 输出尺寸可用 `cv::norm` 计算源点间距离后取最大宽高自动确定。

---

## 相关链接

- 上一课：[[OpenCV第九课 霍夫变换(检测直线或圆)]]
- 下一课：（待添加）
- 关联课程：[[OpenCV第三课 图像几何操作]]（本课为其**进阶**：从 90° / 任意角度旋转、平移，扩展到三点仿射与四点透视矫正）
- 主题归纳：[[OpenCV学习笔记/主题模块/图像处理]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
