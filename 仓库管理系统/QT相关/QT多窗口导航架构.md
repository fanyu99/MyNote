## ***1.QStackedWidget页面切换***
1.基础概念:
	 一叠卡片,同一时刻只显示最上方的那张,需要自行配置导航ComboBox/List/Button(与QTabWidget最大的区别)
	通过setCurrentindex(),hide(),show()等API进行管理
2.核心API:

| <br>API                                              | **<br><br>作用<br><br>** |
| ---------------------------------------------------- | ---------------------- |
| `addWidget(QWidget*)`                                | 追加一页，**返回该页索引**​       |
| `insertWidget(int, QWidget*)`                        | 插到指定位置                 |
| `removeWidget(QWidget*)`                             | 从栈里摘掉（**不 delete**）    |
| `setCurrentIndex(int)`/ `setCurrentWidget(QWidget*)` | 切页                     |
| `currentIndex()`/ `currentWidget()`                  | 当前页索引 / 指针             |
| `count()`                                            | 总页数                    |
| `widget(int)`                                        | 取指定索引的页指针              |
| `indexOf(QWidget*)`                                  | 查某页的索引                 |
	两个信号:
	void currentChanged(int index); // 切页(空栈时返回-1)
	void widgetRemoved(int index); // 删除页
3.相关代码示例:
```C++
auto page1 = new QWidget;
stackedWidget = new QStackedWidget;
stackedWidget->addWidget(page1);
// ComboBox 切页
    QObject::connect(combo, &QComboBox::activated, stackedWidget, &QStackedWidget::setCurrentIndex);
    // 上一页按钮
    QObject::connect(btnPrev, &QPushButton::clicked, this, [this]() {
       int idx = stackedWidget->currentIndex();
        if (idx > 0) {
            stackedWidget->setCurrentIndex(idx - 1);
        }
        // 如果是第一页就切换到最后一页
        else
            stackedWidget->setCurrentIndex(stackedWidget->count() - 1);
    });
    // 下一页按钮
    QObject::connect(btnNext, &QPushButton::clicked, this, [this]() {
        int idx = stackedWidget->currentIndex();
        if (idx < stackedWidget->count() - 1) {
            stackedWidget->setCurrentIndex(idx + 1);
        }
        // 如果是最后一页就切换到第一页
        else
            stackedWidget->setCurrentIndex(0);
    });
```