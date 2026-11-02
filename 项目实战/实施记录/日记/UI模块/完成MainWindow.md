---
title: 完成MainWindow
type: 通用笔记
topic: 项目实战
status: 学习中
created: 2026-08-27
tags:
  - wms
  - qt
  - mysql
  - 实施记录
series: WMS 实施记录
canvas: 实施记录/白板/UI模块/Day24 完成MainWindow.canvas
---

# 完成MainWindow

> 记录「完成MainWindow」当天的任务、实现过程与结论。

[[QT对象树的注意事项]]
1. 用于调度InboundPage/OutboundPage/ProductPage/LoginPage
2. 添加导航栏用于页面导航
3. 添加状态栏显示用户信息
4. 保持页面状态的一致性
## 注意点:
    用户登出时应该调用LoginPage重新登录函数,不然LodingPage因为setPageState而部件被禁用
     在栈上创建的对象,需要注意其析构的顺序,避免二次析构导致崩溃:
```C++
 QPushbutton button("测试");
 QMainWindow window; // 出现问题
 button.setParent(&window); // 出现问题:C++作用域内的局部对象析构顺序为创建顺序的逆向,会先析构window后再析构button,但此时button已经被清理,导致了二次析构!!!
```