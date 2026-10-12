[1.1 QObject 属性系统深度拆解 | AwesomeQt 教程](https://awesome-embedded-learning-studio.github.io/Tutorial_AwesomeQt/advanced/01-qtbase/01-qobject-property-system-advanced)
# 1.Q_PROPERTY完整语法
```C++
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
1. READ/WIRTE/NOTIFY负责属性的读写以及信号
2. RESET指定一个无参成员函数,重置属性到默认值
3. STORED接收一个bool,决定属性是否被持久保存
4. CONSTATN和FINAL是语义标记,
   CONSTAT表示属性的值再对象生命周期内不会发生改变,其属性不能有NOTIFY/WRITE函数;
   FINAL表示该属性不能被子类覆盖
5. SCRIPTABLE和DESIGNABLE控制不同上下文属性的可见性;
   SCRIPTABLE决定属性是否能够对脚本引擎暴露;
   DESIGNABLE决定属性是否能够被QtDesigner显示
6. USER代表这个类的用户可编辑的主属性,例如QTextEdit的USER属性是plainText
## MEMBER-让属性声明更加简洁
除了经典的READ/WRITE,Q_PROPERTY可以使用更加简洁的写法
MEMBER直接绑定一个成员变量,不需要再去写getter/setter函数
```C++
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
但是注意: MEMBER不能够进行像setter函数那样里面封装变更守卫,可能导致不必要的触发信号
因此,一般还是使用READ/WRITE,MEMBER通常只用于纯数据/快速原型

# Q_ENUM与属性系统联动
```C++
enum LogLevel{...};
Q_ENUM(LogLevel)
```
Q_ENUM能够将枚举注册到元对象系统,让属性系统能够再枚举值和字符串之间进行互相转换;可用于序列化/QML绑定等场景
注册后可以直接使用`QMetaEnum::fromType<Config::LogLevel>()`获取元枚举对象,然后使用`keyToValue`/`valueToKey`进行转换

# 动态属性-setProperty/property
Q_PROPERTY 是编译期声明的静态属性。但 Qt 还提供了一套运行时的动态属性机制：通过 `setProperty(name, value)` 和 `property(name)` 可以在不声明 Q_PROPERTY 的情况下给任何 QObject 附加属性。
当你对一个已经用 Q_PROPERTY 声明过的属性名调用 setProperty 时，Qt 走的是「静态属性路径」——调用你的 WRITE 函数。但当你对一个未声明的名称调用 setProperty 时，Qt 走的是「动态属性路径」——在 QObject 内部的一个 QHash 里存一个键值对。
```C++
// 对已声明属性——调用 WRITE 函数
obj.setProperty("debugMode", true);  // 等价于 obj.setDebugMode(true)

// 对未声明名称——创建动态属性
obj.setProperty("customTag", "important");  // 存入内部 QHash
```
`dynamicPropertyNames()` 返回所有动态属性的名称列表。动态属性的值是 QVariant 类型，这意味着你可以存任何能包装进 QVariant 的类型。

动态属性的典型用途包括：给控件附加元数据（比如在 Model/View 里给 delegate 传标记）、在不修改类定义的情况下给第三方类的实例添加状态、以及在 QSS 样式表中用动态属性做条件选择。这个机制虽然灵活，但性能不如静态属性——每次访问都要做一次哈希查找，所以不要在高频路径上滥用。

# QMetaProperty反射

QMetaObject是每个含有Q_OBJECT宏的类都会生成的元对象描述,可以使用`obj->metaObject()`获取
QMetaProperty提供了一组布尔查询函数：`isReadable()`、`isWritable()`、`isResettable()`、`hasNotifySignal()`、`isConstant()`、`isFinal()`。还有 `read(obj)` 和 `write(obj, value)` 方法，可以在不知道属性具体名字的情况下动态读写。

```C++
const QMetaObject* meta = obj->metaObject();
for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
    QMetaProperty prop = meta->property(i);// 获取元属性
    qDebug() << prop.name()
             << "可读:" << prop.isReadable()
             << "可写:" << prop.isWritable();
}
```