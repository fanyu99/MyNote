---
title: QObject 属性系统
type: 主题笔记
系列: 上位机学习
created: 2026-10-13
updated: 2026-10-13
tags:
  - 上位机
  - Qt
  - QObject
  - 元对象系统
  - 主题笔记
status: 进行中
---

# QObject 属性系统

> 概述：`Q_PROPERTY` 让成员变量进入 Qt **元对象系统**，从而支持 QML 绑定、`QMetaProperty` 反射、`setProperty` 动态属性等能力。本页梳理 `Q_PROPERTY` 完整语法与关键字、`MEMBER` 简写、`Q_ENUM` 联动、动态属性机制与反射遍历。

---

## 一、`Q_PROPERTY` 完整语法

```cpp
Q_PROPERTY(type name
           (READ getFunction [WRITE setFunction] |
            MEMBER memberName [(READ getFunction | WRITE setFunction)])
           [RESET resetFunction]
           [NOTIFY notifySignal]
           [REVISION int]
           [DESIGNABLE bool]
           [SCRIPTABLE bool]
           [STORED bool]
           [USER bool]
           [CONSTANT]
           [FINAL]
)
```

各关键字含义：

1. **`READ` / `WRITE` / `NOTIFY`** 负责属性的读写以及信号。
2. **`RESET`** 指定一个无参成员函数，把属性重置到默认值。
3. **`STORED`** 接收一个 `bool`，决定属性是否被持久保存。
4. **`CONSTANT` 和 `FINAL`** 是语义标记：
   - `CONSTANT` 表示属性的值在对象生命周期内不会改变，其属性不能有 `NOTIFY` / `WRITE` 函数；
   - `FINAL` 表示该属性不能被子类覆盖。
5. **`SCRIPTABLE` 和 `DESIGNABLE`** 控制不同上下文的可见性：
   - `SCRIPTABLE` 决定属性是否能够对脚本引擎暴露；
   - `DESIGNABLE` 决定属性是否能够被 Qt Designer 显示。
6. **`USER`** 代表这个类的用户可编辑的主属性，例如 `QTextEdit` 的 `USER` 属性是 `plainText`。

---

## 二、`MEMBER`：让属性声明更简洁

除了经典的 `READ`/`WRITE`，`Q_PROPERTY` 可以使用更简洁的写法：`MEMBER` 直接绑定一个成员变量，不需要再去写 getter/setter。

```cpp
class SimpleConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title MEMBER m_title NOTIFY titleChanged)
signals:
    void titleChanged();
private:
    QString m_title;
};
```

> [!warning] MEMBER 的局限
> `MEMBER` 不能像 setter 那样在内部封装「变更守卫」，可能导致不必要的信号触发。因此一般仍使用 `READ`/`WRITE`，`MEMBER` 通常只用于纯数据或快速原型。

---

## 三、`Q_ENUM` 与属性系统联动

```cpp
enum LogLevel { ... };
Q_ENUM(LogLevel)
```

`Q_ENUM` 能把枚举注册到元对象系统，让属性系统在**枚举值与字符串之间互相转换**，可用于序列化、QML 绑定等场景。

注册后可以直接使用 `QMetaEnum::fromType<Config::LogLevel>()` 获取元枚举对象，再用 `keyToValue` / `valueToKey` 进行转换。

---

## 四、动态属性：`setProperty` / `property`

`Q_PROPERTY` 是**编译期声明的静态属性**；Qt 还提供了一套**运行时动态属性**机制：通过 `setProperty(name, value)` 和 `property(name)`，可以在不声明 `Q_PROPERTY` 的情况下给任何 `QObject` 附加属性。

- 对已经用 `Q_PROPERTY` 声明过的属性名调用 `setProperty`：Qt 走**静态属性路径**，调用你的 `WRITE` 函数。
- 对未声明的名称调用 `setProperty`：Qt 走**动态属性路径**，在 `QObject` 内部的一个 `QHash` 里存一个键值对。

```cpp
// 对已声明属性——调用 WRITE 函数
obj.setProperty("debugMode", true);  // 等价于 obj.setDebugMode(true)

// 对未声明名称——创建动态属性
obj.setProperty("customTag", "important");  // 存入内部 QHash
```

`dynamicPropertyNames()` 返回所有动态属性的名称列表。动态属性的值是 `QVariant` 类型，可以存任何能包装进 `QVariant` 的类型。

典型用途：给控件附加元数据（如在 Model/View 里给 delegate 传标记）、在不修改类定义的情况下给第三方类实例添加状态、在 QSS 样式表中用动态属性做条件选择。

> [!note] 性能
> 动态属性虽然灵活，但每次访问都要做一次哈希查找，性能不如静态属性，不要在高频路径上滥用。

---

## 五、`QMetaProperty` 反射

`QMetaObject` 是每个含有 `Q_OBJECT` 宏的类都会生成的元对象描述，可以用 `obj->metaObject()` 获取。

`QMetaProperty` 提供了一组布尔查询函数：`isReadable()`、`isWritable()`、`isResettable()`、`hasNotifySignal()`、`isConstant()`、`isFinal()`，还有 `read(obj)` 和 `write(obj, value)` 方法，可以在不知道属性具体名字的情况下动态读写。

```cpp
const QMetaObject* meta = obj->metaObject();
for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
    QMetaProperty prop = meta->property(i); // 获取元属性
    qDebug() << prop.name()
             << "可读:" << prop.isReadable()
             << "可写:" << prop.isWritable();
}
```

---

## 相关笔记

- [[上位机学习/QT底层原理深入/信号与槽]] —— 信号槽连接类型与元类型系统
- [[上位机学习/QT+ONNXRuntime项目/QML+CPP的联动知识]] —— `Q_PROPERTY` / `Q_INVOKABLE` 在 QML 中的用法

---

## 参考链接

- [1.1 QObject 属性系统深度拆解 | AwesomeQt 教程](https://awesome-embedded-learning-studio.github.io/Tutorial_AwesomeQt/advanced/01-qtbase/01-qobject-property-system-advanced)

---

- **知识库总览**：[[上位机学习/_MOC]]
- **仓库主页**：[[README]]

---

*本页随学习进度持续更新。*
