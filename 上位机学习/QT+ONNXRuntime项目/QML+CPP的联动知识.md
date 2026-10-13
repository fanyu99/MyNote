---
title: QML 与 C++ 的联动知识
type: 主题笔记
系列: 上位机学习
created: 2026-10-13
updated: 2026-10-13
tags:
  - 上位机
  - Qt
  - QML
  - 信号与槽
  - 主题笔记
status: 进行中
---

# QML 与 C++ 的联动知识

> 概述：上位机 UI 用 QML 写、业务逻辑用 C++ 写，二者靠**元对象系统**打通。本页梳理类型注册宏（`QML_ELEMENT` / `Q_GADGET`）、属性暴露（`Q_PROPERTY`）、方法调用（`Q_INVOKABLE`）三个核心机制，并给出「C++ 后端 + QML 端」的完整示例。

---

## 一、类型注册宏：`QML_ELEMENT` / `Q_GADGET`

### 1. `QML_ELEMENT` 宏

告诉 QML 类型系统将这个类型注册到 QML，使 QML 能够 `import` 导入后直接使用：

```cpp
class ImageProvider
{
    QML_ELEMENT // 注册到 QML 类型系统，QML 能够直接使用
public:
    ...
};
```

类似的还有 `setContextProperty(字符串名, 类对象)`，但这是把对象实例直接塞进 QML 全局上下文，QML 不需要 `import` 就能使用，且全局只有一个该对象实例。

### 2. `Q_GADGET` 宏

让一个**不继承 `QObject`** 的普通类具有元对象能力（属性 / 枚举 / invokable 方法可被发现），但**没有**信号槽 / 父子对象 / 事件循环。适合纯数据类型，例如坐标、配置、状态码：

```cpp
struct AppError
{
    Q_GADGET // 仅数据类型，不包含信号等机制
    QString errorMessage;
    AppErrorCode errorCode;
    ...
};
```

---

## 二、`Q_PROPERTY`：暴露可绑定属性

`Q_PROPERTY` 用于将成员变量暴露为 QML 可绑定的属性，标准写法有 `READ`/`WRITE`/`NOTIFY` 等。`NOTIFY` 信号触发后，QML 绑定会**自动刷新**。

```cpp
class Counter
{
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged FINAL) // 注册属性到 QML 属性系统
};
```

> 更多细节（`MEMBER` 简写、`Q_ENUM`、动态属性、反射）见 [[上位机学习/QT底层原理深入/QObject属性系统]]。

---

## 三、`Q_INVOKABLE`：让普通方法可被 QML 调用

让普通的成员函数能够被 QML 直接调用——相当于不用写槽也能被 QML 调用（槽函数本身也能被 QML 直接调用）。

```cpp
Q_INVOKABLE int add(int a, int b); // 能够让 QML 直接调用该类中的 add 函数
```

---

## 四、完整示例：C++ 后端 + QML 端

### 1. C++ 端

```cpp
#include <QtQml/qqmlregistration.h>

class Backend : public QObject
{
    Q_OBJECT                     // 注册到 QObject 系统，支持信号槽等机制
    QML_ELEMENT                  // QML 里用类名 Backend 创建
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged FINAL) // 设置 QML 可直接绑定的属性
public:
    Q_INVOKABLE void login(const QString& u); // 可以直接被调用的普通函数
signals:
    void userChanged(); // 信号
};
```

### 2. QML 端

```javascript
import com.demo 1.0 // CMake 使用 qt_add_qml_module(... SOURCES backend.h backend.cpp URI com.demo VERSION 1.0)
Backend {
    id: be
    onUserChanged: console.log(user) // QML 端槽函数格式：onSignalChanged
}
Component.onCompleted: be.login("tom")
```

### 3. 纯数据结构示例（`Q_GADGET`）

```cpp
class Point
{
    Q_GADGET
    Q_PROPERTY(int x READ x WRITE setX)
    Q_PROPERTY(int y READ y WRITE setY)
    QML_VALUE_TYPE(point) // QML 端直接用 property point p 进行实例化
};
```

---

## 相关笔记

- [[上位机学习/QT底层原理深入/QObject属性系统]] —— `Q_PROPERTY` 完整语法与元对象系统
- [[上位机学习/QT+ONNXRuntime项目/整体架构]] —— 本项目的 C++ / QML 分层架构

---

- **知识库总览**：[[上位机学习/_MOC]]
- **仓库主页**：[[README]]

---

*本页随学习进度持续更新。*
