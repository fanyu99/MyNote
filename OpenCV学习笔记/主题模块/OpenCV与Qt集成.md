---
title: OpenCV 与 Qt 集成
type: 主题笔记
created: 2026-09-01
updated: 2026-09-01
tags:
  - opencv
  - 主题笔记
  - Qt
status: 进行中
---

# OpenCV 与 Qt 集成（主题模块）

> 归纳：OpenCV 与 Qt 结合时的格式转换与显示方案。

---

## 一、核心概念

### 红蓝互换问题

- OpenCV 读出的彩色图片通常是 **BGR**，而 Qt 常用 **RGB**。
- 直接传给 Qt 会发生**红蓝互换**错误。

### 解决思路

用 `cv::cvtColor` 将 BGR 转为 RGB，再构造 `QImage` 显示。

---

## 二、关键 API / 函数

```cpp
// BGR -> RGB
cv::Mat rgbImage;
cv::cvtColor(image, rgbImage, cv::COLOR_BGR2RGB);

// 构造 QImage
QImage qImage(
    rgbImage.data,
    rgbImage.cols,
    rgbImage.rows,
    static_cast<int>(rgbImage.step),
    QImage::Format_RGB888
);
```

---

## 三、易错点 / 注意事项

- 忘记转换 → 图像红蓝互换。
- `QImage` 构造需使用 RGB 格式与 `step` 步长。

---

## 四、引用自哪些课程笔记

- [[OpenCV第一课]]

---

## 五、相关主题

- [[OpenCV学习笔记/主题模块/图像基础与色彩空间]]
- [[OpenCV学习笔记/主题模块/cv_Mat核心]]
