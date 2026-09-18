# 1. Q_PROPERTY 把C++属性接入到QML绑定系统中
## 1. Q_PROPERTY是什么?
`Q_PROPERTY`是Qt元对象系统中的属性声明宏,当你在C++类中写了`Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)`,QML引擎就能识别到这个属性,将其作为QML属性使用,包括绑定追踪

## 2.定义一个可暴露的C++类
```C++
// app_controller.h
#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

class AppController : public QObject
{
    Q_OBJECT
    // 将 C++ 属性暴露给 QML 引擎
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(int counter READ counter WRITE setCounter NOTIFY counterChanged)
    Q_PROPERTY(QString themeColor READ themeColor WRITE setThemeColor NOTIFY themeColorChanged)

    // 允许该类在 QML 中通过类型名直接实例化
    QML_ELEMENT

public:
    explicit AppController(QObject *parent = nullptr);

    // 属性读取方法
    QString userName() const;
    int counter() const;
    QString themeColor() const;

    // 属性写入方法（内部发射对应的 NOTIFY 信号）
    void setUserName(const QString &name);
    void setCounter(int value);
    void setThemeColor(const QString &color);

    // Q_INVOKABLE 使该方法可从 QML 中调用
    Q_INVOKABLE void increment();
    Q_INVOKABLE void reset();
    Q_INVOKABLE QString greeting() const;

signals:
    void userNameChanged();
    void counterChanged();
    void themeColorChanged();
    // 自定义信号也可以从 C++ 发射到 QML
    void notificationRequested(const QString &message);

private:
    QString m_userName;
    int m_counter = 0;
    QString m_themeColor = "#3498db";
};

#endif // APP_CONTROLLER_H
```
`READ` 指定读取方法，返回属性值；
`WRITE` 指定写入方法，在内部发射变更信号；
`NOTIFY` 指定一个无参信号，QML 引擎通过它来感知属性变化。如果缺了 `NOTIFY`，这个属性在 QML 里就变成了只读的一次性值——绑定追踪不会生效。

`QML_ELEMENT` 宏。这是 Qt 5.15 引入、Qt 6 大力推广的注册方式。过去我们需要手动调用 `qmlRegisterType<AppController>("MyModule", 1, 0, "AppController")`，现在只要在类声明里加上 `QML_ELEMENT`，配合 CMake 的 `qt_add_qml_module`，构建系统会自动完成类型注册。注册之后，QML 就能像使用内置类型一样使用 `AppController`。

`Q_INVOKABLE`。被这个宏修饰的成员函数，QML 引擎可以通过元对象系统调用它。参数和返回值会自动在 C++ 类型与 JavaScript 类型之间转换——`QString` 变成 JS 字符串，`int` 变成 JS 数字(注意计算时会变为real类型)，`QVariantMap` 变成 JS 对象，以此类推。

# 3. C++实现文件
```C++
// app_controller.cpp
#include "app_controller.h"

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

QString AppController::userName() const
{
    return m_userName;
}

void AppController::setUserName(const QString &name)
{
    if (m_userName != name) {
        m_userName = name;
        emit userNameChanged();
    }
}

int AppController::counter() const
{
    return m_counter;
}

void AppController::setCounter(int value)
{
    if (m_counter != value) {
        m_counter = value;
        emit counterChanged();
    }
}

QString AppController::themeColor() const
{
    return m_themeColor;
}

void AppController::setThemeColor(const QString &color)
{
    if (m_themeColor != color) {
        m_themeColor = color;
        emit themeColorChanged();
    }
}

void AppController::increment()
{
    setCounter(m_counter + 1);
}

void AppController::reset()
{
    setCounter(0);
}

QString AppController::greeting() const
{
    if (m_userName.isEmpty()) {
        return "Hello, stranger!";
    }
    return "Hello, " + m_userName + "! Count: " + QString::number(m_counter);
}
```
每个 setter 内部都做了不等判断 `if (m_xxx != xxx)` 再发射信号。这不是防御性编程的多余操作，而是 Qt 属性系统的标准实践。如果不做这个判断，每次调用 setter 都会发射信号，哪怕值根本没变——这在 QML 端会导致不必要的绑定重算和 UI 刷新，严重时甚至造成无限循环。很多新手在这里踩坑，发现 UI 莫名其妙地闪烁或卡顿，最后追查到 setter 里缺少了这个守卫。

# 4. 注册方式类比 QML_ELEMENT vs setContextProperty
## 1. 可复用组件用QML_ELEMENT: 类型注册,QML端实例化
当你用 `QML_ELEMENT` 注册一个 C++ 类型后，QML 可以像使用原生类型一样直接声明这个类型的实例：

```
AppController {
    id: controller
    userName: "Charlie"
}
```
这种方式适合「每个 QML 实例对应一个独立的 C++ 对象」的场景。比如你有一个自定义的 `ImageViewer` 组件，每个实例都有自己的状态和配置，那就应该用类型注册。
在 CMake 端，`QML_ELEMENT` 配合 `qt_add_qml_module` 使用时，你需要把 C++ 源文件加到 `qt_add_executable` 中，构建系统会自动为你的 QML 模块生成类型注册信息。QML 中通过模块 URI 导入后就能使用这些类型。
## 2. 全局实例用setContextProperty: 对象注入,C++管理生命周期
`QQmlContext::setContextProperty()` 的思路完全不同。你不是注册一个「类型」，而是直接把一个已经构造好的 C++ 对象实例塞进 QML 的全局上下文。QML 里通过一个字符串名字来引用它，不需要 import 任何模块：

```
QQmlApplicationEngine engine;
auto *controller = new AppController(&app);
engine.rootContext()->setContextProperty("appController", controller);
```
```
// QML 中直接用 appController 这个名字
Text {
    text: appController.userName
}


这种方式适合「全局唯一的后端对象」场景——比如应用级别的配置管理器、网络客户端、数据源。整个应用只需要一个实例，所有 QML 文件共享同一个对象。