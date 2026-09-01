## 设备坐标系(x右正,y下正)

## 常见API
| 方法                  | 含义                                                |
| ------------------- | ------------------------------------------------- |
| `x(), y()`          | 控件左上角相对于其**父控件**的坐标（不包括父控件的边框）                    |
| `pos()`             | 返回 `QPoint(x, y)`                                 |
| `width(), height()` | 控件的宽度和高度（不包括任何边框）                                 |
| `size()`            | 返回 `QSize(width, height)`                         |
| `geometry()`        | 返回 `QRect(x, y, width, height)`，即**相对于父控件**的位置和大小 |
| `frameGeometry()`   | 返回包含窗口边框的矩形（对于顶级窗口，包含标题栏、边框）                      |
## 常见的坐标转换函数

| 函数                                         | 功能                   |
| ------------------------------------------ | -------------------- |
| `mapToGlobal(const QPoint &localPoint)`    | 将本控件内的局部坐标转换为全局屏幕坐标  |
| `mapFromGlobal(const QPoint &globalPoint)` | 将全局屏幕坐标转换为本控件内的局部坐标  |
| `mapToParent(const QPoint &localPoint)`    | 将本控件内的局部坐标转换为父控件内的坐标 |
| `mapFromParent(const QPoint &parentPoint)` | 将父控件内的坐标转换为本控件内的坐标   |
