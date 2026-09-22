---
tags:
  - CPP/内存管理
  - CPP/内存模型
  - CPP/核心
date: 2026-09-22
---

# C++ 内存管理

承接 [[C++对象模型和虚函数|对象模型与虚函数]] 中"对象由数据成员与隐藏成员构成"的结论，本篇把视角下沉到内存本身：先看一个运行中的程序把内存分成哪些区域，再拆开 `new`/`delete` 与 `malloc`/`free` 的内部流程与区别，然后说明 `placement new` 的特殊之处，最后逐个拆解内存泄漏、悬空指针、use-after-free 与 double free 这几类经典陷阱。资源所有权与智能指针另见 [[RAII,资源所有权与unique_ptr|RAII、资源所有权与 unique_ptr]]。

> [!important] 核心主线
> **分配内存 ≠ 创建对象。**
> `new` = 申请原始内存 + 调用构造函数；`delete` = 调用析构函数 + 释放原始内存。
> `malloc`/`free` 只负责原始内存，不负责对象生命周期；`placement new` 则在已有内存上构造对象。记住这条分界线，后面几类内存错误都可以自己推导出来。

---

## 一、程序的内存分区

一个运行中的 C++ 程序，内存从概念上可划分为若干区域：

```text
高地址
┌──────────────────────┐
│   栈 Stack            │  局部变量、函数参数；向低地址增长
├──────────────────────┤
│        ↓              │
│      （空闲）          │
│        ↑              │
├──────────────────────┤
│   堆 Heap             │  new / malloc 动态申请；向高地址增长
├──────────────────────┤
│   全局/静态区          │  全局变量、static 变量
│   (.data / .bss)      │  已初始化 / 未初始化
├──────────────────────┤
│   常量区 (.rodata)     │  字符串字面量、const 常量等只读数据
├──────────────────────┤
│   代码区 (.text)       │  机器指令（函数代码）
└──────────────────────┘
低地址
```

> [!note] 说明
> 这是编译器与操作系统遵循的**通用概念模型**，各段的实际名称、位置与大小由具体平台和编译器决定，不必死记。
> 值得注意的是：函数代码存放在代码区，因此**不会计入每个对象的大小**（参见 [[对象大小,内存对齐和继承布局|对象大小、内存对齐与继承布局]]）。

### 1.1 常见易错点：指针在栈上，对象在堆上

```cpp
int* p = new int(23);
```

这里容易混淆两样东西：

- 指针变量 `p` 本身是一个局部变量，**通常存放在栈上**；
- `p` 指向的那个 `int` 对象由 `new` 在**堆上**申请，因此要由程序员负责释放。

```text
栈：  p ─────┐
             │ 8 字节（指针）
             ▼
堆：         [ int 对象 23 ]
```

---

## 二、对象的生命周期：内存与对象

### 2.1 对象是什么

一块原始内存只是几块字节。要让这块内存成为一个真正的**对象**，还需要：

- 类型；
- 数据成员；
- 构造函数与析构函数；
- 生命周期；
- 不变量（invariants，对象在任何时候都应保持成立的约束）。

正因如此，**申请到内存不等于创建了对象**——这正是理解 `new`、`malloc` 与 `placement new` 区别的关键。

### 2.2 `new` 的内部流程

```cpp
Camera* camera = new Camera();
```

上面这句 `new` 表达式，概念上完成两件事：

**（1）申请原始内存**

相当于调用：

```cpp
void* memory = operator new(sizeof(Camera)); // 只申请大小为 sizeof(Camera) 的原始内存
```

**（2）在原始内存上调用构造函数**

相当于：

```cpp
Camera* camera = new (memory) Camera(); // placement new：在已有内存上构造对象
```

> [!note] 关于那行"构造调用"的写法
> 构造函数没有可以直接书写的普通函数名，不能写成 `camera->Camera::Camera()` 这类伪语法。概念上更贴切的等价形式就是 **`placement new`**：`new (memory) Camera()`。

> [!warning] 补充：构造函数抛出异常怎么办
> 如果构造函数在申请内存之后抛出异常，`new` 表达式会自动调用匹配的 `operator delete` 把这块原始内存回收，因此**不会内存泄漏**。手写"先 `malloc` 再构造"则会丢掉这层保护。

### 2.3 `delete` 的内部流程

```cpp
delete camera;
```

概念上同样完成两件事，且**顺序与 `new` 相反**：

```cpp
camera->~Camera();        // 1. 调用析构函数：释放成员资源、关闭文件、释放对象内部申请的堆内存
operator delete(camera);  // 2. 释放原始内存
```

### 2.4 小结

```text
new：
1. 分配内存     （operator new）
2. 调用构造函数  （placement new）

delete：
1. 调用析构函数  （~Camera()）
2. 释放内存     （operator delete）
```

---

## 三、`malloc/free` 与 `new/delete` 的区别

### 3.1 `malloc`/`free` 只管原始内存

```cpp
#include <cstdlib>

void* memory = std::malloc(sizeof(Camera));        // 只申请原始内存
Camera* camera = static_cast<Camera*>(memory);      // 此时内存里还没有 Camera 对象
std::free(camera);                                  // 只释放原始内存
```

> [!warning] 注意
> `malloc` **不会调用构造函数**，`free` **也不会调用析构函数**。
> 因此上面的 `memory` 里并没有一个"活的" `Camera` 对象——在它上面访问成员属于**未定义行为**；即使勉强使用，对象内部申请的资源也无人释放，很容易造成内存泄漏。

### 3.2 对比表

| 对比项 | `malloc/free` | `new/delete` |
| --- | --- | --- |
| 所属体系 | C | C++ |
| 是否申请/释放内存 | 是 | 是 |
| 内存大小 | 需自己 `sizeof` 计算 | 自动计算 |
| 是否调用构造/析构函数 | 否 | 是 |
| 返回类型 | `void*`，需自行转换 | 正确的对象指针类型 |
| 失败行为 | 返回 `NULL` | 抛出 `std::bad_alloc` |
| 是否类型安全 | 否 | 是 |
| 能否被重载 | 否 | 是，可重载 `operator new` / `operator delete` |
| 数组语义 | 只分配连续内存块，不记录元素个数 | `new[]` / `delete[]` 配套，专为数组设计 |
| 配对要求 | `malloc` ↔ `free` | `new` ↔ `delete`，`new[]` ↔ `delete[]` |

### 3.3 混用是未定义行为

下面几种写法都属于**未定义行为**，务必避免：

```cpp
int* p1 = new int(1);
std::free(p1);            // 错误：new 配 free

int* p2 = static_cast<int*>(std::malloc(sizeof(int)));
delete p2;                // 错误：malloc 配 delete

int* arr = new int[10];
delete arr;               // 错误：new[] 必须配 delete[]
```

---

## 四、`placement new`：在已有内存上构造对象

相较于 `new` 负责 ***申请内存 + 构造对象***，`placement new` **只负责在已有内存上构造对象**。

### 4.1 示例

```cpp
#include <new>
#include <cstddef>
#include <iostream>

class Camera {
public:
    Camera()  { std::cout << "Camera 构造\n"; }
    ~Camera() { std::cout << "Camera 析构\n"; }
};

int main() {
    // 1. 提前准备一块大小足够、对齐正确的原始内存
    alignas(Camera) std::byte buffer[sizeof(Camera)];

    // 2. 在这块内存上构造对象，不申请新内存
    Camera* camera = new (buffer) Camera();

    // 使用 camera ...

    // 3. 只能手动调用析构函数，不能用 delete
    camera->~Camera();
}
```

要点：

- `alignas(Camera)` 保证 `buffer` 的对齐满足 `Camera` 的要求，否则是未定义行为；
- `buffer` 的大小与**存储期**由提供方负责，`delete` / `free` 都不能用来归还它。

### 4.2 使用场景

主要用于：

- 内存池；
- 自定义分配器；
- 高性能容器；
- 操作系统底层开发；
- 联合体（union）；
- 共享内存。

> [!tip] 一般业务代码无需使用
> 日常开发中，标准容器（如 `std::vector`）与智能指针（如 `std::make_unique`）已经替你处理好了"内存 + 构造"这两步，见 [[RAII,资源所有权与unique_ptr|RAII、资源所有权与 unique_ptr]]。

---

## 五、内存泄漏

**内存泄漏**指：申请了一块内存，却始终没有释放，并且再也拿不到指向它的指针。

```cpp
void leak() {
    int* p = new int(10);
    // 忘记 delete p
} // 函数结束，p 被销毁，但堆上那块内存再也没有指针指向它
```

常见原因：

- 忘记 `delete`；
- 在 `delete` 之前提前 `return` 或抛出异常，跳过了释放语句；
- 用 `new[]` 申请却用 `delete`（或反向）释放——这是未定义行为，也可能连带引发泄漏。

> [!tip] 根本解法
> 优先使用**栈对象、标准容器和 RAII**：把资源交给对象的析构函数管理，就不会因为某条返回路径被跳过而泄漏。

---

## 六、悬空指针与野指针

这两者常被混为一谈，其实**成因完全不同**，需要分开理解。

### 6.1 悬空指针（dangling pointer）

**指针保存的地址仍然有效，但该地址上的对象已经不存在。**

```cpp
int* p = new int(10);
delete p;
// p 仍保存着原来的地址，但那个 int 对象已经被释放
std::cout << *p; // 错误：访问已释放的内存，属于未定义行为
```

### 6.2 野指针（wild pointer）

**指针从未被初始化，指向一个随机的、不可预测的地址。**

```cpp
int* p;   // 未初始化，p 的值是垃圾数据
*p = 10;  // 错误：向未知地址写入，未定义行为
```

### 6.3 如何规避

```cpp
int* p = new int(10);
delete p;
p = nullptr; // 悬空指针置空，防止再次误用
```

> [!note] 说明
> `delete nullptr;` 是**安全**的（什么也不做），所以"删除后置空"是一种廉价而有效的防御手段。但要注意：**空指针解引用同样会崩溃**，置空之后仍需在使用前判空。
> 另外，尽量在**定义时立即初始化**指针，可以避免野指针。

---

## 七、use-after-free

**use-after-free** 指对**已经释放的内存**继续访问。

```cpp
int* p = new int(10);
delete p;
*p = 20; // 错误：对象已释放，属于未定义行为
```

在工业视觉项目中，这类问题尤其隐蔽：

```text
采集线程：释放了图像缓冲区
推理线程：仍然在使用这块缓冲区
         → use-after-free
```

> [!warning] 为什么危险
> 释放后的内存可能已被重新分配给别的对象，此时写入会**破坏无关数据**；而且程序往往不会立刻崩溃，bug 可能在很久之后才以奇怪的方式暴露，极难排查。

---

## 八、double free

**double free** 指**同一块内存被释放两次**。

```cpp
int* p = new int(10);
delete p;
delete p; // 错误：同一块内存被释放两次，未定义行为
```

更常见的场景是**所有权不清晰**：一个对象的生命周期已经交给了另一个对象处理，自己又释放了一遍。

> [!warning] 实例：Qt 的 `WA_DeleteOnClose`
> 在**栈上**创建 `QDialog`（如 `InboundEditDialog dialog(...)`）的同时，又在构造函数中设置了 `WA_DeleteOnClose`，就形成了"双重所有权"：
> - 对话框关闭时，Qt 通过 `deleteLater()` 主动删除它；
> - 退出作用域时，栈对象又被自动析构。
>
> 两方都认为自己拥有这个对象，于是**重复释放导致崩溃**。正确做法是：`WA_DeleteOnClose` 只配合**堆分配**使用（非模态 `open()` 用 `new`，模态 `exec()` 用栈对象且不设该属性）。
> 详见 [[2026-09-11 Day25 完成InventoryPage及其基础板块的编写#^e9c434]] 与 [[QT对话框的WA_DeleteOnClose]]。

> [!important] 自定义资源管理类必须考虑
> - **Rule of Three（三法则）**：析构函数 + 拷贝构造函数 + 拷贝赋值运算符；
> - **Rule of Five（五法则）**：在三法则基础上，再加上移动构造函数 + 移动赋值运算符。
>
> 一旦类自己持有资源（例如裸指针），就必须明确这五个函数的行为。否则编译器生成的默认浅拷贝会让两个对象指向同一块内存，析构时就会 double free。
> 更省事的做法是**把资源交给 `std::unique_ptr` / `std::shared_ptr` 托管**，让标准库来承担这份责任。

---

## 九、核心总结

### 9.1 一句话主线

```text
内存分区   → 栈 / 堆 / 全局静态 / 常量 / 代码
new       → operator new 申请原始内存 + placement new 调用构造函数
delete    → 调用析构函数 + operator delete 释放原始内存
malloc    → 只申请原始内存，不构造对象
placement new → 在已有内存上构造，需手动析构
经典陷阱   → 内存泄漏、悬空/野指针、use-after-free、double free
```

### 9.2 速查表

| 问题 | 答案 |
| --- | --- |
| `new` 做了哪两件事？ | 申请原始内存 + 调用构造函数 |
| `delete` 做了哪两件事？ | 调用析构函数 + 释放原始内存 |
| `malloc`/`free` 会调用构造/析构函数吗？ | 不会，只管理原始内存 |
| `new` 与 `malloc` 失败时分别怎样？ | `new` 抛 `std::bad_alloc`；`malloc` 返回 `NULL` |
| `new` 用 `free`、`malloc` 用 `delete` 可以吗？ | 不可以，属于未定义行为 |
| `new[]` 要用什么释放？ | `delete[]`，用 `delete` 是未定义行为 |
| `placement new` 与普通 `new` 有何不同？ | 只在已有内存上构造对象，不申请内存 |
| `placement new` 构造的对象怎么销毁？ | 手动调用析构函数，不能用 `delete` / `free` |
| 悬空指针与野指针的区别？ | 悬空指针指向已释放的对象；野指针从未初始化 |
| 什么是 use-after-free？ | 释放后继续访问那块内存 |
| 什么是 double free？ | 同一块内存被释放两次 |
| 怎样减少这类错误？ | RAII + 智能指针，优先用栈对象与标准容器 |

### 9.3 易错点

- `new` 配 `delete`、`new[]` 配 `delete[]`，`malloc`/`free` 与 `new`/`delete` 不能交叉使用。
- `delete` 后把指针置为 `nullptr`（`delete nullptr` 是安全的），但置空后仍要防止解引用。
- 不要为了省事把栈对象交给会主动 `delete` 它的机制（如 Qt 的 `WA_DeleteOnClose`）。
- 类一旦自己持有资源，就要按 Rule of Three / Rule of Five 明确拷贝、移动与析构行为。
- 别忘了一个 `new` 表达式其实包含"申请内存"和"构造对象"两步——这是整个 `new`/`malloc`/`placement new` 体系的分水岭。

---

> [!tip] 延伸
> 相关学习记录见 [[C++深入/学习进度.md]]，对象的构造、析构与多态基类虚析构函数见 [[C++对象模型和虚函数]]，资源所有权与智能指针见 [[RAII,资源所有权与unique_ptr|RAII、资源所有权与 unique_ptr]]。
