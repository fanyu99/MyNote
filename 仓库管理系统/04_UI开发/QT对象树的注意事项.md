## 1. 什么是对象树?
	对象树是QT的一种内存管理机制,所有除QObject基类以外的类都能够有两个成员:1-parent父对象 2-children子对象列表;
	模型类似于:
	QMainWindow（根）
    └── centralWidget
    └── QVBoxLayout
        ├── QPushButton ("OK")
        ├── QPushButton ("Cancel")
        └── QLineEdit
## 2.注意事项
1. 核心机制: **父死子亡**,当parent被析构时,其children列表中的所有成员都会被析构
2. 注意事项:QT对象尽量在***堆***上进行***new***分配内存,在栈上分配会出现以下问题:
  ```C++
// ✅ 安全的写法：先父后子
int main() {
    QWidget window;                    // 父，先构造
    QPushButton quit("Quit", &window);  // 子，后构造
}
// 析构顺序与构造相反：quit 先析构（从 window 的 children 里移除自己）
// 然后 window 析构，此时 children 已空，不会重复删除
  
// ❌ 危险的写法：先子后父
int main() {
    QPushButton quit("Quit");          // 子，先构造
    QWidget window;                    // 父，后构造
    quit.setParent(&window);
}
// 析构顺序：window 先析构 → 它去 delete quit
// 然后 quit 作为局部变量又自动析构一次 → 双重释放，崩溃！
  ```
    问题主要出现在C++的析构顺序中:C++局部变量的析构顺序与其创建顺序相反,这样写会先析构window(由于对象树机制,会直接析构quit),之后quit又是局部变量,会被二次析构,程序崩溃