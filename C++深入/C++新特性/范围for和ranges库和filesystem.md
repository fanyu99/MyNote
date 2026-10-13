---
tags:
  - CPP/范围for
  - CPP/ranges
  - CPP/filesystem
  - CPP/核心
date: 2026-10-13
---

# 范围 for、ranges 库与 filesystem

承接 [[optional、string_view、variant、span|optional、string_view、variant、span]] 中"类型可以不拥有数据"的思路，本篇把 A7 的三块实用特性串起来：**范围 for** 是迭代器循环的语法糖（讲清它展开成什么，以及代理对象带来的坑）；**ranges** 用管道把算法与视图组合成惰性数据流，`view` 与 `string_view`/`span` 一样**不拥有数据**；**filesystem** 用 `path` 统一跨平台路径操作。

> [!important] 核心主线
> - **范围 for**：`for (auto& x : c)` 本质是 `auto&& __range = c;` + `begin/end` 迭代器循环；默认用 `const auto&`，遇到**代理对象**（如 `vector<bool>`）用 `auto&&`。
> - **ranges**：`c | views::filter(...) | views::transform(...)` 惰性组合，**不产生中间容器**；view 只"看"数据，随时可能悬空。
> - **filesystem**：`path` 用 `/` 拼接即自动跨平台；每个 API 都有**抛异常**与**接收 `error_code`** 两版。

---

## 一、范围 for 的本质：语法糖

```cpp
for (auto& x : c) { body }
```

等价于（`auto& x` 是你自己写的 for-range-declaration）：

```cpp
{
    auto&& __range = c;              // ← 关键：绑定为 auto&&
    auto __begin = begin(__range);   // 有成员 begin 则用，否则 ADL 找 begin(range)
    auto __end   = end(__range);
    for (; __begin != __end; ++__begin) {
        auto& x = *__begin;          // 这一行取决于你写的 for-range-declaration
        body
    }
}
```

> [!note] `auto&& __range` 的两点作用
> 1. 它能绑定左值和右值，因此**遍历临时容器也安全**（把临时对象的生命期延长到整个循环）；
> 2. 起止迭代器类型可以不同（**哨兵 sentinel**），这是 C++17 才放宽的。

---

## 二、四种写法怎么选

```cpp
std::vector<Heavy> v;            // Heavy 内含 std::string

for (auto h : v)          // ✗ 每个元素拷一份副本（含 string 的堆分配），只读却付拷贝代价
for (auto& h : v)         // ✓ 可修改元素
for (const auto& h : v)   // ✓✓ 只读首选，零拷贝
for (auto&& h : v)        // ✓ 万能引用，最稳（代理对象场景）
```

| 写法 | 拷贝元素？ | 能改元素？ | 适用 |
| --- | --- | --- | --- |
| `auto h` | 是 | 改的是副本 | 只读且元素小（如 `int`） |
| `auto& h` | 否 | 能 | 需要修改元素 |
| `const auto& h` | 否 | 不能 | **只读默认首选** |
| `auto&& h` | 否 | 能 | 元素是**代理对象**时唯一稳妥写法 |

---

## 三、需要注意的地方

### 3.1 遍历中修改容器是未定义行为

在循环体内**增删元素**（`push_back` / `erase` 等）会使迭代器失效 → UB。修改元素**的值**本身没问题。需要边遍历边增删时，改用索引循环或显式迭代器并妥善处理失效规则。

### 3.2 对循环变量按引用捕获 → 悬空

```cpp
for (const auto& item : items) {
    fs.push_back([&item]{ use(item); });   // ✗
}
```

> [!warning] 为什么错
> range-for 每次迭代都会**新建**一个局部变量 `item`，本轮迭代结束即销毁。按引用捕获后，闭包存的是对**已销毁变量**的引用，任务真正执行时即悬空。
> 修复：按值捕获 `[item]`，或先取出需要的值再捕获。
> 对比：传统 `for (int i = 0; ...; ++i)` 里 `i` **是同一个变量**，`[&i]` 捕获后所有闭包共享它，遍历结束时 `i` 是终止值——这是另一种坑。

### 3.3 `vector<bool>` 的代理陷阱

`std::vector<bool>` 为了位压缩，`*it` 返回的不是 `bool&`，而是代理类 `std::vector<bool>::reference`。

```cpp
std::vector<bool> flags{false, false};

for (auto b : flags)  b = true;   // b 是代理 reference，仍指向那位 → flags 确实被改成 {true,true}
for (bool b : flags)  b = true;   // ✓ 这才是"副本"：b 是真 bool，改它不影响 flags
for (auto&& b : flags) b = true;  // ✓ 万能引用，稳妥
for (auto& b : flags)  b = true;  // ✗ 编译错误：*it 是 prvalue 代理，非常量左值引用不能绑定
```

> [!important] 关键点
> `for (auto b : flags)` 里的 `b` **不是 `bool`**，而是代理对象——它仍指向底层那一位，所以 `b = true` 会真的改到 `flags`。真正"只改副本、原容器不变"的是 **`for (bool b : flags)`**。
> 通用结论：容器迭代器返回**代理对象**时，用 `auto&&` 承接最安全；不要用 `auto`（拿到的是代理，语义易被误解）或 `auto&`（编译不过）。

---

## 四、ranges 库（C++20）

### 4.1 传统 STL 算法 vs ranges

```cpp
// 传统写法：要"奇数的平方"——必须传两个迭代器，还要中间容器
std::vector<int> tmp;
std::transform(v.begin(), v.end(), std::back_inserter(tmp),
               [](int x){ return x * x; });
```

ranges 用管道一次串起来，**不产生中间容器**：

```cpp
auto result = v | std::views::filter([](int x){ return x % 2 == 0; })
                | std::views::transform([](int x){ return x * x; })
                | std::views::take(3);

for (int x : result) std::cout << x << " ";   // 以 v = {1,2,3,4,5,6} 为例，输出 4 16 36
```

> [!note] 惰性求值
> `views` 组合出来的对象**不计算任何值**，只有在遍历时才逐个计算；`take(3)` 取满后即停，不会多算。

### 4.2 常见 views

```cpp
v | std::views::filter(pred)       // 过滤
v | std::views::transform(fn)      // 映射
v | std::views::take(n)            // 取前 n 个
v | std::views::drop(n)            // 跳过前 n 个
v | std::views::reverse            // 反转（要求双向范围）
v | std::views::take_while(pred)   // 取到条件第一次不成立
v | std::views::drop_while(pred)   // 跳过直到条件第一次不成立
v | std::views::split(delim)       // 分割（结果元素是子范围，不是 string）
v | std::views::join               // 把嵌套范围拍平
v | std::views::keys / values      // 取 pair 的 key / value（C++20）
v | std::views::elements<0>        // 取 tuple 第 N 个元素（C++20）

std::views::iota(1)                // 从 1 开始的无限序列
std::views::iota(1, 10)            // [1, 10)
```

> [!warning] views 不拥有数据
> `view` 只是底层数据的"视图"，与 `string_view`/`span` 同类：**底层数据一旦失效，view 立刻悬空**。因此不要把 view（或指向局部的 view）长期缓存或作为返回值。
> `std::views::split` 在 C++20 的行为不够好用（结果元素是子范围，且按字符串分隔的处理在 **C++23** 才修好），老工程里要留意。

### 4.3 ranges 算法

```cpp
std::ranges::sort(v);                                        // 直接传容器
std::ranges::sort(v, std::greater<>{});                      // 降序
std::ranges::find(v, 42);                                    // 返回迭代器，不用 v.begin()
std::ranges::count_if(v, [](int x){ return x > 5; });
std::ranges::any_of(v, pred);
std::ranges::all_of(v, pred);

// 投影（projection）：省掉比较用 lambda
struct User { std::string name; int age; };
std::ranges::sort(users, std::less{}, &User::age);           // 按 age 排序
std::ranges::find(users, "Bob", &User::name) != users.end(); // 按 name 查找（C++20）
```

> [!note] 与旧算法的区别
> ranges 算法直接吃**整个范围**，并支持**投影**（用 `&T::member` 指定比较/查找字段，省掉 `[](auto& a, auto& b){...}`）；返回类型仍和旧算法一致（如 `find` 返回迭代器）。

---

## 五、`std::filesystem` 文件管理

```cpp
#include <filesystem>
namespace fs = std::filesystem;
```

### 5.1 `path` 路径操作

```cpp
fs::path p = "models/v1/yolo.onnx";
// p.filename()    → "yolo.onnx"
// p.stem()        → "yolo"          ← 不含扩展名
// p.extension()   → ".onnx"
// p.parent_path() → "models/v1"
```

`path` 自动处理 `/` 与 `\`，适合跨平台项目。

> [!note] 拼接与输出
> - 拼接用 `/` 运算符：`base / "a" / "b.txt"`；
> - 取字符串用 `p.string()`；Windows 下 `path` 内部是宽字符，含非 ASCII 路径时 `.string()` 可能抛异常，可考虑 `.u8string()`（C++20 起返回 `std::u8string`）。

### 5.2 目录创建与删除

```cpp
fs::create_directory(dir);      // 创建一级（父目录必须已存在）
fs::create_directories(dir);    // 递归创建（推荐）
fs::remove(path);               // 删文件或空目录
fs::remove_all(path);           // 递归删除（危险！返回删除项数）
fs::rename(from, to);           // 重命名 / 移动
fs::copy(from, to);             // 递归复制
fs::copy_file(from, to);        // 复制单个文件
fs::exists(p);
fs::is_directory(p);
fs::is_regular_file(p);
fs::is_empty(p);
```

### 5.3 遍历目录

```cpp
// 非递归
for (const auto& entry : fs::directory_iterator(dir)) {
    std::cout << entry.path() << "\n";
}
// 递归
for (const auto& entry : fs::recursive_directory_iterator(base)) {
    if (entry.is_regular_file())
        std::cout << fs::relative(entry.path(), base).string() << "\n";
}
```

> [!warning] 遍历无顺序
> 目录迭代顺序**不保证**，需要顺序就自己收集后 `sort`。

### 5.4 错误处理：两套 API

```cpp
// 版本 1：抛异常（filesystem_error）
fs::file_size(path);                    // 文件不存在 → 抛 fs::filesystem_error

// 版本 2：传 error_code，不抛
std::error_code ec;
auto size = fs::file_size(path, ec);
if (ec) { /* 处理错误，ec.message() 拿到描述 */ }
```

> [!note] 什么时候用哪版
> 期望存在的路径用抛异常版更省事；在"可能不存在/权限不足是常态"的探测型代码（例如配置文件查找、批量扫描）里，用 `error_code` 版可以把错误当返回值处理，避免异常开销与意外终止。

---

## 六、核心总结

### 6.1 一句话主线

```text
范围 for     → 语法糖：auto&& __range + begin/end 迭代器循环
             → 默认 const auto&；代理对象用 auto&&；遍历中增删容器是 UB
             → 对循环变量按引用捕获会悬空（每轮 item 都是新局部变量）
ranges views → 管道组合，惰性求值，零中间容器；取满即停
             → view 不拥有数据（同 string_view/span）；filter 后 size 不是 O(1)
ranges 算法  → 直接传容器，支持投影（&T::member）省掉比较 lambda
filesystem   → path 用 / 拼接，自动跨平台；每个 API 有抛异常/error_code 两版
             → remove_all 是头号危险 API；遍历顺序不保证，要排序就自己排
```

### 6.2 速查表

| 问题 | 答案 |
| --- | --- |
| 范围 for 的默认写法？ | `const auto&`（只读）；要改元素用 `auto&`；代理对象用 `auto&&` |
| 遍历中 `push_back` 会怎样？ | 迭代器失效，未定义行为 |
| 范围 for 遍历临时容器安全吗？ | 安全，`auto&& __range` 会延长临时对象生命期 |
| `vector<bool>` 里 `for (auto b : flags) b = true;` 会改到容器吗？ | 会——`auto` 拿到的是代理 `reference`，仍指向底层位 |
| 那怎么写才是"只改副本"？ | `for (bool b : flags)`，`b` 是真 `bool` |
| `views` 组合会立即计算吗？ | 不会，惰性；只有遍历时才逐个计算 |
| `filter` 之后 `size()` 是 O(1) 吗？ | 不是，filter 不保留大小信息 |
| view 能作返回值吗？ | 除非底层数据活得够久，否则同 `string_view` 一样会悬空 |
| `path` 怎么拼接？ | 用 `/` 运算符，自动处理跨平台分隔符 |
| 遍历目录顺序有保证吗？ | 没有，需要顺序就自己 `sort` |
| 文件不存在时 `file_size` 怎样？ | 抛 `filesystem_error`，或传 `error_code` 拿错误 |
| 哪个 API 最危险？ | `remove_all`，递归删除且不可恢复 |

### 6.3 易错点

- 把 `for (auto b : flags)`（`vector<bool>`）当成"改副本"——它实际会改到容器；要只改副本得写 `for (bool b : flags)`。
- 在范围 for 里对循环变量**按引用捕获**进闭包 → 每轮变量随迭代销毁，闭包悬空。
- 在遍历中增删容器元素 → 迭代器失效 UB（改元素值没事）。
- 把 `view` 当拥有型对象缓存/返回 → 底层数据一失效就悬空。
- 以为 `filter` 之后 `size()` 是 O(1) → 是 O(n)。
- 用 `.string()` 打印含非 ASCII 的 Windows 路径 → 可能抛异常，改用 `.u8string()`。
- 误用 `remove_all` 或忽略"遍历顺序不保证"。

---

> [!tip] 延伸
> 相关学习记录见 [[C++深入/学习进度.md]]；"不拥有数据"的视图语义见 [[optional、string_view、variant、span|optional、string_view、variant、span]]；`submit` 中用到范围 for 与 `invoke` 的场景见 [[lambda和可变参数模板|lambda 与可变参数模板]]；迭代器与模板容器见 [[C++深入/C++模板/C++ 函数模板与类型推导|函数模板与类型推导]]。
