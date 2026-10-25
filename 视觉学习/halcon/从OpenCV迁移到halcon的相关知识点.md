---
title: 从 OpenCV 迁移到 HALCON：基础入门
type: 课程笔记
课程: 1
created: 2026-10-18
updated: 2026-10-18
tags:
  - halcon
  - opencv
  - 机器视觉
  - 课程笔记
status: 学习中
---

# 从 OpenCV 迁移到 HALCON：基础入门

> **本课目标**：借助已有的 OpenCV 基础，快速建立 HALCON 的心智模型——搞清它的三个核心数据类型、坐标约定、语言语法，并跑通「读图 → 分割 → 筛选 → 测量 → 显示」的完整流程。
> **前置知识**：[[OpenCV第一课 基础入门|OpenCV 基础]]（图像、通道、阈值、轮廓、形态学）
> **参考索引**：[[OpenCV学习笔记/_MOC|OpenCV 知识库]] · [[视觉学习/_MOC|视觉学习总览]]

---

## 一、先建立认知：HALCON 和 OpenCV 不是同类东西

| 维度 | OpenCV | HALCON |
| --- | --- | --- |
| 定位 | 通用计算机视觉库 | 工业机器视觉平台 |
| 核心 | 函数库 + 你写流程 | 2000+ 算子 + HDevelop 快速原型 |
| 数据 | `cv::Mat` 一统天下 | **Image / Region / XLD** 三类分工 |
| 内存 | 手动/引用计数，需注意释放 | 自动管理，基本不用管 |
| 强项 | 生态大、免费、深度学习研究 | 形状匹配、亚像素测量、标定、工业鲁棒性 |
| 弱项 | 工业级匹配/标定要自己搭 | 商业授权、运行时 License 成本 |
| 开发方式 | 直接写 C++/Python | 先 HDevelop 拖算子验证，再导出代码 |

> [!tip] 一句话理解
> OpenCV 是「给你一堆积木」；HALCON 是「给你一套已经调好的工业流水线工具 + 一个可视化调试台」。
> 学 HALCON 的重点不是学图像原理，而是**学它把问题拆成哪几类对象、用哪些算子组合**。

---

## 二、三个核心数据类型（最重要的思维转变）

OpenCV 里，图像、掩膜、轮廓本质上都是 `Mat`。HALCON 把它们**严格分开**：

| 类型          | 是什么                      | OpenCV 里对应什么                 | 典型算子                                                     |
| ----------- | ------------------------ | ---------------------------- | -------------------------------------------------------- |
| **Image**   | 像素矩阵（可多通道）               | `cv::Mat`                    | `read_image`、`rgb1_to_gray`、`gauss_filter`               |
| **Region**  | 一组像素的集合（游程编码），**像素级**    | 二值掩膜 `mask` / 连通域            | `threshold`、`connection`、`select_shape`、`opening_circle` |
| **XLD**     | **亚像素轮廓**/多边形，**坐标是浮点数** | `findContours` 的点集（但精度更高）    | `edges_sub_pix`、`fit_circle_contour_xld`                 |
| **Control** | 数字/字符串/tuple，即普通变量       | `int`、`double`、`std::vector` | `area_center`、`tuple_*`                                  |

### 关键区别：Region ≠ 轮廓

- **Region** 是「哪些像素属于目标」，适合做面积、重心、形态学、Blob 计数。
- **XLD** 是「边界在哪」，坐标可以是 `123.47` 这种小数，适合做**亚像素测量**、圆/直线拟合。
- OpenCV 的 `findContours` 同时承担了这两件事；HALCON 把它们拆开了：
  - Region → 轮廓：`gen_contour_region_xld`
  - 轮廓 → Region：`gen_region_contour_xld`

```halcon
* Region 转 XLD 轮廓
gen_contour_region_xld (Region, Contours, 'border')

* XLD 轮廓转 Region
gen_region_contour_xld (Contours, RegionFromXLD, 'filled')
```

> [!warning] 常见误区
> 不要拿 `threshold` 的结果直接当「一个目标」。`threshold` 返回的是**所有满足条件的像素组成的一个 Region**；要拆成一个个独立目标，必须再调用 `connection`。这一点和 OpenCV 的 `connectedComponents` 类似，但 HALCON 里是显式两步。

---

## 三、坐标系统：Row / Column，不是 x / y

这是从 OpenCV 迁移过来**最容易错**的地方。

|              | OpenCV   | HALCON                     |
| ------------ | -------- | -------------------------- |
| 坐标顺序         | `(x, y)` | `(Row, Column)` = `(y, x)` |
| 原点           | 左上角      | 左上角                        |
| Row 方向       | 向下       | 向下                         |
| Column 方向    | 向右       | 向右                         |
| 像素 (x,y) 的含义 | 像素左上角    | **像素中心**                   |

### 两个直接后果

1. **函数参数顺序反过来**：OpenCV 写 `cv2.circle(img, (x, y), r, ...)`，HALCON 写 `gen_circle(Circle, Row, Column, Radius)`。
2. **亚像素坐标差 0.5**：HALCON 的整数坐标是像素中心，OpenCV 的整数坐标是像素左上角。两边做亚像素结果对齐时，经验规则是 **HALCON 坐标 ≈ OpenCV 坐标 + 0.5**。

```halcon
* 错误直觉：以为第一个参数是列
gen_circle (Circle, 100, 200, 50)   * Row=100, Column=200

* 所有几何算子统一是 Row 在前、Column 在后
gen_rectangle1 (Rectangle, Row1, Column1, Row2, Column2)
area_center (Region, Area, Row, Column)
smallest_rectangle2 (Region, Row, Column, Phi, Length1, Length2)
```

> [!tip] 记忆法
> HALCON 里看到 `Row` 就想「行 = y」，看到 `Column` 就想「列 = x」。凡是成对出现，**永远是 Row 在前**。

---

## 四、HDevelop 入门

HALCON 的 IDE 叫 **HDevelop**，学习阶段 90% 的时间都在这里。

### 界面四块

| 区域 | 作用 |
| --- | --- |
| 程序窗口 | 写算子代码，逐行执行 |
| 变量窗口 | 实时查看 Iconic（Image/Region/XLD）和 Control（数值）变量 |
| 图形窗口 | 显示图像和结果，可缩放、取坐标 |
| 算子窗口 | 查算子、看参数、生成代码（F1 查帮助） |

### 运行快捷键

| 按键 | 作用 |
| --- | --- |
| `F5` | 从头运行整个程序 |
| `F6` | 单步执行（最常用，配合变量窗口看每一步结果） |
| `F1` | 查看当前算子帮助 |
| `F2` | 设置/取消断点 |

> [!tip] 学习方式
> 初学不要一次性 F5 跑完。用 **F6 单步 + 变量窗口**，每一步都看一眼 Region 长什么样、tuple 里是什么值，这比看十遍文档都有效。

### 第一个程序：读图、显示、取信息

```halcon
* HALCON 注释以星号开头
* 读入 HALCON 自带示例图
read_image (Image, 'particle')

* 获取尺寸（注意输出是 Width, Height）
get_image_size (Image, Width, Height)

* 开一个和图像一样大的窗口
dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
dev_display (Image)

* 判断通道数，彩色图转灰度
count_channels (Image, Channels)
if (Channels == 3)
    rgb1_to_gray (Image, GrayImage)
else
    copy_image (Image, GrayImage)
endif

* 在窗口里写字
disp_message (WindowHandle, 'Size: ' + Width + ' x ' + Height, 'window', 12, 12, 'black', 'true')

* 结束时关闭窗口（HDevelop 一般不需要手动释放图像）
dev_close_window ()
```

### 图像尺寸顺序也不同

```halcon
get_image_size (Image, Width, Height)   * HALCON 先 Width 后 Height
```
OpenCV 的 `img.shape` 是 `(height, width, channels)`；HALCON 是 `Width, Height`。写 ROI 时别搞混。

---

## 五、HALCON 语言基础

### 1. 赋值用 `:=`，比较用 `=`

```halcon
A := 10          * 赋值
if (A = 10)      * 比较
    ...
endif
```

### 2. 没有变量声明，没有显式类型

```halcon
Row := 100.5          * 自动是 real
Name := 'particle'    * 自动是 string
Region := ...         * iconic 变量
```

### 3. Tuple：HALCON 的「数组」

```halcon
T := [1, 2, 3]
Len := |T|            * 长度 = 3
First := T[0]         * 索引从 0 开始
T := [T, 4]           * 追加后变成 [1,2,3,4]

* 一个算子返回多个值，输出参数放在最后
area_center (Region, Area, Row, Column)
* Area / Row / Column 都是 tuple；如果是多个区域，每个区域一个元素
```

> [!warning] 对象索引的坑
> **Control tuple 索引从 0 开始**（`T[0]`），但 `select_obj` / `copy_obj` 这类算子选择**对象**时，`Index` 从 **1** 开始。
> 这是 HALCON 的经典不一致，写循环时特别容易越界。

### 4. 控制流

```halcon
if (Score > 0.8)
    * ...
else
    * ...
endif

for Index := 0 to Number - 1 by 1
    * ...
endfor

while (Found = 0)
    * ...
endwhile

switch (Type)
    case 1
        * ...
    case 2
        * ...
    default
        * ...
endswitch
```

### 5. 算子签名怎么读

HALCON 文档里的签名长这样：

```text
area_center (Region : : : Area, Row, Column)
```

规则是：

```text
算子名 (输入图标 : 输入控制 : 输出图标 : 输出控制)
```

- **图标参数**：Image / Region / XLD
- **控制参数**：数字、字符串、tuple
- 调用时**不需要写冒号**，按顺序传参即可：

```halcon
area_center (Region, Area, Row, Column)
```

### 6. `dev_*` 算子只管显示

`dev_open_window`、`dev_display`、`dev_set_color`、`dev_set_draw` 都只影响图形窗口，不改变数据。调试时随便用，正式代码里可以精简。

---

## 六、OpenCV → HALCON 算子对照表

> 这张表建议收藏。左边是你已经会的，右边是要查的。

### 图像读写与显示

| 用途 | OpenCV | HALCON |
| --- | --- | --- |
| 读图 | `cv2.imread` | `read_image` |
| 存图 | `cv2.imwrite` | `write_image` |
| 显示 | `cv2.imshow` | `dev_display` |
| 开窗口 | `cv2.namedWindow` | `dev_open_window` |
| 获取尺寸 | `img.shape` | `get_image_size` |
| 通道数 | `img.shape[2]` | `count_channels` |
| 通道拆分 | `cv2.split` | `decompose3` / `access_channel` |
| 通道合并 | `cv2.merge` | `compose3` / `compose4` |
| 灰度化 | `cv2.cvtColor(BGR2GRAY)` | `rgb1_to_gray` |
| 裁剪 / ROI | `img[y1:y2, x1:x2]` | `gen_rectangle1` + `reduce_domain` |
| 等待按键 | `cv2.waitKey` | 不需要；HDevelop 自动刷新，可用 `stop()` |

> [!note] 颜色顺序
> OpenCV 是 **BGR**；HALCON 从文件读入的彩色图通道顺序一般是 **RGB**（`decompose3` 得到的就是 R、G、B）。
> 从工业相机取图时可能是 BGR，需要 `compose3` 重排或直接确认相机参数。**不要在 HALCON 里默认 BGR。**

### 分割与区域处理

| 用途 | OpenCV | HALCON |
| --- | --- | --- |
| 固定阈值 | `cv2.threshold` | `threshold` |
| 自动阈值 | `cv2.threshold(OTSU)` | `bin_threshold` |
| 自适应/动态阈值 | `cv2.adaptiveThreshold` | `dyn_threshold` |
| 连通域 | `cv2.connectedComponents` | `connection` |
| 按特征筛选 | 手写遍历 | `select_shape` |
| 区域合并 | `bitwise_or` | `union1` / `union2` |
| 区域求交 | `bitwise_and` | `intersection` |
| 区域求差 | `bitwise_not` + and | `difference` |
| 补集 | `bitwise_not` | `complement` |
| 填洞 | 手写 flood fill | `fill_up` |
| 腐蚀/膨胀 | `cv2.erode/dilate` | `erosion_circle` / `dilation_circle` |
| 开/闭运算 | `cv2.morphologyEx` | `opening_circle` / `closing_rectangle1` |
| 面积/重心 | `cv2.contourArea` / moments | `area_center` |
| 外接正矩形 | `cv2.boundingRect` | `smallest_rectangle1` |
| 最小外接旋转矩形 | `cv2.minAreaRect` | `smallest_rectangle2` |
| 最小外接圆 | `cv2.minEnclosingCircle` | `smallest_circle` |

> [!warning] `smallest_rectangle2` 返回的是**半边长**
> OpenCV 的 `minAreaRect` 返回 `(w, h)` 全宽全高；HALCON 的 `smallest_rectangle2` 返回 `Length1, Length2`，是**半宽半高**。换算：`width = 2 * Length1`。

### 边缘、轮廓与测量

| 用途 | OpenCV | HALCON |
| --- | --- | --- |
| Canny 边缘 | `cv2.Canny` | `edges_sub_pix (…, 'canny', …)` |
| 亚像素轮廓 | 无直接对应 | `edges_sub_pix`、`lines_gauss` |
| 轮廓筛选 | 手写 | `select_contours_xld` |
| 轮廓长度 | `cv2.arcLength` | `length_xld` |
| 圆拟合 | `cv2.minEnclosingCircle` / 手写 | `fit_circle_contour_xld` |
| 直线拟合 | `cv2.fitLine` | `fit_line_contour_xld` |
| 霍夫直线 | `cv2.HoughLinesP` | `hough_lines` / `lines_gauss` |
| 霍夫圆 | `cv2.HoughCircles` | `hough_circles`（区域）/ `fit_circle_contour_xld`（拟合） |

### 匹配、变换与标定

| 用途 | OpenCV | HALCON |
| --- | --- | --- |
| 灰度模板匹配 | `cv2.matchTemplate` | `create_ncc_model` + `find_ncc_model` |
| 形状匹配（旋转/缩放/遮挡） | 无直接对应 | `create_shape_model` + `find_shape_model` |
| 仿射变换 | `cv2.warpAffine` | `affine_trans_image` / `affine_trans_region` |
| 单应矩阵 | `cv2.findHomography` | `vector_to_hom_mat2d` + `hom_mat2d_*` |
| 相机标定 | `cv2.calibrateCamera` | `calibrate_cameras` |
| 手眼标定 | 手写 | `calibrate_hand_eye` |

> [!tip] HALCON 的杀手锏
> `create_shape_model` / `find_shape_model` 是 HALCON 最值钱的部分：**带旋转、缩放、部分遮挡的目标定位**，工业上大量场景靠它。
> OpenCV 的 `matchTemplate` 只能做平移匹配，旋转缩放要自己实现，这就是两者最大的体验差距之一。

---

## 七、实战 1：Blob 分析（颗粒计数）

对应 OpenCV 的「阈值 → 连通域 → 轮廓 → 统计」。

```halcon
* 1. 读图并转灰度
read_image (Image, 'particle')
rgb1_to_gray (Image, GrayImage)

* 2. 固定阈值：得到一个包含所有目标像素的 Region
threshold (GrayImage, Regions, 0, 128)

* 3. 拆分连通域：每个颗粒变成一个独立对象
connection (Regions, ConnectedRegions)

* 4. 按面积筛选（去掉噪点和粘连的大块）
select_shape (ConnectedRegions, SelectedRegions, 'area', 'and', 100, 99999)

* 5. 统计数量与重心
count_obj (SelectedRegions, Number)
area_center (SelectedRegions, Area, Row, Column)

* 6. 显示
dev_display (Image)
dev_set_draw ('margin')        * 'fill' 填充 / 'margin' 只画边
dev_set_color ('red')
dev_set_line_width (2)
dev_display (SelectedRegions)

* 7. 逐个标注序号
for Index := 0 to Number - 1 by 1
    disp_message (WindowHandle, '#' + (Index + 1), 'image', Row[Index] - 20, Column[Index], 'green', 'true')
endfor
```

### 逐行对照 OpenCV

```python
# OpenCV 等价流程
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
_, binary = cv2.threshold(gray, 128, 255, cv2.THRESH_BINARY_INV)
num, labels, stats, centroids = cv2.connectedComponentsWithStats(binary)
for i in range(1, num):
    if 100 <= stats[i, cv2.CC_STAT_AREA] <= 99999:
        ...
```

> [!tip] 选择阈值的方法
> 先用 `min_max_gray` 看灰度范围，或用 `bin_threshold` 让 HALCON 自动按直方图选阈值：
> ```halcon
> bin_threshold (GrayImage, Region)
> ```
> 效果不好再换 `threshold` 手动调、或 `dyn_threshold` 做光照不均场景。

---

## 八、实战 2：亚像素圆测量

这是 HALCON 明显强于 OpenCV 的地方：边缘定位到亚像素，拟合半径精度高。

```halcon
read_image (Image, 'ring')
rgb1_to_gray (Image, GrayImage)

* 1. 亚像素边缘提取，得到 XLD
edges_sub_pix (GrayImage, Edges, 'canny', 1.5, 20, 40)

* 2. 只要足够长的轮廓
select_contours_xld (Edges, SelectedEdges, 'contour_length', 50, 99999, -0.5, 0.5)

* 3. 圆拟合
fit_circle_contour_xld (SelectedEdges, 'algebraic', -1, 0, 0, 3, 2, Row, Column, Radius, StartPhi, EndPhi, PointOrder)

* 4. 把拟合结果画出来
gen_circle_contour_xld (ContCircle, Row, Column, Radius, 0, 6.28318, 'positive', 1.5)
dev_display (Image)
dev_set_color ('green')
dev_set_line_width (2)
dev_display (ContCircle)
disp_message (WindowHandle, 'Radius: ' + Radius, 'window', 12, 12, 'black', 'true')
```

要点：

- `edges_sub_pix` 的输出是 **XLD**，不是 Region，坐标带小数。
- `fit_circle_contour_xld` 的参数里，`'algebraic'` 是拟合算法；后面的 `-1, 0, 0, 3, 2` 依次是最大点数、闭合距离、端点裁剪、迭代次数、裁剪系数。初学照抄这组常用值即可。
- `Radius` 是**整个半径**，不是直径也不是半边长。

---

## 九、实战 3：形状匹配（工业定位核心）

```halcon
* ===== 1. 创建模板（用一张清晰的模板图）=====
read_image (Template, 'pen')

* 如果要限定模板区域，可以用 reduce_domain
* gen_rectangle1 (ROI, 80, 100, 400, 500)
* reduce_domain (Template, ROI, Template)

create_shape_model (Template, 'auto', rad(-15), rad(30), 'auto', 'auto', 'use_polarity', 'auto', 'auto', ModelID)

* ===== 2. 在目标图中查找 =====
read_image (Image, 'pen_parts')
find_shape_model (Image, ModelID, rad(-15), rad(30), 0.7, 5, 0.5, 'least_squares', 0, 0.8, Row, Column, Angle, Score)

* ===== 3. 把匹配到的位置画出来 =====
get_shape_model_contours (ModelContours, ModelID, 1)
dev_display (Image)
dev_set_color ('green')
dev_set_line_width (2)

for Index := 0 to |Score| - 1 by 1
    * 把模板轮廓从原点变换到匹配位置
    vector_angle_to_rigid (0, 0, 0, Row[Index], Column[Index], Angle[Index], HomMat2D)
    affine_trans_contour_xld (ModelContours, ContoursTrans, HomMat2D)
    dev_display (ContoursTrans)
    disp_message (WindowHandle, 'Score: ' + Score[Index], 'image', Row[Index] - 40, Column[Index], 'green', 'true')
endfor

* 释放模型（HALCON 中模型句柄需要显式清理）
clear_shape_model (ModelID)
```

关键参数：

| 参数 | 含义 | 初学建议 |
| --- | --- | --- |
| `AngleStart / AngleExtent` | 允许的旋转范围 | 用 `rad()` 转成弧度 |
| `MinScore` | 最低得分（0~1） | 0.6~0.8 起调 |
| `NumMatches` | 最多找几个 | 0 = 全部 |
| `MaxOverlap` | 允许的重叠比例 | 0.5 常用 |
| `SubPixel` | 亚像素精度 | `'least_squares'` 或 `'interpolation'` |
| `Greediness` | 搜索贪婪度 | 0.8~0.9；漏检就降低 |

> [!warning] 形状匹配的适用边界
> `create_shape_model` 依赖**边缘结构**，适合有稳定轮廓的零件；如果目标纹理丰富但没有明显边缘，用 `create_ncc_model`（灰度相关）。两者都不是深度学习，遮挡太多、形变太大时仍然会失效。

---

## 十、从 HDevelop 到实际代码

HDevelop 里跑通后，可以导出成 C++ / C# / Python：

- HDevelop 菜单：**文件 → 导出程序**，选择目标语言。
- 核心类型映射：

| HDevelop | C++ | C# | Python |
| --- | --- | --- | --- |
| `Image` / `Region` / `XLD` | `HObject` | `HObject` | `HObject` |
| 数值/字符串/tuple | `HTuple` | `HTuple` | `HTuple` |
| 算子 `read_image` | `ReadImage(...)` | `HOperatorSet.ReadImage(...)` | `ha.read_image(...)` |

C++ 示例：

```cpp
#include "HalconCpp.h"
using namespace HalconCpp;

HObject image;
ReadImage(&image, "particle");

HTuple width, height;
GetImageSize(image, &width, &height);

HObject region;
Threshold(image, &region, 0, 128);
```

> [!tip] 工程化建议
> 不要手写算子封装。**先在 HDevelop 调通参数，再导出代码**，然后把导出的过程封装成函数。HALCON 的参数调优高度依赖图形反馈，直接在 C++ 里盲调会非常痛苦。

---

## 十一、常见坑清单

| # | 坑 | 正确做法 |
| --- | --- | --- |
| 1 | 把坐标写成 `(x, y)` | HALCON 是 `(Row, Column)` |
| 2 | 以为 `threshold` 直接分出多个目标 | 先 `threshold` 再 `connection` |
| 3 | `dev_display` 看不到 Region 边界 | `dev_set_draw ('margin')` |
| 4 | 彩色图当成 BGR 处理 | HALCON 文件图通常是 RGB |
| 5 | 用 `select_obj` 时索引从 0 开始 | 对象索引从 **1** 开始 |
| 6 | 赋值写成 `=` | 赋值是 `:=`，比较才是 `=` |
| 7 | 角度直接填度数 | 用 `rad()` / `deg()` 转换 |
| 8 | 把 `smallest_rectangle2` 的 `Length1` 当全宽 | 它是**半边**长，宽度 = `2*Length1` |
| 9 | 循环里反复 `dev_display` 导致很卡 | 程序开头 `dev_update_off()`，结尾 `dev_update_on()` |
| 10 | 换了图片路径就报错 | `read_image` 的相对路径基于 HDevelop 当前目录，建议用绝对路径或统一资源目录 |
| 11 | 模型用完不清理 | `clear_shape_model` / `clear_ncc_model` |
| 12 | 在 C++ 里还在查 HDevelop 的算子名 | 导出代码时大小写和返回值会变，以导出结果为准 |

### 调试三板斧

```halcon
* 1. 看区域长什么样
dev_display (Region)

* 2. 看数值对不对
* 变量窗口直接看，或
disp_message (WindowHandle, 'Number: ' + Number, 'window', 12, 12, 'black', 'true')

* 3. 卡在中间就停住
stop ()
```

---

## 十二、7 天入门计划

| 天 | 主题 | 练习 |
| --- | --- | --- |
| Day 1 | HDevelop 界面、`read_image` / `dev_display`、语法 | 读 3 张图，打印尺寸、通道数，用 `dev_set_part` 放大看局部 |
| Day 2 | Image / Region / XLD / tuple | 手工 `threshold` 一块区域，观察变量窗口里 Region 和 tuple 的区别 |
| Day 3 | Blob 分析 | 颗粒计数，并画出每个目标的序号 |
| Day 4 | 形态学 + `select_shape` | 对同一张图做开/闭运算，比较计数变化；用 `'circularity'`、`'rectangularity'` 筛选 |
| Day 5 | 亚像素 XLD + 拟合 | 测圆孔半径、拟合一条边，和 OpenCV 结果对比误差 |
| Day 6 | 形状匹配 | 用 `'pen'` 示例做旋转定位，调 `MinScore` 和 `Greediness` 看效果 |
| Day 7 | 小项目 | 任选：零件计数 / 缺陷面积统计 / 尺寸测量，输出一份带标注的结果图 |

### 建议的练习顺序

1. **先复现，再改参数**：每个例子先原样跑通，再改阈值、改面积范围、改角度范围，观察结果变化。
2. **每学一个算子，问自己「OpenCV 里对应什么」**：有对照就不会觉得 HALCON 陌生。
3. **刻意练习 Row/Column**：写几何相关代码时先默念「Row 是 y」。

---

## 十三、下一步学什么

基础打通后，按工业项目出现的频率排序：

1. **Blob 进阶**：`dyn_threshold`、`local_threshold`、`watersheds`，处理光照不均和粘连目标
2. **XLD 测量**：`fit_line_contour_xld`、`fit_circle_contour_xld`、`gen_measure_rectangle2` 一维测量
3. **形状匹配进阶**：缩放、金字塔层数、`find_scaled_shape_model`、`find_aniso_shape_model`
4. **标定**：`calibrate_cameras`、畸变校正、手眼标定 `calibrate_hand_eye`
5. **HALCON + C# / C++ 工程集成**：HDevEngine、相机采集、与 PLC/上位机通信
6. **HALCON 深度学习**：分类、检测、分割（DLM），和 YOLO 做对比

> [!tip] 和已有知识的连接
> 你学过 [[YOLO学习/_MOC|YOLO 目标检测]]，可以把 HALCON 的形状匹配和深度学习方法放在一起比较：**传统形状匹配 = 可解释、样本少、速度快、但怕形变；深度学习 = 鲁棒、泛化好、但要数据、要算力、可解释性弱**。工业选型时这条分界线非常实用。

---

## 附：本课速查卡

```halcon
* 读显
read_image (Image, 'xxx')
get_image_size (Image, Width, Height)
dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
dev_display (Image)

* 灰度
rgb1_to_gray (Image, GrayImage)

* ROI
gen_rectangle1 (ROI, Row1, Column1, Row2, Column2)
reduce_domain (Image, ROI, ImageROI)

* 分割
threshold (GrayImage, Region, MinGray, MaxGray)
connection (Region, ConnectedRegions)

* 筛选
select_shape (ConnectedRegions, Selected, 'area', 'and', MinArea, MaxArea)

* 测量
area_center (Selected, Area, Row, Column)
smallest_rectangle2 (Selected, Row, Column, Phi, Length1, Length2)

* 亚像素
edges_sub_pix (GrayImage, Edges, 'canny', 1.5, 20, 40)
fit_circle_contour_xld (Edges, 'algebraic', -1, 0, 0, 3, 2, Row, Column, Radius, StartPhi, EndPhi, PointOrder)

* 匹配
create_shape_model (Template, 'auto', rad(-15), rad(30), 'auto', 'auto', 'use_polarity', 'auto', 'auto', ModelID)
find_shape_model (Image, ModelID, rad(-15), rad(30), 0.7, 5, 0.5, 'least_squares', 0, 0.8, Row, Column, Angle, Score)
```

---

*创建于 2026-10-18。后续可按「Blob 进阶 / XLD 测量 / 形状匹配 / 标定」拆成独立课程笔记。*
