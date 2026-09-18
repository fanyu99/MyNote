# 1. ListModel+ListView 纯QML的列表方案
## ListModel

```qml
ListModel {
    id: fruitModel

    ListElement {
        name: "Apple"
        color: "#e74c3c"
        price: 5.5
        inStock: true
    }
    ListElement {
        name: "Banana"
        color: "#f39c12"
        price: 3.2
        inStock: true
    }
    ListElement {
        name: "Grape"
        color: "#9b59b6"
        price: 12.0
        inStock: false
    }
}
// 动态添加一条数据
fruitModel.append({ "name": "Orange", "color": "#f39c12", "price": 4.0, "inStock": true })

// 修改第一条数据的 price
fruitModel.set(0, { "price": 6.0 })

// 删除第二条
fruitModel.remove(1)

// 清空所有数据
fruitModel.clear()
```
## ListView 
```qml
ListView {
    id: fruitList
    width: parent.width
    height: parent.height
    model: fruitModel
    spacing: 8
    clip: true

    // 基础的 listview 需要指定一个 delegate
    delegate: Rectangle {
        width: fruitList.width
        height: 60
        color: "#ffffff"
        radius: 8
        border.color: "#e0e0e0"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            Rectangle {
                width: 36
                height: 36
                radius: 18
                color: model.color
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: model.name
                    font.pixelSize: 15
                    font.bold: true
                }
                Text {
                    text: "Price: $" + model.price.toFixed(2)
                    font.pixelSize: 12
                    color: "#888"
                }
            }

            Text {
                text: model.inStock ? "In Stock" : "Out of Stock"
                font.pixelSize: 12
                color: model.inStock ? "#27ae60" : "#e74c3c"
            }
        }
    }
}
```
## delegate的抽取与复用
当 delegate 的代码量比较大时（几十行甚至上百行），直接内联在 `ListView` 里会让文件变得很难维护。更好的做法是把 delegate 抽成一个独立的组件：
```qml
// FruitDelegate.qml
import QtQuick

Rectangle {
    // 声明对外暴露的属性，由 ListView 自动注入 model 数据
    required property string name
    required property color fruitColor
    required property real price
    required property bool inStock

    width: ListView.view.width
    height: 60
    color: "#ffffff"
    radius: 8
    border.color: "#e0e0e0"

    // ... 内部布局同上
}
```
然后在ListView中进行引用
```qml
ListView {
    delegate: FruitDelegate {}
}
```
`ListView` 会自动把 model 中的字段名和 delegate 的 `required property` 匹配起来。字段名和属性名**必须一致**——如果 model 里叫 `color`，delegate 里也叫 `color`，它们就能自动对接。如果你在 model 里叫 `fruitColor` 而 delegate 里叫 `color`，那就需要手动映射了。

# Repeater重复器
```qml
// 使用网格布局
GridLayout {
    columns: 3
    rowSpacing: 12
    columnSpacing: 12
	// 重复器
    Repeater {
        model: fruitModel
        delegate: Rectangle {
            Layout.fillWidth: true
            height: 100
            color: "#ffffff"
            radius: 8
            border.color: "#e0e0e0"
            Column {
                anchors.centerIn: parent
                spacing: 8
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: model.color
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: model.name
                    font.pixelSize: 13
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
}
```
`Repeater` 配合 `GridLayout` 可以轻松实现网格效果。`columns` 指定每行放几个元素，`Repeater` 依次创建的 delegate 会按从左到右、从上到下的顺序填入网格。
# GridView网格视图
```qml
GridView {
    width: parent.width
    height: parent.height
    cellWidth: width / 3
    cellHeight: 120
    model: fruitModel
    clip: true

    delegate: Rectangle {
        width: GridView.view.cellWidth - 8
        height: GridView.view.cellHeight - 8
        color: "#ffffff"
        radius: 8

        Column {
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                width: 36
                height: 36
                radius: 18
                color: model.color
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                text: model.name
                font.pixelSize: 13
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
```
**注意:**`GridView` 有**虚拟化**机制——只有当前可见区域内的 delegate 会被实例化，滚动出视野的 delegate 会被回收复用。这意味着即使 model 有一万条数据，实际创建的 delegate 实例也只有屏幕上能看到的那些。这对性能至关重要。

# C++QAbstractListModel 从后端驱动QML列表
`ListModel` 够简单，但有一个致命的限制：数据只能存在 QML 里。如果你的数据来自数据库、网络请求、或者复杂的业务计算，你不可能先把这些数据全部序列化成 QML 的 `ListElement` 再显示——那太低效了，也不符合前后端分离的架构原则。
Qt 提供的解决方案是 `QAbstractListModel`。你在 C++ 端继承这个类，实现几个虚函数，QML 就能通过 `ListView` 直接消费这个模型。数据留在 C++ 端管理，QML 只负责展示。
## QAbstractListModel实现:
### [[[Model View]]]

## 在QML中使用C++模型Model
在main.cpp中创建模型实例后通过setContextProperty进行注入
```C++
// main.cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "fruit_model.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    FruitModel fruitModel;
    engine.rootContext()->setContextProperty("fruitModel", &fruitModel); // 注入Model

    const QUrl url(u"qrc:/QmlModelDelegateDemo/Main.qml"_qs);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
```
然后直接在QML里使用
```qml
ListView {
    model: fruitModel   // 直接引用 C++ 模型对象

    delegate: Rectangle {
        width: ListView.view.width
        height: 60

        Text {
            text: model.name           // 对应 NameRole
        }
        Rectangle {
            color: model.fruitColor    // 对应 ColorRole
        }
        Text {
            text: "$" + model.price    // 对应 PriceRole
        }
    }
}
```
# 虚拟化与性能注意事项
`ListView` 和 `GridView` 都有内置的虚拟化机制。
这意味着不管你的 model 有多少条数据，实际创建的 delegate 实例数量等于可见区域内能显示的数量加上下各一个缓冲区。滚动时，离开视野的 delegate 会被回收到复用池，新的 delegate 从池中取出并填充新数据。这个机制大大减少了内存占用和实例化开销。

但虚拟化也带来了一些需要注意的点。delegate 里的 `Component.onCompleted` 和 `Component.onDestruction` 不等价于数据的「创建」和「销毁」——它们只代表 delegate 这个 UI 组件的创建和回收。你不能在这两个信号里做数据的初始化或清理，因为同一个 delegate 可能被多条数据复用。正确的做法是使用 `onXXXChanged` 信号处理器或 `required property` 绑定来响应数据变化。

另一个性能注意点是 delegate 的复杂度。每个 delegate 内部的元素越多、绑定表达式越复杂，滚动时的帧率就越低。如果发现列表滚动有卡顿，首先检查 delegate 里是否有不必要的嵌套和复杂绑定。一个简单的优化是把 delegate 的高度固定（避免高度计算触发额外布局），以及尽量减少 delegate 内的 `Loader` 和条件渲染。