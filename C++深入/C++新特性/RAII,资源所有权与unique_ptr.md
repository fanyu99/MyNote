---
tags:
  - CPP/RAII
  - CPP/智能指针
  - CPP/核心
date: 2026-09-21
---

# RAII、资源所有权与 unique_ptr

承接 [[对象切片,类型转换,dynamic_cast|对象切片、类型转换与 dynamic_cast]] 中"多态容器应保存指针"的结论，本篇回答这些指针"由谁负责释放"：先讲 RAII 与资源所有权的思想，再依次说明 `unique_ptr`（独占）、`shared_ptr`（共享）与 `weak_ptr`（观察）如何使用，以及各自最容易踩的坑。

> [!important] 核心主线
> **RAII：把资源的生命周期绑定到对象的生命周期——构造时获取资源，析构时释放资源。**
> 所有权回答"谁负责最终释放"；智能指针则把所有权**用类型表达出来**：`unique_ptr` 独占、`shared_ptr` 共享、`weak_ptr` 只观察不拥有。

---

## 一、RAII：资源获取即初始化

### 1.1 RAII 的要求

`RAII` 即 **Resource Acquisition Is Initialization**，核心思想是：**把资源的生命周期绑定到对象的生命周期**。

> [!important] 要求
> - 对象**构造时**获取资源；
> - 对象**析构时**释放资源。
>
> 其价值在于：**把"释放资源"交给对象的析构函数自动完成**，不再依赖程序员在每条返回路径上手动清理。

### 1.2 RAII 依赖的语言规则

RAII 能够成立，依赖 C++ 的一条重要规则：

```text
局部对象离开作用域时，会自动调用其析构函数；
并且析构顺序与构造顺序相反。
```

这里的"作用域"可以是：函数体、`if`、`for`、`while`，以及任意创建的 `{}` 块。因此无论从哪条路径离开作用域（正常返回、`break`、`continue`，甚至抛出异常），析构都会被执行——这正是 RAII 能取代手写 `delete` 的根本原因。

---

## 二、什么是资源所有权

**资源所有权**回答一个问题：*谁负责最终的资源释放*。

围绕所有权，设计接口时需要想清楚下面几件事：

1. 谁拥有对象？
2. 谁负责析构对象？
3. 所有权能否转移？
4. 是否允许多个对象共同拥有它？
5. 这个指针是否只是临时观察，不负责释放？

> [!note] 说明
> 现代 C++ 的做法是把这些答案**编码进类型**：独占用 `std::unique_ptr`，共享用 `std::shared_ptr`，只观察则用裸指针 `T*` 或 `std::weak_ptr`。

---

## 三、`std::unique_ptr`：独占智能指针

同一时间**只能有一个** `unique_ptr` 拥有某个对象；离开作用域时，它自动释放所拥有的对象资源。

### 3.1 基本使用

```cpp
#include <memory>

auto p = std::make_unique<int>(89);
```

### 3.2 函数如何接收 unique_ptr

**（1）按值接收 → 接管所有权**

```cpp
void consume(std::unique_ptr<int> p) {
    std::cout << *p << '\n';
}

// 使用
consume(std::move(p)); // 必须 std::move：所有权转移到参数，调用后 p 变为 nullptr
```

**（2）按引用接收 → 不接管所有权**

```cpp
void print(const std::unique_ptr<int>& p) {
    if (p) {
        std::cout << *p << '\n';
    }
}

// 使用
print(p); // 并没有发生所有权转移
```

> [!tip] 选择原则
> 只在**确实要转移所有权**时才按值接收；如果只是想使用对象本身，优先传 `const T&` 或 `T*`（上例的 `print` 更推荐写成 `print(const int&)`）。

### 3.3 重要性质：不能复制，只能移动

`unique_ptr` **不能被复制，只能被移动**：

```cpp
auto p  = std::make_unique<int>(89);
auto p1 = std::move(p); // 所有权转移：p1 接管对象，p 变为 nullptr
```

> [!warning] std::move 本身不执行移动
> `std::move` **不执行任何移动**，它只是把对象转换成"可被移动的右值引用"形式；真正的移动由**移动构造函数**和**移动赋值运算符**完成。

### 3.4 `std::unique_ptr` 与多态

`std::unique_ptr` 是保存多态对象的常用方式：它满足 RAII、能自动释放派生类对象，且**不会发生对象切片**（参见 [[对象切片,类型转换,dynamic_cast|对象切片、类型转换与 dynamic_cast]]）。

```cpp
std::vector<std::unique_ptr<Animal>> animals;
animals.push_back(std::make_unique<Dog>());
```

> [!note] 前提
> 通过基类指针 `delete` 派生类对象时，基类**必须有虚析构函数**，否则派生类部分不会被正确析构。

### 3.5 常用操作

1. `reset()`：释放当前所拥有的对象，并改为拥有传入的新对象（不传参则变为空）；
2. `release()`：放弃所有权并返回裸指针，但**不释放对象**；*(尽量减少使用,主要用于与旧式C接口交互)*
3. `get()`：获取所保存的裸指针，不会转移所有权。

| 操作 | 是否释放对象 | `unique_ptr` 是否继续拥有 | 主要用途 |
|---|---|---|---|
| `get()` | 否 | 是 | 临时获取裸指针观察对象 |
| `release()` | 否 | 否 | 转移给不接收 `unique_ptr` 的旧接口 |
| `reset()` | 是（原对象） | 否，或拥有新对象 | 释放对象或替换所管理的对象 |

> [!warning] release() 之后的责任
> `release()` 返回的裸指针**由调用者负责 `delete`**；忘记释放就会内存泄露。

### 3.6 不要让 `unique_ptr` 管理栈对象

```cpp
int a = 29;
std::unique_ptr<int> p(&a); // 危险：p 析构时会执行 delete &a
```

> [!warning] 这是未定义行为，不是"无法执行"
> `unique_ptr` 内部会用 `delete` 去释放它管理的对象，而 `&a` 指向的是**栈上对象**。对栈对象执行 `delete` 属于**未定义行为**——程序可能崩溃，也可能看似正常，不能依赖其结果。
> 结论：**`unique_ptr` 只能管理由 `new` 得到的（堆）对象**，请优先使用 `std::make_unique`。

### 3.7 不要手动删除它管理的对象

一旦把对象交给 `unique_ptr`，就不要再手动 `delete` 该对象，否则会造成 **double free**（重复释放）——这类 bug 很难排查。

---

## 四、`std::shared_ptr`：共享智能指针

`shared_ptr` 表示**多个 `shared_ptr` 共同拥有同一个对象**。

其特点为：

1. 可以复制；
2. 内部维护**引用计数**；
3. 引用计数为 0 时自动释放对象；
4. 管理成本较高（引用计数 + 控制块）；
5. 使用不当会产生**循环引用**。

### 4.1 基本使用

```cpp
#include <memory>

auto p1 = std::make_shared<int>(34);
auto p2 = p1; // 引用计数 +1，两者共同拥有同一个对象
```

> [!tip] 优先使用 make_shared
> `std::make_shared<T>(...)` 把对象和控制块一次分配，通常比 `std::shared_ptr<T>(new T(...))` 更高效、也更安全。

### 4.2 循环引用及其解决方法

如果多个对象用 `shared_ptr` **互相持有**，就形成环：每个对象的引用计数都无法降为 0，于是谁也不会被释放，造成**内存泄露**。

```cpp
#include <memory>

struct child; // 前置声明：parent 中的 shared_ptr<child> 需要它

struct parent {
    std::shared_ptr<child> c; // 拥有其子节点
};

struct child {
    std::shared_ptr<parent> p; // 拥有其父节点
};

int main() {
    auto par = std::make_shared<parent>();
    auto ch  = std::make_shared<child>();

    par->c = ch;  // parent 拥有 child
    ch->p  = par; // child 拥有 parent → 互相持有，引用计数都无法归零
}
```

```text
par ──shared_ptr──▶ child
 ▲                    │
 └──────shared_ptr────┘
   引用计数永不归零 → 两个对象都不会被释放
```

> [!warning] 修正：把其中一个方向改成 weak_ptr
> 解决方法是让**其中一个方向**（通常是"反向"的父子指针）改用 `std::weak_ptr`，它**不增加引用计数**，因而不会形成环。

```cpp
struct child {
    std::weak_ptr<parent> p; // 只观察，不拥有
};
```

### 4.3 如何用 `weak_ptr` 访问对象

`weak_ptr` 不能直接访问对象，必须先用 `lock()` 提升为 `shared_ptr`（此时需要对象仍然存活）：

```cpp
if (auto sp = ch->p.lock()) { // lock() 返回 shared_ptr；对象还活着则非空
    // 在这里可以安全访问 sp
} else {
    // 对象已被释放
}
```

> [!warning] weak_ptr 是"非拥有"的智能指针
> `weak_ptr` 本身**是智能指针家族的一员**，但它是**非拥有（non-owning）**的：它不参与引用计数，**不会**自动管理对象的生命周期。
> 因此不能把它当作"能自动管理内存"的工具，它主要作为配合 `shared_ptr` **打破循环引用**、或安全"观察"某个对象是否还活着的工具。

---

## 五、核心总结

### 5.1 一句话主线

```text
RAII         → 构造获取、析构释放，把释放责任交给析构函数
所有权       → 谁负责最终释放；用类型表达：独占 / 共享 / 观察
unique_ptr   → 独占、只能移动、不能复制；栈对象不可托管
shared_ptr   → 共享、引用计数；注意循环引用
weak_ptr     → 非拥有、不计数；lock() 后访问，用于打破环
```

### 5.2 速查表

| 问题 | 答案 |
| --- | --- |
| RAII 的两个要求？ | 构造时获取资源，析构时释放资源 |
| 资源所有权回答什么问题？ | 谁负责最终的资源释放 |
| `unique_ptr` 能复制吗？ | 不能，只能移动（`std::move`） |
| `std::move` 会移动对象吗？ | 不会，它只把对象转换为可移动的右值引用 |
| `get` / `release` / `reset` 的区别？ | 观察 / 放弃所有权但不释放 / 释放并可替换 |
| 为什么不能托管栈对象？ | `unique_ptr` 会用 `delete`，对栈对象是未定义行为 |
| `shared_ptr` 何时释放对象？ | 引用计数降为 0 时 |
| 什么是循环引用？ | 对象互相用 `shared_ptr` 持有，引用计数无法归零 |
| 怎么打破循环引用？ | 让其中一个方向改用 `std::weak_ptr` |
| `weak_ptr` 怎么访问对象？ | `lock()` 提升为 `shared_ptr` 后访问，并判空 |
| `weak_ptr` 会延长对象寿命吗？ | 不会，它是非拥有指针，不增加引用计数 |

### 5.3 易错点

- `unique_ptr` 只能管理堆对象，用 `&栈变量` 构造会导致析构时 `delete` 栈地址（未定义行为）。
- 交给 `unique_ptr` 后不要再手动 `delete`，否则可能 double free。
- `release()` 不会释放资源，别忘了接手它返回的裸指针。
- `shared_ptr` 互相持有会循环引用，务必把其中一个方向改成 `weak_ptr`。
- `weak_ptr` 不能直接解引用，必须先 `lock()` 再判空。

---

> [!tip] 延伸
> 相关学习记录见 [[C++深入/学习进度.md]]，上一篇见 [[对象切片,类型转换,dynamic_cast|对象切片、类型转换与 dynamic_cast]]，其中已出现 `std::vector<std::unique_ptr<Base>>` 的用法。
