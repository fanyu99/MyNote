---
tags:
  - CPP/constexpr
  - CPP/编译期
  - CPP/核心
date: 2026-10-12
---

# constexpr 编译期常量

承接 [[类型推导和编译期计算|类型推导与编译期计算]]，本篇回答"如何把计算从运行期搬到编译期"：先厘清 `constexpr` 与 `const` 的区别，再看 `constexpr` 函数、字面类型与 `constexpr` 构造函数，最后延伸到 C++20 的 `consteval` 与 `constinit`，并用编译期生成 CRC-32 查找表作为综合示例。

> [!important] 核心主线
> **`constexpr` 表达"可在编译期求值"这一更强语义：只要上下文允许，编译器就会在编译期完成计算，把结果直接烧进二进制。**
> 它不等于"一定在编译期执行"——放在运行期上下文里，`constexpr` 函数同样是普通函数；而 `consteval` 则强制编译期求值。

---

## 一、`constexpr` 与 `const` 的区别

`const` 只表示变量**初始化后不可修改**，初始值可以来自运行时计算：

```cpp
int runtime_input = read_from_user();
const int bound = runtime_input; // 合法：运行期确定初值，之后不可改
```

`constexpr` 语义更强，要求初值在**编译期**就能确定：

```cpp
constexpr int bound = 1024;         // 编译期常量
// constexpr int bad = runtime_input; // 编译错误：初值不是常量表达式
```

> [!important] 记忆
> `constexpr` 蕴含 `const`（本身不可改），但 `const` 不蕴含 `constexpr`。
> 一个变量是 `constexpr`，说明它不仅是只读的，而且初值在编译期已定。

---

## 二、`constexpr` 函数

`constexpr` 函数的执行时机**取决于调用上下文**，而非函数定义本身：

```cpp
constexpr int square(int x) {
    return x * x;
}

// C++17 起也支持 constexpr lambda
constexpr auto add = [](int a, int b) constexpr { return a + b; };

// 编译期求值：参数与上下文都是常量表达式
constexpr int kResult = square(8);  // 编译器可直接把 kResult 折叠为 64

// 运行期求值：参数来自运行时
int runtime_input = 42;
int result = square(runtime_input); // 退化为普通函数调用，运行时执行
```

> [!note] 说明
> 同一个 `constexpr` 函数，既可参与编译期常量表达式，也可在运行时当普通函数用。
> 只有当**所有**实参都是常量表达式、且出现在要求常量表达式的上下文（如 `constexpr` 初始化、模板实参、`static_assert`）时，才会在编译期求值。

### 2.1 C++11 → C++17 的逐步放开

| 能力 | C++11 | C++14 | C++17 |
|---|---|---|---|
| 局部变量 | 仅一条 `return` | 允许 | 允许 |
| 循环（`for` / `while`） | 禁止 | 允许 | 允许 |
| `if`-`else` | 禁止（只能用三元运算符） | 允许 | 允许 |
| 成员函数修改成员 | 禁止（隐式 `const`） | 允许 | 允许 |
| `constexpr` lambda | 不支持 | 有限 | 正式支持 |
| 标准库 `constexpr` | 极少 | 增多 | 大量增加 |

> [!warning] 注意
> C++11 的 `constexpr` 函数体只能是"一个 `return` 语句"（外加若干 `static_assert`、类型别名等），几乎只能写递归；循环与分支能力是 C++14 才加入的。

---

## 三、`static_assert` 与 `if constexpr`

`static_assert` 是**编译期断言**，第一个参数必须是常量表达式，天然与 `constexpr` 函数配合，把检查提前到编译期：

```cpp
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

static_assert(factorial(0) == 1, "factorial(0) should be 1");
static_assert(factorial(5) == 120, "factorial(5) should be 120");
static_assert(factorial(10) == 3628800, "factorial(10) should be 3628800");
```

`if constexpr`（C++17）同样以常量表达式为条件，用于**编译期分支**：

```cpp
constexpr auto add = [](int a, int b) constexpr { return a + b; };

int main() {
    if constexpr (add(1, 2) == 3) {      // 条件必须是常量表达式
        std::cout << "1+2=" << add(1, 2) << '\n';
    }
}
```

> [!important] `if constexpr` 与普通 `if` 的区别
> `if constexpr` 在**编译期**决定分支，未命中的分支**不会实例化**（在模板里尤其关键）；普通 `if` 两个分支都会被编译。

---

## 四、字面类型与 `constexpr` 构造函数

要让一个**类**能作为 `constexpr` 变量，该类必须是**字面类型（literal type）**：

1. 标量类型（含指针、引用），或元素为字面类型的数组；
2. 类类型：所有非静态数据成员与基类都是字面类型，且至少有一个 `constexpr` 构造函数；析构函数需为**平凡析构**（C++20 起放宽为 `constexpr` 析构函数）。

```cpp
// 字面类型
struct Point {
    float x;
    float y;

    constexpr Point(float x_, float y_) : x(x_), y(y_) {}
    // 隐式析构函数平凡，满足条件
};

// 不是字面类型：std::vector 的析构函数非平凡
struct NotLiteral {
    std::vector<int> vec;
};

constexpr Point kOrigin{0.0f, 0.0f};
static_assert(kOrigin.x == 0.0f);
static_assert(kOrigin.y == 0.0f);
```

构造函数体里也可以写逻辑——只要落在 `constexpr` 允许的范围内（C++14 起允许循环、分支）：

```cpp
struct BcdDecimal {
    unsigned char bcd{};

    constexpr explicit BcdDecimal(int decimal) {
        int remainder = decimal;
        int shift = 0;
        while (remainder > 0) {
            bcd |= static_cast<unsigned char>((remainder % 10) << shift);
            remainder /= 10;
            shift += 4;
        }
    }

    constexpr int to_decimal() const {          // constexpr 成员函数（C++14）
        int result = 0;
        int multiplier = 1;
        unsigned char temp = bcd;
        while (temp > 0) {
            result += (temp & 0x0F) * multiplier;
            temp >>= 4;
            multiplier *= 10;
        }
        return result;
    }
};

constexpr BcdDecimal kDec42{42};
static_assert(kDec42.bcd == 0x42, "BCD of 42 should be 0x42");
static_assert(kDec42.to_decimal() == 42, "Round-trip conversion should work");
```

> [!warning] `constexpr` 析构函数（C++20）
> C++20 起析构函数可以是 `constexpr`，并允许在常量求值中使用 `new`，但**分配的内存必须在常量求值结束前释放**：

```cpp
// C++20
struct Resource {
    int* data;
    std::size_t size;

    constexpr Resource(std::size_t n) : data{}, size(n) {}
    constexpr ~Resource() { /* 清理逻辑 */ }
};
```

---

## 五、`consteval`：强制编译期求值（C++20）

`consteval` 声明的函数是**立即函数（immediate function）**，每次调用都必须产生常量，无法退化为运行时调用：

```cpp
constexpr int square(int x) { return x * x; }
consteval int c_square(int x) { return x * x; }   // C++20

int runtime = 5;
int a = square(runtime);      // ✓ 合法：退化为普通运行时函数
// int b = c_square(runtime); // ✗ 编译错误：consteval 只接受编译期参数
int c = c_square(5);          // ✓
```

`consteval` 常用来做**参数校验**，把非法输入挡在编译期：

```cpp
consteval int checked_port(int p) {
    if (p < 1 || p > 65535) {
        throw "端口不合法";        // 在常量求值中触发即编译错误
    }
    return p;
}

constexpr int kPort = checked_port(8080); // ✓
// constexpr int kBad = checked_port(0);  // ✗ 编译失败
```

> [!note] 对比记忆
> - `constexpr`：**可以**在编译期求值（上下文决定）；
> - `consteval`：**必须**在编译期求值；
> - `constinit`：只要求**静态初始化**，不要求值可被编译期求值。

---

## 六、`constinit`：保证静态初始化

不同编译单元里全局对象的动态初始化顺序不确定（静态初始化顺序问题），可能读到未初始化的 0：

```cpp
// a.cpp
extern int g_base;            // 在别的编译单元定义
// b.cpp
int g_derived = g_base * 2;   // 若 g_base 尚未初始化，这里可能读到 0
```

`constinit`（C++20）要求变量**进行静态初始化**（用常量表达式初始化），从而避免顺序问题：

```cpp
constinit int g_value = 100;  // 保证在程序启动前完成静态初始化
```

> [!warning] 只约束初始化时机
> `constinit` 只保证"初始化在编译期完成"，**不约束可变性**：变量之后仍然可以被修改（它不需要是 `const`）。

---

## 七、综合示例：编译期生成 CRC-32 查找表

把 256 项的查表在编译期算好，运行时只剩"查表"：

```cpp
#include <array>
#include <cstdint>

constexpr std::array<std::uint32_t, 256> make_crc32_table() {
    std::array<std::uint32_t, 256> table{};
    constexpr std::uint32_t kPolynomial = 0xEDB88320u;

    for (std::size_t i = 0; i < 256; ++i) {
        std::uint32_t crc = static_cast<std::uint32_t>(i);
        for (int j = 0; j < 8; ++j) {
            crc = (crc & 1) ? ((crc >> 1) ^ kPolynomial) : (crc >> 1);
        }
        table[i] = crc;
    }
    return table;
}

// 编译期生成完整的 CRC-32 查找表
constexpr auto kCrc32Table = make_crc32_table();

constexpr std::uint32_t crc32_compute(const std::uint8_t* data, std::size_t len) {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < len; ++i) {
        crc = (crc >> 8) ^ kCrc32Table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFFu;
}
```

> [!tip] 价值
> 查表数据在编译期确定，运行时零初始化开销；同时表本身是 `constexpr`，天然放进只读段。

---

## 八、核心总结

### 8.1 一句话主线

```text
constexpr  → 可在编译期求值；放在运行期上下文即普通函数
const      → 只读，初值可运行期确定；constexpr 蕴含 const
字面类型    → constexpr 变量的前提（标量/引用，或满足条件的类）
consteval  → 强制编译期求值，无法退化为运行时
constinit  → 只保证静态初始化，避免初始化顺序问题
static_assert / if constexpr → 编译期断言与编译期分支
```

### 8.2 速查表

| 问题 | 答案 |
| --- | --- |
| `const` 与 `constexpr` 的区别？ | `const` 只读、初值可运行期定；`constexpr` 要求编译期常量 |
| `constexpr` 函数一定在编译期执行吗？ | 不一定，取决于调用上下文 |
| `constexpr` 变量需要什么类型？ | 字面类型（literal type） |
| `constexpr` 函数体 C++11 有什么限制？ | 只能一条 `return`（外加 `static_assert` 等） |
| `constexpr` 构造函数里能写循环吗？ | C++14 起可以 |
| `consteval` 与 `constexpr` 的关键区别？ | `consteval` 强制编译期，不能退化为运行时 |
| `constinit` 解决什么问题？ | 静态初始化顺序问题 |
| `if constexpr` 与普通 `if` 的区别？ | `if constexpr` 编译期分支，未命中分支不实例化 |

### 8.3 易错点

- 把 `constexpr` 当成"一定在编译期执行"——它只是**允许**编译期求值。
- `constexpr` 变量必须是字面类型；含 `std::vector`、`std::string` 等非平凡析构成员的类不能用作 `constexpr` 变量。
- C++11 的 `constexpr` 函数体受限（单 `return`），循环 / 分支是 C++14 才有的。
- `consteval` 函数传入运行期值直接编译失败，不能"退化"。
- `constinit` 只约束初始化，不约束变量是否可改。

---

> [!tip] 延伸
> 相关学习记录见 [[C++深入/学习进度.md]]，类型推导见 [[类型推导和编译期计算|类型推导与编译期计算]]，编译期实例化与模板的关系见 [[C++深入/C++模板/C++ 函数模板与类型推导|函数模板与类型推导]]。
