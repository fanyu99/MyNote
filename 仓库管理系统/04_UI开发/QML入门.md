# 1. QML是什么? 
QML是一种基于JS和CSS构建的声明式语言
# 2. C++QT怎么加载QML?
## CMakeLists.txt基础写法
```CMake
cmake_minimum_required(VERSION 3.26)
project(QmlSyntaxDemo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
# 查找QtQuick包
find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_add_executable(${PROJECT_NAME}
    main.cpp
)
# 添加qml模块
qt_add_qml_module(${PROJECT_NAME}
    URI QmlSyntaxDemo # C++程序
    VERSION 1.0
    QML_FILES Main.qml # QML文件
)
# 链接QtQuick
target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Quick
)
```
## 主程序基础写法
```C++
#include <QGuiApplication>
#include <QQmlApplicationEngine>  // 加载qml引擎
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine; // 获取qml引擎
    const QUrl url(u"qrc:/QmlSyntaxDemo/Main.qml"_qs); // qt_add_qml_module()自动生成的qml文件路径
	// 信号槽,如果引擎启动失败退出程序 
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);
	// 加载qml文件
    engine.load(url);

    return app.exec();
}
```
# QML基础写法有哪些?
```qml
Item {
    width: 400 // 属性命令式绑定
    height: 300
    property alias buttonWidth: buttonRect.width // 属性别名:buttonRect.width的别名为buttonWidth
    property alias labelText: text1.text
    property int baseSize: 100 // 自定义属性
    Label{
	    id: text1
	    width: 200
	    height: 100
	    text: qsTr("Hello")
	    // 自定义的属性信号处理器: onPropertyNameChanged
	    // 注意自定义属性名PropertyName首字母需要大写
	    onTextChanged: function(){
		    width: 400
		    height: 200
	    }
    }
	// 子对象
    Rectangle {
	    id: buttonRect // 自定义的对象名,用于快捷访问其属性等
        width: {
            var calculated = parent.width * 0.5; // var万能类型,parent.width*0.5属性绑定
            if (calculated < baseSize) {
                calculated = baseSize;
            }
            return calculated; // width = calculated
        }
        height: width * 0.6 // 属性声明式绑定:绑定到width,width变化时height自动变化
        color: "steelblue" // 设置颜色
        // 信号处理器: 点击
        onClicked: function(){
	        labelText: qsTr("你好啊")
        }
    }
}
```
# QML最大的坑之一: 绑定断裂
## 典型例子:
```qml
Item {
    width: 400
    height: 300

    property int baseWidth: 200

    Rectangle {
        id: rect
        width: parent.width * 0.5     // 声明式绑定,绑定到parent.width
        height: 100
        color: "#4CAF50"

        MouseArea {
            anchors.fill: parent
            onClicked: function() {
                // 命令式赋值——这里会打断绑定！
                rect.width = 150; // 导致原绑定的parent.width无法恢复!
                console.log("Width after click:", rect.width);
            }
        }
    }

    // 一个按钮来修改 baseWidth
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 50
        color: "#2196F3"

        Text {
            anchors.centerIn: parent
            text: "Click the green rectangle first, then resize the window"
            color: "white"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: function() {
                baseWidth = 300;    // 修改 baseWidth
            }
        }
    }
}
```
一旦绑定断裂原来的parent.width变化时,rect的width便不会自动跟随变化!!!
## 如何避免绑定断裂?
1. 修改绑定源
```qml
Item {
    property int baseWidth: 200 // 声明一个统一的属性用于其他属性进行绑定,通过修改这个属性来更改其他的属性值的同时保持绑定关系不变

    Rectangle {
        // width 始终通过绑定计算，不直接赋值
        width: baseWidth
        height: 100
        color: "#4CAF50"

        MouseArea {
            anchors.fill: parent
            onClicked: function() {
                // 修改 baseWidth 而不是直接改 width
                baseWidth = 150;     // 绑定保持完好
            }
        }
    }
}
```
2. 使用Qt.binding()重新绑定
```qml
Item {
    Rectangle {
        id: rect
        width: parent.width * 0.5
        height: 100

        MouseArea {
            anchors.fill: parent
            onClicked: function() {
                // 先做命令式赋值
                rect.width = 150;

                // 然后重新建立绑定
                rect.width = Qt.binding(function() {
                    return rect.parent.width * 0.3;
                });
            }
        }
    }
}
```
`Qt.binding()` 接受一个函数作为参数，返回一个绑定对象。把这个绑定对象赋值给属性时，引擎会重新建立绑定关系。这种写法适合需要在运行时动态切换绑定表达式的场景。
3. 使用Binding元素进行条件性属性绑定
```qml
Item {
    property bool useCompactMode: false

    Rectangle {
        id: rect
        height: 100

        // 默认宽度
        width: parent.width * 0.5

        // 用 Binding 元素条件性地覆盖
        Binding {
            target: rect // 目标为rect
            property: "width" // 属性为width
            value: 100 // 值为100
            when: useCompactMode // 当useCompactMode为true时进行绑定,false自动解除绑定
        }

        MouseArea {
            anchors.fill: parent
            onClicked: function() {
                useCompactMode = !useCompactMode;
            }
        }
    }
}
Window {
    width: 400; height: 300; visible: true

    TextField { id: field; x: 20; y: 20; width: 200 }

    Text {
        id: label
        x: 20; y: 70
        text: "init"
    }

    Binding {
        target: label // 绑定对象为label
        property: "text" // 绑定的对象属性label.text
        value: field.text // 绑定到field.text
    }
}
```

# Qt Quick Controls 快捷控件库
## [[Qt Quick Controls库]]


