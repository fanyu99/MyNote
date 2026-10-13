---
tags:
  - CPP/新特性
  - CPP/optional
  - CPP/variant
  - CPP/string_view
  - CPP/span
  - CPP/核心
date: 2026-10-12
---

# optional、string_view、variant、span

承接 [[RAII,资源所有权与unique_ptr|RAII、资源所有权与 unique_ptr]] 里"把语义编码进类型"的思路，本篇整理现代 C++ 中四个常用的"语义包装"类型：用 `optional` 表达"有 / 没有"，用 `variant` 表达"是其中之一"，用 `string_view` / `span` 表达"对已有数据的只读视图"。

> [!important] 核心主线
> **它们都把"不确定性"或"借用"写进类型，用来替代裸 `bool` 标志、`union`，以及 `(指针 + 长度)` 这一对手工参数。**
> 一句话选型：需要**拥有**数据用 `optional` / `variant`；只是**视图**已有数据用 `string_view` / `span`。

---

## 一、`std::optional<T>`：表达"有值 / 无值"

用于表示"某个值可能存在、也可能不存在"，替代返回魔法值或用裸 `bool` 标志：

```cpp
#include <optional>

std::optional<int> divide(int a, int b) {
    if (b == 0) return std::nullopt;   // 明确表示"没有值"
    return a / b;
}

auto r = divide(10, 2);

if (r.has_value()) { /* ... */ }
if (r) { int v = *r; }            // 隐式转 bool

int a = r.value();                // 无值时抛 std::bad_optional_access
int b = r.value_or(0);            // 无值时返回 0 —— 最安全、最常用
int c = *r;                       // 无值时是 UB（不做检查）
```

### 1.1 原地构造

`emplace` 直接在 `optional` 内部构造对象，避免临时对象与拷贝：

```cpp
std::optional<Config> cfg;
cfg.emplace(8080, "cam0");        // 直接在内部构造 Config(8080, "cam0")
```

### 1.2 使用注意

> [!warning] 三点注意
> 1. 空的 `optional` **不会构造**内部对象（但会预留它的存储）；
> 2. `sizeof(std::optional<T>)` 通常**大于** `sizeof(T)`（多一个标志位 + 对齐填充）；
> 3. 代价是它**始终预留 `T` 那么大的空间**，即便为空。对超大对象用 `optional<T>` 会撑大栈帧——此时改用 `std::optional<std::unique_ptr<T>>`。

> [!tip] 何时使用
> 需要"**未设置 / 空**"这类语义时用它；如果只是需要一个二值标志，直接用 `bool` 即可，不必引入 `optional`。

---

## 二、`std::variant<...>`：类型安全的联合体

`variant` 是类型安全的 `union`，同一时刻只保存**其中一个**类型的值：

```cpp
#include <variant>

std::variant<int, double, std::string> v;  // 默认构造第一个类型（int = 0）
v = 42;                                     // 存 int
v = 3.14;                                   // 存 double（旧的 int 被正确析构）
v = std::string("hello");                   // 存 string
```

### 2.1 取值

```cpp
std::variant<int, std::string> v = 42;

int i = std::get<int>(v);                          // ✓
// std::string s = std::get<std::string>(v);        // ✗ 抛 std::bad_variant_access
int j = std::get<0>(v);                            // 也可按索引取

if (auto p = std::get_if<int>(&v)) {               // 检查式：成功返回指针，失败返回 nullptr
    std::cout << *p;
}

if (std::holds_alternative<int>(v)) { /* ... */ }  // 手动判断
```

### 2.2 `std::visit`：对"当前那个类型"统一处理

`visit` 会把当前存储的值交给传入的可调用对象（lambda / 函数 / 仿函数）：

```cpp
struct ConfigValue {
    std::variant<int, double, bool, std::string> data;
};

std::string to_string(const ConfigValue& cv) {
    return std::visit([](const auto& v) -> std::string {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>)
            return v ? "true" : "false";
        else if constexpr (std::is_arithmetic_v<std::decay_t<decltype(v)>>)
            return std::to_string(v);
        else
            return std::string(v);
    }, cv.data);
}
```

不清楚当前类型时，常用 `overloaded` 模板给每个类型写一个分支：

```cpp
template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

double area(const Shape& s) {
    return std::visit(overloaded{
        [](const Circle& c)    { return c.r * c.r * M_PI; },
        [](const Rectangle& r) { return r.w * r.h; },
        [](const Triangle& t)  { return 0.5 * t.b * t.h; },
    }, s);
}
```

### 2.3 赋值二义性与局限

> [!warning] 赋值二义性
> 若多个备选类型都能匹配同一个初值，会因二义性报错：
> ```cpp
> std::variant<int, long, double> v;
> v = 39;    // ✓ int
> v = 3.14;  // ✓ double
> v = {1};   // ✗ 报错：int 还是 long？
> ```

> [!important] variant 相对虚函数的劣势
> - `variant` 是**封闭集合**：所有类型必须一开始就列全，新增一种类型要改动所有用到它的地方；而虚函数是**开放集合**，新增派生类不必改基类代码。
> - 用泛型 lambda + `if constexpr` 时，**未被覆盖的类型会静默落入最后的 `else` 分支**（可能什么都没做），不像虚函数那样由编译器强制覆盖每个派生类。

---

## 三、`std::string_view`：只读字符串视图

`string_view` 是一个"指针 + 长度"的**非拥有**视图（`const char* data` + `size_t size`），不拷贝、不拥有数据：

```cpp
#include <string_view>
using namespace std::literals;

void log(std::string_view msg);   // 零拷贝
log("hello");                     // 不构造 std::string，直接指过去
log(std::string("hello"));        // 也能接 std::string（隐式转换）
log("hello"sv);                   // 字面量后缀
```

### 3.1 与其它字符串类型的对比

| 类型 | 大小 | 是否拥有 | 是否保证以 `\0` 结尾 |
| --- | --- | --- | --- |
| `const char*` | 8 | 否 | 是（C 字符串约定） |
| `std::string` | 32（典型） | **是** | 是 |
| `std::string_view` | 16 | 否 | **否** |
| `const std::string&` | 8 | 否（传引用，不拷贝） | 是 |

> [!warning] 三点注意
> 1. 不保证以 `\0` 结尾，**不能直接传给 C 接口**；
> 2. **不延长**所引用数据的生命周期，注意悬空；
> 3. 尽量避免作为成员变量或返回值持有（除非能保证底层数据存活）——它只是"借用"，不做生命周期管理。

### 3.2 典型用法（零拷贝切片）

```cpp
// 去掉前缀（不拷贝）
std::string_view strip_prefix(std::string_view sv, std::string_view prefix) {
    if (sv.substr(0, prefix.size()) == prefix)
        return sv.substr(prefix.size());
    return sv;
}

// 按分隔符切分，返回 view 的 vector，零拷贝
std::vector<std::string_view> split(std::string_view sv, char delim) {
    std::vector<std::string_view> out;
    while (true) {
        auto pos = sv.find(delim);
        if (pos == std::string_view::npos) { out.push_back(sv); break; }
        out.push_back(sv.substr(0, pos));
        sv.remove_prefix(pos + 1);
    }
    return out;
}
```

> [!warning] 返回值的生命周期
> 上面的 `split` 只有在**被切分的源字符串**仍然存活时，返回的这些 view 才有效。若源是临时 `std::string`，view 会立刻悬空。

---

## 四、`std::span<T>`：连续内存视图

处理"一块连续内存"时，用 `span` 比 `vector` / `(指针, 长度)` 更通用、更安全：

```cpp
void process(const std::vector<int>& v);   // ✗ 只能接 vector，数组、C 数组都不行
void process(const int* p, size_t n);      // ✗ 两个参数，容易传错 n，类型不安全
void process(std::span<const int> data);   // ✓ 任何连续内存都能接，长度自带
```

各种来源都能直接喂进去：

```cpp
#include <span>

std::vector<int> v{1, 2, 3};
std::array<int, 3> a{4, 5, 6};
int c_array[3]{7, 8, 9};

process(v);                    // ✓
process(a);                    // ✓
process(c_array);              // ✓ 连 C 数组都能接
process({v.data() + 1, 2});    // ✓ 还能取子区间
```

### 4.1 常见操作

```cpp
std::span<int> s(vec);

s.size();  s.empty();          // 大小
s[0];      s.front();          // 元素
s.subspan(1, 3);               // 子区间视图（不拷贝）
s.first(2); s.last(2);         // 前 / 后 n 个
s.data();                      // 底层指针
```

### 4.2 常见应用：图像 / 缓冲区

不关心数据来自 `cv::Mat`、`vector<uint8_t>` 还是相机裸缓冲区：

```cpp
void process_roi(std::span<const uint8_t> pixels, int width, int height) {
    for (size_t i = 0; i < pixels.size(); ++i) {
        // ...
    }
}

cv::Mat img = cv::imread("a.png", cv::IMREAD_GRAYSCALE);
process_roi({img.data, static_cast<size_t>(img.total())}, img.cols, img.rows);

std::vector<uint8_t> buf = read_from_camera();
process_roi(buf, w, h);

uint8_t raw[1024];
process_roi(raw, w, h);
```

> [!warning] 与 `string_view` 相同的约束
> 1. 只作为**函数参数**使用，不要作为返回值或长期成员（同 `string_view`，它是借用）；
> 2. **不做边界检查**：`s[i]` 越界访问是 UB，`span` 不会抛异常。

---

## 五、四者选型决策

```text
        ┌─────────────── 拥有数据 ───────────────┐
        │                                        │
   optional<T>                          variant<T1,T2,...>
   "有 或 没有"                          "是这些类型之一"
        │                                        │
        └────────── 表达"值的不确定性" ───────────┘

        ┌─────────────── 不拥有数据 ──────────────┐
        │                                        │
   string_view                            span<T>
   "字符串的视图"                          "连续内存的视图"
        │                                        │
        └────────── 表达"对已有数据的视图" ────────┘
```

> [!tip] 一句话
> **是否拥有数据**决定用上面一组还是下面一组；**是"有 / 没有"还是"其中之一"**决定用 `optional` 还是 `variant`；**是字符串还是任意连续内存**决定用 `string_view` 还是 `span`。

---

## 六、核心总结

### 6.1 一句话主线

```text
optional      → 有 / 无值；value_or 最安全；始终占 T 大小
variant       → 类型安全 union；get / get_if / visit；封闭集合
string_view   → 字符串只读视图；零拷贝；不保证 \0、不延长生命周期
span          → 连续内存只读视图；接 vector/array/C 数组；不做边界检查
选型          → 拥有用 optional/variant，视图用 string_view/span
```

### 6.2 速查表

| 问题 | 答案 |
| --- | --- |
| `optional` 无值时 `value()` 怎样？ | 抛 `std::bad_optional_access` |
| `optional` 最安全的取值方式？ | `value_or(默认值)` |
| 空 `optional` 会构造对象吗？ | 不会，但会预留 `T` 的存储 |
| `variant` 怎么安全取值？ | `get_if` / `holds_alternative` 判断，或 `visit` |
| `visit` 未覆盖某类型会怎样？ | 泛型 lambda + `if constexpr` 时会静默走 `else` |
| `variant` 与虚函数的本质区别？ | variant 是封闭集合，虚函数是开放集合 |
| `string_view` 能做 C 接口参数吗？ | 不行，不保证以 `\0` 结尾 |
| `span` 越界访问会抛异常吗？ | 不会，是 UB；`span` 不做边界检查 |
| `string_view` / `span` 能作成员吗？ | 尽量避免，除非能保证底层数据存活 |

### 6.3 易错点

- 用 `*r` / `value()` 前不判空——空 `optional` 会 UB 或抛异常。
- 对超大对象用 `optional<T>` 撑大栈帧——应改用 `optional<unique_ptr<T>>`。
- `variant` 赋值时多个类型都能匹配 → 二义性编译错误。
- 用 `visit` + 泛型 lambda 时漏了某个类型，却以为编译器会报错（实际会静默走 `else`）。
- 把 `string_view` / `span` 当拥有型对象存成成员或返回——底层数据一失效就悬空。
- 以为 `span` 会做边界检查——不会。

---

> [!tip] 延伸
> 相关学习记录见 [[C++深入/学习进度.md]]，"把语义编码进类型"与智能指针见 [[RAII,资源所有权与unique_ptr|RAII、资源所有权与 unique_ptr]]，`if constexpr` 编译期分支见 [[constexpr编译常量|constexpr 编译期常量]] 与 [[C++深入/C++模板/C++ 函数模板与类型推导|函数模板与类型推导]]。
