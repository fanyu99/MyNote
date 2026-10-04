---
tags:
  - CPP/模板
  - CPP/类型推导
  - CPP/核心
date: 2026-09-23
---

# C++ 函数模板与类型推导

承接 [[右值引用,移动语义,完美转发|右值引用、移动语义与完美转发]] 中"万能引用 + 引用折叠"的结论，本篇把模板自身的规则补齐：先说明模板成立的前提，再梳理函数模板的实参推导、类模板与非类型模板参数、特化与偏特化，最后落到 `SFINAE`、`concepts`，并回到线程池 `submit` 的模板实现。

> [!important] 核心主线
> **模板把"类型"变成参数：编译器在编译期用具体类型替换模板参数，为每个用到的类型各生成一份代码（实例化）。**
> 由此推出三条主线：模板体里的操作必须对推导出的类型有效；实参推导只做"求同"，不会为了让两个实参一致而隐式转换；而模板中的 `T&&` 是**万能引用**，配合 `std::forward` 实现完美转发。

---

## 一、函数模板的使用条件

模板体里的操作必须对推导出的类型有效：

```cpp
template <typename T>
T maxValue(T a, T b) {
    return a > b ? a : b;
}
```

这里要求：`T` 必须支持 `>` 运算，并且 `a > b ? a : b` 的结果类型能够转换为返回类型 `T`。

> [!note] 为什么会有这个约束
> 模板在**编译期**按具体类型实例化。传入的类型若不具备模板体用到的操作（运算符、成员函数等），实例化就会失败——报错出现在**实例化点**，而不是模板定义处。

---

## 二、函数模板的实参推导

模板实参推导时，编译器**不会**为了让两个实参类型相同，就把其中一个转换成另一个类型：

```cpp
template <typename T>
void show(T a, T b) { /* ... */ }

show(3, 4);           // 推导成功：T = int
show(3, 5.3);         // 推导失败：3 → int，5.3 → double，两者冲突
show<double>(3, 4.5); // 显式指定 T = double，此时 3 才隐式转换为 double
```

> [!tip] 两个阶段要分清
> "推导阶段"不做实参之间的类型统一；一旦**显式写出**模板实参（`show<double>`），`T` 就被固定，此后普通的函数调用转换（`int` → `double`）才被允许。

---

## 三、类模板

```cpp
#include <iostream>

template <typename T>
class Box {
public:
    explicit Box(T value) : value_(value) {}
    const T& get() const { return value_; }
private:
    T value_;
};

int main() {
    Box<int> intBox(42);
    Box<double> doubleBox(3.14);
    std::cout << intBox.get() << '\n';
    std::cout << doubleBox.get() << '\n';
}
```

> [!note] 与函数模板的区别
> 类模板通常**不能**省略模板实参（C++17 的 CTAD 是例外）：`Box b(42);` 在 C++17 之前必须写成 `Box<int> b(42);`。并且 `Box<int>` 与 `Box<double>` 是**两个彼此独立的类型**。

---

## 四、引用、`const` 与数组的推导

形参形式直接决定推导结果，先看总表：

| 形参形式 | 传入 `int x` | 传入 `const int cx` | 传入右值 `10` | 特点 |
|---|---|---|---|---|
| `T`（值传递） | `T = int` | `T = int`（丢弃顶层 `const`） | `T = int` | 发生拷贝；忽略顶层 `const` |
| `T&`（左值引用） | `T = int` | `T = const int` | ✗ 无法绑定 | 保留引用与 `const` 信息 |
| `const T&` | `T = int` | `T = int` | `T = int` ✓ | 可绑定临时对象，并"吸收" `const` |

### 4.1 值传递通常会忽略顶层的 `const`

```cpp
template <typename T>
void byValue(T value) {}   // 值传参

const int x = 10;
byValue(x);                // T 推导为 int，而不是 const int
```

### 4.2 左值引用参数会保留引用和 `const` 信息

```cpp
template <typename T>
void byReference(T& value) {}   // 左值引用

int x = 10;
const int cx = 20;

byReference(x);   // T = int
byReference(cx);  // T = const int
```

### 4.3 `const T&` 既能接受临时对象，又保留 `const` 信息

```cpp
template <typename T>
void byConstReference(const T& value) {}   // const 左值引用

int x = 10;
const int cx = 20;

byConstReference(x);   // T = int
byConstReference(cx);  // T = int（const 被形参自身的 const 吸收）
byConstReference(10);  // T = int（可绑定临时对象）
```

> [!important] 三种形式的取舍
> - `T`：**需要一份副本**时用，代价是拷贝，且丢失引用与顶层 `const`。
> - `T&`：要**修改**实参，或想保留实参的 `const` 属性。
> - `const T&`：**只读且不想拷贝**，同时要能接受临时对象/字面量——最常用的"只读形参"。

### 4.4 数组参数

**（1）值传递 → 退化为指针**

```cpp
template <typename T>
void byValue(T value) {}

int numbers[5]{};
byValue(numbers);   // T 推导为 int*，数组长度信息丢失
```

**（2）数组引用 → 保留长度**

```cpp
#include <cstddef>

template <typename T, std::size_t N>
void showArray(const T (&array)[N]) {
    std::cout << "元素数量：" << N << '\n';
}

int numbers[5]{};
showArray(numbers);   // T = int，N = 5
```

> [!tip] 用途
> `const T (&)[N]` 让编译器**在编译期**求出数组长度，是"遍历定长数组而不必额外传长度"的经典写法。

---

## 五、模板参数不只有类型

模板参数除类型参数外，还可以是**非类型模板参数**——即编译期常量，例如数组容量：

```cpp
#include <cstddef>

template <typename T, std::size_t Capacity>
class FixedBuffer {
public:
    T& operator[](std::size_t index) { return data_[index]; }
private:
    T data_[Capacity];
};
```

> [!note] 非类型模板参数的要求
> 它必须是**编译期常量表达式**，且类型受限：C++20 之前只能是整型、枚举、指针、左值引用等；C++20 起放宽到浮点数、字面类型等。

---

## 六、特化与偏特化

### 6.1 全特化：针对一个完全确定的类型

```cpp
// 主模板：其他类型都走这里
template <typename T>
struct TypeName {
    static const char* get() {
        return "other";
    }
};

// 全特化：int 类型专门使用
template <>
struct TypeName<int> {
    static const char* get() {
        return "int";
    }
};
```

> [!warning] 全特化必须写完整
> 全特化是"把主模板的某个参数固定为具体类型后，**重新给出整个模板**"。它必须写成 `template <> struct TypeName<int> { ... };`，把 `struct TypeName<int>` 的类头写全——只写成员函数体是不合法的。

### 6.2 偏特化：只固定一部分参数，或限制类型形状

类模板可以偏特化。通用模板：

```cpp
template <typename T>
struct TypeCategory {          // 不限制类型，通用版本
    static const char* get() {
        return "普通类型";
    }
};
```

针对指针类型的偏特化：

```cpp
template <typename T>
struct TypeCategory<T*> {      // 限制为 T*，对指针类型偏特化
    static const char* get() {
        return "指针类型";
    }
};
```

### 6.3 函数模板：用重载代替偏特化

**函数模板不支持**类模板那种形式的偏特化，一般用**函数重载**实现同类需求：

```cpp
template <typename T>
void show(const T&) {
    std::cout << "通用类型";
}

void show(const char*) {   // 针对字符串字面量，用重载处理
    std::cout << "字符串";
}
```

> [!记住]
> - 类模板支持偏特化；
> - 函数模板通常用**重载**解决"针对特定类型走不同实现"的问题。

偏特化的常见用途：

- 对指针 / 数组 / 引用等类型采用不同的处理方式；
- 根据类型形状选择不同实现（如判别是否为指针）。

---

## 七、`SFINAE` 与 `concepts`

### 7.1 `SFINAE`：替换失败，不报错

`SFINAE`（Substitution Failure Is Not An Error）：当编译器尝试把模板参数代入时，如果某个候选模板因类型不合适而**替换失败**，编译器可以**丢弃这个候选而不立即报错**；只要还存在可用候选，就继续选择。

```cpp
#include <type_traits>
#include <iostream>

template <
    typename T,
    typename = std::enable_if_t<std::is_integral_v<T>>   // 使该模板仅适用于「整型」
>
void process(T value) {
    std::cout << "处理整数\n";
}

int main() {
    process(10);   // 调用成功
    process(3.5);  // 该候选被排除 → 无匹配函数，编译报错
}
```

> [!note] `SFINAE` 的作用边界
> `SFINAE` **不是**让非法调用神奇地成功，而是**把不适用的模板候选排除在重载选择之外**。
> 所以如果只有上面这一个版本，`process(3.5)` 最终仍会报"没有匹配的函数"——这正是它的价值：为"与其他重载组合"创造条件。

### 7.2 与其它重载组合：`enable_if` 的正确写法

要把"整型路径 / 浮点路径"拆成两个重载，**不能**写成"只差默认模板实参"的形式——那会被判定为**重定义**：

```cpp
// ✗ 错误示范：两个模板签名相同 → 重定义，编译失败
template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
void process(T) { /* ... */ }

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
void process(T) { /* ... */ }
```

正确做法是把 `enable_if` 放到**非类型模板形参**上，让两个模板的参数列表真正不同：

```cpp
#include <type_traits>
#include <iostream>

template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void process(T) {
    std::cout << "整数路径\n";
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
void process(T) {
    std::cout << "浮点数路径\n";
}

int main() {
    process(1);    // 整数路径
    process(1.5);  // 浮点数路径
}
```

> [!warning] 这是最容易踩的坑
> **默认模板实参不区分重载**：两个函数模板若只有默认模板实参不同，会被编译器视为同一个签名，直接报 `redefinition`。
> 因此 SFINAE 重载要借助**非类型模板形参**（`... , int> = 0`）或返回类型来区分；C++20 起更推荐直接用 `concepts`。

### 7.3 C++20 `<concepts>`：更直接的约束

```cpp
#include <concepts>
#include <iostream>

template <std::integral T>   // 直接把约束写在模板形参上
void process(T value) {
    std::cout << "处理整数\n";
}
```

`std::integral` 明确声明了 `T` 必须满足的类型要求，报错信息也远比 `enable_if` 清晰，是 `SFINAE` 的现代替代方案。

---

## 八、模板与完美转发

模板参数推导会把 `T&&` 变成**万能引用**，这是完美转发的基础（详见 [[右值引用,移动语义,完美转发|右值引用、移动语义与完美转发]]）：

```cpp
template <typename T>
void wrapper(T&& arg) {          // 万能引用，不是普通右值引用
    foo(std::forward<T>(arg));   // 按 T 的推导结果决定是否移动
}

// 传入左值：T 推导为 int& → arg 类型 int& && 折叠为 int&  （保持左值）
// 传入右值：T 推导为 int  → arg 类型 int&&                （保持右值）
```

> [!important] 三条要点
> 1. 模板中 `T&&` 是**万能引用**：传入左值时 `T` 推导为左值引用类型，传入右值时 `T` 推导为普通类型；
> 2. `std::forward<T>` 依据推导结果**保留参数原来的左值 / 右值属性**；
> 3. `&&` 只有在"模板参数推导"语境下才是万能引用——`const T&&`、`std::vector<T>&&` 都不是。

---

## 九、实践：线程池中的重复逻辑抽成模板

在我的线程池实现（[fanyu99/ThreadPool](https://github.com/fanyu99/ThreadPool)）中，入队 / 处理等重复逻辑都用模板表达。以提交任务为例：

```cpp
// 模板参数 F 为可调用对象类型，Args 为参数包
template <typename F, typename... Args>
auto submit(F&& f, Args&&... args)
{
    using return_type = std::invoke_result_t<F, Args...>;   // 推导返回值类型

    // （此处的局部加锁见下方「注意」）
    {
        std::mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        if (stop.load()) {
            throw std::runtime_error("ThreadPool is stopped!\n");
        }
    }

    // 无返回值：直接把任务加入队列
    if constexpr (std::is_void_v<return_type>)   // 编译期分支，只实例化命中的一支
    {
        // 用元组绑定参数，再用 apply 展开调用
        auto task = [func = std::forward<F>(f),
                     argu = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            std::apply(func, argu);
        };
        if (!taskqueue->push(std::move(task)))   // lambda 不可拷贝，用移动语义
        {
            Logger::instance().error("Task submission failed!");
            throw std::runtime_error("Task submit failed!\n");
        }
        totalTasks.fetch_add(1);
        Logger::instance().debug("Task submitted, total tasks: " +
                                 std::to_string(getTotalTasksCount()));
    }
    // 有返回值：用 packaged_task / future 回传结果
    else
    {
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [func = std::forward<F>(f),
             argu = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(func, argu);
            });   // 用 shared_ptr 保证任务在异步执行期间存活

        std::future<return_type> resfuture = task->get_future();
        if (!taskqueue->push([task]() { (*task)(); }))
        {
            // 提交失败：返回一个已带异常的 future
            Logger::instance().error("Task submission failed!");
            std::promise<return_type> promise;
            promise.set_exception(
                std::make_exception_ptr(std::runtime_error("Task submit failed!\n")));
            return promise.get_future();
        }
        totalTasks.fetch_add(1);
        Logger::instance().debug("Task submitted, total tasks: " +
                                 std::to_string(getTotalTasksCount()));
        return resfuture;
    }
}
```

> [!warning] 注意：函数内的局部 `std::mutex` 起不到同步作用
> `std::mutex mtx;` 定义在**函数内部**，每次调用都会新建一把锁，因此它保护的临界区并没有被任何其它线程互斥——等于没加锁。
> 要真正互斥，互斥量必须作为**成员变量**（如 `std::mutex queueMutex_;`）与其保护的数据一同存在，或直接依赖队列自身的加锁；若 `stop` 本身就是 `std::atomic<bool>`，`stop.load()` 已是原子操作，这段加锁可以直接删去。

> [!note] 这段代码用到的模板知识
> - `std::invoke_result_t<F, Args...>`：用模板元编程求出**调用结果类型**，从而知道该返回 `void` 还是 `future<T>`；
> - `if constexpr`：**编译期**分支，两条路径中只会实例化命中那条，因此无返回值分支里的 `return` 语句不会造成类型冲突；
> - `F&& ... Args&&`：万能引用 + 参数包，配合 `std::forward` 把参数原样转交；
> - `std::make_tuple` + `std::apply`：把参数打包，再在真正调用时展开——这是"把可调用对象延迟到线程里执行"的常见手法。

---

## 十、对比：C++ 模板 vs Qt 元对象系统

两者都和"类型"打交道，但属于**完全不同的机制**：

| | C++ 模板 | Qt 元对象系统 |
|---|---|---|
| 发生时机 | **编译期**实例化 | **运行期**反射 |
| 依赖 | 编译器直接生成代码 | `moc` 预处理 + `QMetaObject` 等运行时数据 |
| 主要能力 | 针对不同类型生成通用代码 | `QObject` 派生类的运行时类型信息、信号槽、属性系统 |
| 典型工具 | `template`、`concepts`、`SFINAE` | `QMetaObject`、`QMetaMethod`、`Q_PROPERTY` |

```text
C++ 模板        → 解决"编译期如何针对不同类型生成通用代码"；
Qt 元对象系统   → 解决"运行期如何识别和处理 QObject、信号槽、属性等元信息"。
```

---

## 十一、核心总结

### 11.1 一句话主线

```text
模板的前提   → 模板体里的操作必须对推导出的类型有效
实参推导     → 只求同、不做实参间转换；显式指定模板实参后才允许转换
形参形式     → T 丢引用/顶层 const；T& 保留；const T& 吸收 const 且可绑临时对象
数组         → 值传递退化为指针；数组引用 const T (&)[N] 保留长度 N
非类型参数   → 模板参数还可是编译期常量（容量、整数、指针等）
特化         → 类模板可全特化/偏特化；函数模板用重载代替偏特化
SFINAE       → 替换失败只丢候选不报错；重载要借非类型模板形参区分
concepts     → C++20 直接约束模板形参，替代 enable_if
万能引用     → 模板中的 T&&；配合 std::forward 实现完美转发
```

### 11.2 速查表

| 问题 | 答案 |
| --- | --- |
| 模板体里的操作有什么要求？ | 必须对推导出的类型有效，否则实例化失败 |
| `show(3, 5.3)` 为什么推导失败？ | 单一 `T` 推出 `int` 与 `double`，冲突；推导不做实参间转换 |
| `show<double>(3, 4.5)` 呢？ | `T` 固定为 `double`，`3` 在调用时隐式转换 |
| 值传递会保留顶层 `const` 吗？ | 不会，`const int` 推出 `T = int` |
| `T&` 保留 `const` 吗？ | 保留，`const int` 推出 `T = const int` |
| `const T&` 对 `const int` 推出什么？ | `T = int`（`const` 被形参吸收），且可绑临时对象 |
| 数组按值传递会怎样？ | 退化为指针，`T = int*`，长度丢失 |
| 怎样保留数组长度？ | 用数组引用 `const T (&)[N]`，推出 `N = 5` |
| 模板参数只能是类型吗？ | 不是，还可是非类型模板参数（编译期常量） |
| 类模板能偏特化吗？ | 能 |
| 函数模板能偏特化吗？ | 不能，用函数重载代替 |
| `SFINAE` 是什么？ | 替换失败只丢弃候选、不报错，用于把不适用的模板排除出重载集 |
| `enable_if` 写两个重载要注意什么？ | 默认模板实参形式会**重定义**；要用非类型模板形参 `..., int> = 0` |
| C++20 用什么替代 `SFINAE`？ | `concepts`（如 `template <std::integral T>`） |
| 模板中的 `T&&` 是什么？ | 万能引用；左值推为 `T&`，右值推为 `T` |
| 模板什么时候实例化？ | 编译期 |

### 11.3 易错点

- **全特化要写完整**：必须是 `template <> struct TypeName<int> { ... };`，把类头写全，不能只写成员。
- **`std::is_integral_v<T>` 覆盖所有整型**（`char`、`bool`、`long`…），不是"仅限 `int`"。
- **默认模板实参不区分重载**：两个只差默认模板实参的模板是重定义；SFINAE 重载要改用非类型模板形参。
- **示例中的函数调用要写在函数体内**，不能直接放在全局作用域。
- **`&&` 只有模板推导下才是万能引用**：`const T&&`、`std::vector<T>&&` 都不是。
- **值传递会丢引用与顶层 `const`**，数组还会退化为指针——需要这些信息时改用引用/数组引用。

---

> [!tip] 延伸
> 相关学习记录见 [[C++深入/学习进度.md]]；上一篇见 [[右值引用,移动语义,完美转发|右值引用、移动语义与完美转发]]，其中"万能引用 + 引用折叠"正是本篇第八节的基础；本篇第九节的完整实现见 [fanyu99/ThreadPool](https://github.com/fanyu99/ThreadPool)。
