# 1,ApplicationWindow
```qml
import QtQuick
import QtQuick.Controls // controls库

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true
    title: "Qt Quick Controls Demo"

    // 菜单栏
    menuBar: MenuBar {
        Menu {
            title: "&File"
            MenuItem { text: "&New"; onTriggered: console.log("New") }
            MenuItem { text: "&Open"; onTriggered: console.log("Open") }
            MenuSeparator {}
            MenuItem { text: "&Quit"; onTriggered: Qt.quit() }
        }
        Menu {
            title: "&Help"
            MenuItem { text: "&About"; onTriggered: console.log("About") }
        }
    }

    // 顶部工具栏
    header: ToolBar {
        Row {
            anchors.fill: parent
            spacing: 8

            Button {
                text: "Action 1"
                onClicked: console.log("Action 1")
            }
            Button {
                text: "Action 2"
                onClicked: console.log("Action 2")
            }
        }
    }

    // 底部状态栏（用 ToolBar 模拟）
    footer: ToolBar {
        Label {
            anchors.fill: parent
            anchors.leftMargin: 10
            text: "Ready"
            verticalAlignment: Text.AlignVCenter
        }
    }

    // 中央内容区——背景色
    background: Rectangle {
        color: "#f5f5f5"
    }
}
```
# 2.Button 按钮
```qml
Column {
    spacing: 12

    // 普通按钮
    Button {
        text: "Click Me"
        onClicked: console.log("Button clicked")
    }

    // 带图标的按钮
    Button {
        text: "Save"
        icon.name: "document-save"
        onClicked: console.log("Save clicked")
    }

    // 可切换按钮（checkable）
    Button {
        text: checked ? "Active" : "Inactive"
        checkable: true 
        onCheckedChanged: console.log("Checked:", checked)
    }

    // 高亮按钮
    Button {
        text: "Important"
        highlighted: true
        onClicked: console.log("Important clicked")
    }

    // 禁用按钮
    Button {
        text: "Disabled"
        enabled: false
    }
}
```
`checkable: true` 把普通按钮变成了一个切换开关，点击后在选中/未选中之间切换。
`highlighted: true` 会用主题的高亮色来渲染按钮，通常用于引导用户注意的关键操作。
`icon.name` 使用系统主题图标名称（遵循 freedesktop.org 的图标命名规范），在不同平台上会自动匹配对应的图标。

# 3. TextField 单行文本输入框

```qml
Column {
    spacing: 12

    TextField {
        placeholderText: "Enter your name..."
        onTextChanged: console.log("Text:", text)
    }

    TextField {
        placeholderText: "Password"
        echoMode: TextInput.Password   // 密码模式
    }

    TextField {
        placeholderText: "Numbers only"
        // 限制只能输入数字
        validator: IntValidator { bottom: 0; top: 9999 }
    }

    TextField {
        placeholderText: "Max 10 characters"
        maximumLength: 10
    }
}
```
# 4.ComboBox 下拉选择框

```qml
Column {
    spacing: 12

    // 静态列表
    ComboBox {
        model: ["Red", "Green", "Blue", "Yellow"]
        onCurrentIndexChanged: console.log("Selected:", currentText)
    }

    // 带 ListModel 的动态列表
    ComboBox {
        textRole: "name"        // 显示 model 中的 "name" 字段
        model: ListModel {
            ListElement { name: "Apple"; color: "#F44336" }
            ListElement { name: "Banana"; color: "#FFEB3B" }
            ListElement { name: "Grape"; color: "#9C27B0" }
        }
        onActivated: function(index) {
            console.log("Selected fruit:", currentText,
                       "Color:", model.get(index).color)
        }
    }

    // 可编辑的 ComboBox
    ComboBox {
        editable: true
        model: ["Option A", "Option B", "Option C"]
        onAccepted: function() {
            // 用户输入了自定义文本后按回车
            if (find(editText) === -1) {
                console.log("New item:", editText);
            }
        }
    }
}
```
`textRole` 属性在使用 `ListModel` 时特别有用——它指定了用 model 中的哪个字段作为显示文本。
`editable: true` 让用户可以在输入框中输入自定义内容，而不只是从列表中选择。

# 5. CheckBox 复选框

```qml
Column {
    spacing: 12

    CheckBox {
        text: "Enable notifications"
        onCheckedChanged: console.log("Checked:", checked)
    }

    CheckBox {
        text: "Remember me"
        checked: true
    }

    // 三态复选框
    CheckBox {
        text: "Select All"
        tristate: true
        checkState: Qt.PartiallyChecked
        onCheckStateChanged: console.log("State:", checkState)
    }
}
```
三态模式下，`checkState` 的取值有三种：
1. `Qt.Unchecked`（未选中，值为 0）、
2. `Qt.PartiallyChecked`（部分选中，值为 1）、
3. `Qt.Checked`（选中，值为 2）。
普通模式下，`checked` 属性就是 `checkState === Qt.Checked` 的便捷方式。

# 6. Slider 滑块
```qml
Column {
    spacing: 16

    // 水平滑块
    Slider {
        from: 0
        to: 100
        value: 50
        onValueChanged: console.log("Value:", value)
    }

    // 带步进的水平滑块
    Slider {
        from: 0
        to: 1.0
        value: 0.5
        stepSize: 0.1
        onValueChanged: console.log("Opacity:", value)
    }

    // 垂直滑块
    Row {
        spacing: 20

        Slider {
            orientation: Qt.Vertical
            from: 0
            to: 100
            value: 30
        }

        Slider {
            orientation: Qt.Vertical
            from: 0
            to: 100
            value: 70
        }
    }
}
```
`from` 和 `to` 定义了值的范围，`stepSize` 定义了每次移动的步进值。`value` 属性始终持有当前的滑块位置值。`Slider` 的 `onValueChanged` 信号在拖动过程中会频繁触发，所以如果你的响应逻辑比较重（比如触发网络请求），应该加上防抖处理。

# 7. 布局系统
## 1. ColumnLayout 纵向布局
```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 400
    height: 500
    visible: true
    title: "ColumnLayout Demo"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        Label { text: "Username:" }
        TextField {
            Layout.fillWidth: true
            placeholderText: "Enter username"
        }

        Label { text: "Password:" }
        TextField {
            Layout.fillWidth: true
            echoMode: TextInput.Password
            placeholderText: "Enter password"
        }

        CheckBox {
            text: "Remember me"
            Layout.alignment: Qt.AlignLeft
        }

        Button {
            text: "Login"
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            highlighted: true
            onClicked: console.log("Login clicked")
        }

        // 弹性空间——把后续元素推到底部
        Item { Layout.fillHeight: true }

        Label {
            text: "Forgot password?"
            Layout.alignment: Qt.AlignHCenter
            color: "#2196F3"
        }
    }
}
```
`Layout.fillWidth: true` 让控件横向填满布局的可用宽度。
`Layout.preferredHeight: 44` 给按钮指定了推荐的 44 像素高度。
`Item { Layout.fillHeight: true }` 是一个常见的技巧——它创建一个不可见的弹性空间，占据剩余的所有纵向空间，从而把后面的元素推到布局的底部。

## 2. RowLayout 横向布局
```qml
RowLayout {
    spacing: 12

    Label { text: "Search:" }
    TextField {
        Layout.fillWidth: true
        Layout.preferredWidth: 200
        placeholderText: "Type to search..."
    }
    Button {
        text: "Go"
        onClicked: console.log("Search")
    }
    ComboBox {
        model: ["All", "Documents", "Images", "Music"]
        Layout.preferredWidth: 120
    }
}
```

## 3. GridLayout 网格布局
```qml
GridLayout {
    columns: 2 // 指定网格为2列
    rowSpacing: 12
    columnSpacing: 16

    Label { text: "First Name:" }
    TextField {
        Layout.fillWidth: true
        placeholderText: "John"
    }

    Label { text: "Last Name:" }
    TextField {
        Layout.fillWidth: true
        placeholderText: "Doe"
    }

    Label { text: "Email:" }
    TextField {
        Layout.fillWidth: true
        placeholderText: "john@example.com"
    }

    Label { text: "Country:" }
    ComboBox {
        Layout.fillWidth: true
        model: ["China", "USA", "Japan", "Germany", "UK"]
    }

    // 跨列按钮
    Button {
        Layout.columnSpan: 2 // 横跨两列
        Layout.fillWidth: true
        Layout.preferredHeight: 44
        text: "Submit"
        highlighted: true
        onClicked: console.log("Submit clicked")
    }
}
```

# 8. 弹出组件
## 1. Dialog 对话框
```qml
ApplicationWindow {
    id: window
    width: 600
    height: 400
    visible: true
    title: "Dialog Demo"

    Column {
        anchors.centerIn: parent
        spacing: 12

        Button {
            text: "Show Message Dialog"
            onClicked: messageDialog.open()
        }

        Button {
            text: "Show Confirm Dialog"
            onClicked: confirmDialog.open()
        }

        Button {
            text: "Show Input Dialog"
            onClicked: inputDialog.open()
        }
    }

    // 消息对话框
    Dialog {
        id: messageDialog
        title: "Information"
        modal: true
        anchors.centerIn: parent

        Label {
            text: "This is a message dialog.\nOperation completed successfully."
        }

        standardButtons: Dialog.Ok
        onAccepted: console.log("Message dialog closed")
    }

    // 确认对话框
    Dialog {
        id: confirmDialog
        title: "Confirm"
        modal: true
        anchors.centerIn: parent

        Label {
            text: "Are you sure you want to delete this item?\nThis action cannot be undone."
        }

        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: console.log("Confirmed")
        onRejected: console.log("Cancelled")
    }

    // 输入对话框
    Dialog {
        id: inputDialog
        title: "Enter Name"
        modal: true
        anchors.centerIn: parent

        Column {
            spacing: 8
            width: parent.width

            Label { text: "Please enter your name:" }
            TextField {
                id: nameField
                width: parent.width
                placeholderText: "Your name"
            }
        }

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: console.log("Name:", nameField.text)
        onRejected: console.log("Input cancelled")
    }
}
```
`standardButtons` 属性是 `Dialog` 的便捷特性——你只需要用位或运算符（`|`）组合需要的标准按钮，Qt 会自动渲染并处理按钮点击。
`Dialog.Ok`、`Dialog.Cancel`、`Dialog.Yes`、`Dialog.No`、`Dialog.Apply`、`Dialog.Close` 等都是预定义的标准按钮。
`modal: true` 让对话框变为模态，用户必须先处理对话框才能继续操作主窗口。

## 2. Popup 弹出层
`Popup` 是更底层的弹出组件，它不自带标题栏和按钮，只提供弹出/关闭的基本行为和背景遮罩。它适合实现自定义的浮动面板、工具提示、通知栏等。
```qml
ApplicationWindow {
    id: window
    width: 600
    height: 400
    visible: true

    Button {
        anchors.centerIn: parent
        text: "Show Popup"
        onClicked: customPopup.open()
    }

    Popup {
        id: customPopup
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 300
        height: 200
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#ffffff"
            radius: 12
            border.color: "#e0e0e0"
            border.width: 1

            // 阴影效果（简易版）
            Rectangle {
                anchors.fill: parent
                anchors.margins: -4
                radius: 14
                color: "transparent"
                border.color: "#10000000"
                z: -1
            }
        }

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Text {
                text: "Custom Popup"
                font.pixelSize: 18
                font.bold: true
            }

            Text {
                text: "This is a custom popup without standard buttons.\nClick outside or press Escape to close."
                font.pixelSize: 14
                color: "#666666"
                wrapMode: Text.WordWrap
                width: parent.width
            }

            Button {
                text: "Close"
                onClicked: customPopup.close()
            }
        }
    }
}
```
`closePolicy` 控制弹出的关闭方式。
`Popup.CloseOnEscape` 允许按 Escape 键关闭，`Popup.CloseOnPressOutside` 允许点击弹出区域外部关闭。
`x` 和 `y` 手动计算了居中位置——`Popup` 默认不会自动居中，需要你自己定位。

## 3.Menu 右键菜单与下拉菜单

```qml
ApplicationWindow {
    width: 600
    height: 400
    visible: true
    title: "Menu Demo"

    // 顶部菜单
    menuBar: MenuBar {
        Menu {
            title: "&File"
            MenuItem {
                text: "&New"
                onTriggered: console.log("New")
            }
            MenuItem {
                text: "&Open..."
                onTriggered: console.log("Open")
            }
            MenuSeparator {}
            MenuItem {
                text: "&Save"
                onTriggered: console.log("Save")
            }
            MenuItem {
                text: "Save &As..."
                onTriggered: console.log("Save As")
            }
            MenuSeparator {}
            MenuItem {
                text: "&Quit"
                onTriggered: Qt.quit()
            }
        }

        Menu {
            title: "&Edit"
            MenuItem {
                text: "&Undo"
                onTriggered: console.log("Undo")
            }
            MenuItem {
                text: "&Redo"
                onTriggered: console.log("Redo")
            }
            MenuSeparator {}
            MenuItem {
                text: "Cu&t"
                onTriggered: console.log("Cut")
            }
            MenuItem {
                text: "&Copy"
                onTriggered: console.log("Copy")
            }
            MenuItem {
                text: "&Paste"
                onTriggered: console.log("Paste")
            }
        }
    }

    // 右键菜单
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: function(mouse) {
            contextMenu.popup()
        }
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: "Refresh"
            onTriggered: console.log("Refresh")
        }
        MenuItem {
            text: "Properties"
            onTriggered: console.log("Properties")
        }
        MenuSeparator {}
        MenuItem {
            text: "Help"
            onTriggered: console.log("Help")
        }
    }
}
```