
# 1.`QML_ELEMENT`/`Q_GADGET`宏
## `QML_ELEMENT`宏
告诉QML类型系统将这个类型注册到QML,使QML能够使用`import`进行导入后直接用
```C++
class ImageProvider{
QML_ELEMENT // 注册到QML类型系统,QML能够直接使用	
	public:
	...
};
```
类似的还有`setContextProperty(字符串名,类对象)`,但是这是将对象实例直接塞进QML全局上下文,QML不需要`import`就能使用,但是全局只有一个该对象实例!
## `Q_GADGET`宏
让一个不继承`QObject`的普通类具有元对象能力(属性/枚举/invokable方法可被发现),但是没有**信号槽/父子对象/事件循环**,适合纯数据类型,例如坐标/配置/状态码等
```C++
struct AppError
{
Q_GADGET // 仅数据类型,不包含信号等机制
QString errormessage;
AppErrorCode errorCode;
	...
};
```

# 2.`Q_PROPERTY`宏
`Q_PROPERTY`用于将成员变量暴露为QML可绑定的属性,标准写法有READ/WRITE/NOTIFY等
NOTIFY信号触发后QML绑定自动刷新
```C++
class counter{
	Q_OBJECT
	Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged FINAL) // 注册属性到QML属性系统
};
```
## 3.`Q_INVOKABLE`宏
让普通的成员函数能够被QML直接调用,相当于不用写槽也能被QML调用
槽函数能够被QML直接调用
```C++
Q_INVOKABLE int add(int a,int b); // 能够让QML直接调用该类中的add函数
```

# 4.C++后端QML可实例化对象示例
C++端
```C++
#include <QtQml/qqmlregistration.h>
class Backend : public QObject {
    Q_OBJECT // 注册到QObject系统,支持信号槽等机制
    QML_ELEMENT                  // QML 里用类名 Backend 创建
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged FINAL) // 设置QML可直接绑定的属性
public:
    Q_INVOKABLE void login(const QString& u); // 可以直接被调用的普通函数
signals:
    void userChanged(); // 信号
};
```
QML端
```js
import com.demo 1.0 // CMAKE使用qt_add_qml_module(... SOURCES backend.h backend.cpp URI com.demo VERSION 1.0)
Backend {
    id: be
    onUserChanged: console.log(user) // QML端槽函数格式:onSignalChanged
}
Component.onCompleted: be.login("tom")
```
纯数据结构示例(Q_GADGET)
```C++
class Point{
	Q_GADGET
	Q_PROPERTY(int x READ x WRITE setX)
	Q_PROPERTY(int y READ y WRITE setY)
	QML_VALUE_TYPE(point) // QML端直接使用property point p进行实例化
	
};
```