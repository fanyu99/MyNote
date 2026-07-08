# 1.使用QSS文件进行对部件的形态管理:
***1.盒子模型***
```
╔═══════════════════ margin（外边距） ═══════════════════╗
║                                                         ║
║   ╔══════════════════ border（边框） ═════════════════╗  ║
║   ║                                                   ║  ║
║   ║   ╔═══════════ padding（内边距） ═══════════════╗  ║  ║
║   ║   ║                                           ║  ║  ║
║   ║   ║            content（内容区）                ║  ║  ║
║   ║   ║                                           ║  ║  ║
║   ║   ╚═══════════════════════════════════════════╝  ║  ║
║   ║                                                   ║  ║
║   ╚═══════════════════════════════════════════════════╝  ║
║                                                         ║
╚═════════════════════════════════════════════════════════╝
```
- **content**（内容区）：QLabel 文字、QPushButton 文本等
- **padding**（内边距）：content 和 border 之间的空白区域
- **border**（边框）：控件的边界线条
- **margin**（外边距）：控件和其它控件之间的空白（不参与绘制，只影响布局）
***2.相关代码示例***
```css
QTextEdit #textedit/*指定控件的名称*/ {
    background-color: palette(base);    /*使用Qpalette管理背景颜色*/

    color: palette(text);

    font-size: 16px;  /*设置字体大小*/
    
    font-family: "微软雅黑";/*设置字体类型*/

    border: 1px solid palette(mid);/*设置边界的相关形态*/

    border-radius: 5px;

    padding: 10px;
}
QTextEdit:focus /*指定伪状态focus时的形态*/{
    border-color: palette(highlight);
}
```
***3.相关的架构***

| 场景           | 架构                  | 优势/具体做法                               |
| ------------ | ------------------- | ------------------------------------- |
| 小项目          | 单/多QSS文件+手动切换       | 根据主题进行手动切换app的stylesheet              |
| 中型项目         | 多QSS文件+enum主题       | 根据enum的主题进行相关切换                       |
| 大型项目或长期维护的项目 | base.qss+enum主题覆盖   | base.qss管理控件的形态与布局<br>QPalette管理颜色和字体 |
| 超大型项目        | QSS + cpp Style API | 动态样式极多，使用C++接管样式                      |
# 2.使用QPalette进行对部件的颜色管理:
***1.QPalette的Role***

| **Role**        |       **语义**        |                   **典型使用场景**                    |
| :-------------- | :-----------------: | :---------------------------------------------: |
| `Window`        |        通用背景色        |                  QWidget 默认背景                   |
| `WindowText`    |    配 Window 的前景色    |                     普通标签文字                      |
| `Base`          |    **可编辑/交互区**背景    |    QLineEdit、QTextEdit、QComboBox 下拉、ItemView    |
| `Text`          |     配 Base 的前景色     |                     输入框里的文字                     |
| `AlternateBase` |        交替行背景        | QAbstractItemView 开 `setAlternatingRowColors`时用 |
| Button          |        按钮背景         |                                                 |
| ButtonText      |       按钮文字颜色        |                                                 |
| BrightText      | 与WindowText强对比的醒目文字 |                      用作区分                       |
| ......          |       ......        |                     ......                      |

***2.相关示例***
```C++
QString path = (theme == Theme::Light) ? "../theme/light.json" : "../theme/dark.json";//使用json文件保存Role相关的颜色/字体设置
    QFile f(path);
    if (!f.open(QFile::ReadOnly)) {
        qDebug() << "打开调色板文件失败:" << path;
        return;
    }
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    f.close();
    QPalette p;
    p.setColor(QPalette::Window, QColor(obj["window"].toString()));//设置窗口背景颜色
    p.setColor(QPalette::WindowText, QColor(obj["windowText"].toString()));
    p.setColor(QPalette::Base, QColor(obj["base"].toString()));
	qApp->setPalette(p);// 设置palette同步刷新
    qApp->setStyleSheet(qApp->styleSheet()); // 同步刷新
```