---
title: OpenCV 第三课 图像几何操作
type: 课程笔记
课程: 3
created: 2026-09-02
updated: 2026-09-02
tags:
  - opencv
  - 课程笔记
  - 几何变换
status: 学习中
---

# OpenCV第三课 图像几何操作

> **本课目标**：掌握 OpenCV 中图像的**几何操作**——缩放、保持比例缩放、裁剪（ROI）、翻转、旋转（90° 与任意角度）与平移。理解图像坐标体系与 `cv::Size` 的宽高顺序，并能在 C++/Python 中熟练运用。
> **涉及主题**：[[OpenCV学习笔记/主题模块/图像处理]] · [[OpenCV学习笔记/主题模块/cv_Mat核心]]

---

## 一、图像的宽高与坐标

### 概念

#### 图像坐标模型

OpenCV 中的图像坐标 **x 右正、y 下正**（与 Qt 相同，都是常见的图像坐标体系），原点在图像**左上角**。

```
(0,0) ────→ x（宽度方向）
  │
  ↓
  y（高度方向）
```

> ⚠️ 访问像素 / 定义矩形时，坐标书写要格外小心，详见下方「核心要点」。

#### cv::Size(width, height)

OpenCV 中表示**大小**的常用数据类型。

- 参数顺序为 **宽、高**（width, height），**容易写反，务必注意**！
- 与 `image.cols`（宽）/ `image.rows`（高）对应。

#### cv::resize() —— 缩放

完整函数形式：

```cpp
cv::resize(
    src,             // 原图
    dst,             // 输出图
    dsize,           // 目标尺寸 (width, height)
    fx,              // x 方向缩放比例
    fy,              // y 方向缩放比例
    interpolation    // 插值算法
);
```

代码示例：

```cpp
// 1. 缩放为指定尺寸
cv::Mat result;
cv::resize(image, result, cv::Size(800, 600));

// 2. 按指定比例缩放
cv::Mat result;
cv::resize(
	image, 
	result, 
	cv::Size(),
	0.5,
	0.5,
	cv::INTER_AREA
);
```

### 核心要点

1. **`dsize` 与 `(fx, fy)` 二选一**：指定了 `dsize` 时，`fx / fy` 传 `0`（或不填）；按比例缩放时，`dsize` 传 `cv::Size()`。
2. **常见插值算法**：

```cpp
cv::INTER_NEAREST // 最近邻，速度快、质量较差
cv::INTER_LINEAR  // 双线性，默认值
cv::INTER_CUBIC   // 双三次，质量好但速度慢
cv::INTER_AREA    // 缩小图像时效果较好
```

3. **选型建议**：
   - **缩小**图像 → `cv::INTER_AREA`
   - **放大**图像 → `cv::INTER_LINEAR` 或 `cv::INTER_CUBIC`

---

## 二、保持宽高比例缩放

### 概念

如果只指定宽度，按**原图宽高比**计算出对应高度，即可保证缩放后不变形。

### 核心要点

**比例公式**（等比）：

```
目标高度 : 目标宽度 = 原高度 : 原宽度
targetHeight = targetWidth * image.rows / image.cols
```

```cpp
int targetWidth = 800;
int targetHeight = static_cast<int>(
    image.rows * static_cast<double>(targetWidth) / image.cols
);
cv::Mat resized;
cv::resize(image, resized, cv::Size(targetWidth, targetHeight));
```

> ⚠️ 注意计算顺序：先转 `double` 再乘，避免整数除法导致精度丢失（即 `static_cast<double>(targetWidth)` 要放在乘法前）。

---

## 三、裁剪：ROI

### cv::Rect

```cpp
cv::Rect(0, 0, 100, 100); // 从左上角 (0,0) 截取 100*100 的矩形
// 参数顺序：x, y, width, height
```

```cpp
cv::Rect rect(0, 0, 100, 100);
cv::Mat roi = image(rect);        // 裁剪出感兴趣区域,共享内存
cv::Mat roiCopy = image(rect).clone(); // 需要独立数据时深拷贝
```

### 重要

1. **ROI 默认是浅拷贝**（共享底层内存）！若需独立数据，使用 `.clone()` 或 `.copyTo()`。
2. **ROI 大小不能超过图像本身范围**，否则越界报错或产生未定义行为。

---

## 四、翻转：cv::flip

```cpp
// 函数原型
cv::flip(src, dst, flipCode);
```

`flipCode` 的取值：

| flipCode | 效果 |
| --- | --- |
| `1` | 左右翻转（水平镜像） |
| `0` | 上下翻转（垂直镜像） |
| `-1` | 上下 + 左右翻转（180°） |

---

## 五、旋转 90°：cv::rotate

```cpp
cv::rotate(src, dst, cv::ROTATE_90_CLOCKWISE);
```

常用参数：

```cpp
cv::ROTATE_90_CLOCKWISE        // 顺时针 90°
cv::ROTATE_180                 // 旋转 180°
cv::ROTATE_90_COUNTERCLOCKWISE // 逆时针 90°
```

> ⚠️ 注意：旋转 90° 时，图像的**宽和高会互换**！

---

## 六、任意角度旋转与平移：warpAffine

90° 旋转有专用函数 `cv::rotate`，但**任意角度**旋转（以及平移、缩放组合）需要通过**仿射变换**完成。

### 任意角度旋转

```cpp
// 1. 获取旋转中心：图像中心
cv::Point2f center(image.cols / 2.0F, image.rows / 2.0F);

// 2. 获取旋转矩阵
cv::Mat rotationMatrix = cv::getRotationMatrix2D(
    center,
    30.0,  // 逆时针 30°（正值逆时针）
    1.0    // 缩放比例
);

// 3. 仿射变换
cv::Mat rotated30;
cv::warpAffine(image, rotated30, rotationMatrix, image.size());
// 尺寸保持和 image 一致，超出部分会被裁切

cv::imshow("逆时针旋转 30 度", rotated30);
```

### 平移

用仿射变换矩阵实现平移，其中 `tx / ty` 为 x / y 方向的平移像素数：

```cpp
// 创建矩阵: 
[1,0,tx] // x1 = x*1 + y*0 + tx
[0,1,ty] // y1 = x*0 + y*1 + ty
// 以原比例进行平移:
cv::Mat M = (cv::Mat_<double>(2, 3) <<
    1, 0, tx,
    0, 1, ty);

cv::Mat translated;
cv::warpAffine(image, translated, M, image.size());
```

### 核心要点

- `cv::getRotationMatrix2D(center, angle, scale)` 生成 **2×3 仿射矩阵**，`angle` 正值表示**逆时针**。
- `cv::warpAffine(src, dst, M, dsize)` 用该矩阵做仿射变换。
- 旋转/平移后的图若保持原尺寸，边缘会被裁切；如需完整显示，需按旋转后的包围盒**调整 `dsize`**（可用 `cv::boundingRect` 计算）。

---

## 七、API 对照

| 功能 | C++ | Python |
| --- | --- | --- |
| 缩放 | `cv::resize` | `cv2.resize` |
| 裁剪（ROI） | `image(rect)` | `image[y1:y2, x1:x2]` |
| 深拷贝裁剪 | `image(rect).clone()` | `image[...].copy()` |
| 翻转 | `cv::flip` | `cv2.flip` |
| 旋转 90° | `cv::rotate` | `cv2.rotate` |
| 任意角度旋转 | `getRotationMatrix2D` + `warpAffine` | `cv2.getRotationMatrix2D` + `cv2.warpAffine` |
| 平移 | `warpAffine` | `cv2.warpAffine` |
| 宽度 | `image.cols` | `image.shape[1]` |
| 高度 | `image.rows` | `image.shape[0]` |

---

## 八、代码示例

### C++ 示例：完整演示几何操作

```cpp
#include <opencv2/opencv.hpp>

int main()
{
    cv::Mat image = cv::imread("test.jpg");
    if (image.empty()) {
        std::cerr << "读取图片失败" << std::endl;
        return -1;
    }

    // 1. 缩放为指定尺寸
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(400, 300));

    // 2. 保持宽高比缩放
    int targetWidth = 400;
    int targetHeight = static_cast<int>(
        image.rows * static_cast<double>(targetWidth) / image.cols
    );
    cv::resize(image, resized, cv::Size(targetWidth, targetHeight));

    // 3. 裁剪 ROI（默认浅拷贝）
    cv::Mat roi = image(cv::Rect(50, 50, 200, 200));
    cv::Mat roiCopy = image(cv::Rect(50, 50, 200, 200)).clone();

    // 4. 翻转
    cv::Mat flipped;
    cv::flip(image, flipped, 1); // 左右翻转

    // 5. 旋转 90°
    cv::Mat rotated;
    cv::rotate(image, rotated, cv::ROTATE_90_CLOCKWISE);

    // 6. 任意角度旋转
    cv::Point2f center(image.cols / 2.0F, image.rows / 2.0F);
    cv::Mat M = cv::getRotationMatrix2D(center, 30.0, 1.0);
    cv::Mat rotated30;
    cv::warpAffine(image, rotated30, M, image.size());

    cv::imshow("原图", image);
    cv::imshow("翻转", flipped);
    cv::waitKey(0);
    return 0;
}
```

### Python 示例

```python
import cv2
import numpy as np

image = cv2.imread("test.jpg")
if image is None:
    print("读取图片失败")
    exit()

# 1. 缩放为指定尺寸（注意是 (宽, 高)）
resized = cv2.resize(image, (400, 300))

# 2. 保持宽高比缩放
target_width = 400
target_height = int(image.shape[0] * target_width / image.shape[1])
resized = cv2.resize(image, (target_width, target_height))

# 3. 裁剪 ROI（numpy 切片默认是浅拷贝/视图，需独立时用 .copy()）
h, w = image.shape[:2]
roi = image[50:250, 50:250]          # [y1:y2, x1:x2]
roi_copy = image[50:250, 50:250].copy()

# 4. 翻转
flipped = cv2.flip(image, 1)         # 1 左右，0 上下，-1 上下左右

# 5. 旋转 90°
rotated = cv2.rotate(image, cv2.ROTATE_90_CLOCKWISE)

# 6. 任意角度旋转
center = (w // 2, h // 2)
M = cv2.getRotationMatrix2D(center, 30, 1.0)   # 正值逆时针
rotated30 = cv2.warpAffine(image, M, (w, h))

cv2.imshow("原图", image)
cv2.imshow("翻转", flipped)
cv2.waitKey(0)
cv2.destroyAllWindows()
```

---

## 九、本节小结

- 图像坐标 **x 右正、y 下正**，原点在左上角；`cv::Size` 参数顺序是**宽、高**，极易写反。
- `cv::resize` 中 `dsize` 与 `(fx, fy)` **二选一**；缩小用 `INTER_AREA`，放大用 `INTER_LINEAR` / `INTER_CUBIC`。
- 保持比例缩放：`targetHeight = targetWidth * rows / cols`，注意先转浮点再乘。
- ROI 默认**浅拷贝**，独立数据需 `.clone()`；且 ROI 不能超出图像范围。
- `cv::flip`：`1` 左右、`0` 上下、`-1` 上下左右。
- `cv::rotate` 只支持 90° 整数倍，90° 旋转会**互换宽高**。
- 任意角度旋转与平移用 `getRotationMatrix2D` + `warpAffine`，`angle` 正值逆时针。

---

## 相关链接

- 上一课：[[OpenCV第二课 cv_Mat模型]]
- 下一课：[[OpenCV第四课 阈值处理和二值化]]
- 主题归纳：[[OpenCV学习笔记/主题模块/图像处理]] · [[OpenCV学习笔记/主题模块/cv_Mat核心]]
- 知识库总览：[[OpenCV学习笔记/_MOC]]
