# 1.`std::optional<T>`
用来判断某个变量是否存在
```C++
#include <optional>

std::optional<int> divide(int a, int b) {
    if (b == 0) return std::nullopt;   // 明确表示"没有值"
    return a / b;
}

auto r = divide(10, 2);

// 检查
if (r.has_value()) { ... }
if (r) { int v = *r; }    // 隐式转 bool

// 取值（关键区别）
int a = r.value();          // 无值时抛 std::bad_optional_access
int b = r.value_or(0);      // 无值时返回 0 ★ 最安全、最常用
int c = *r;                 // 无值时 UB（不做检查）

// 原地构造（避免临时对象）
std::optional<Config> cfg;
cfg.emplace(8080, "cam0");  // 直接在 optional 内部构造，无拷贝

```
>[!注意]
>1.空的`optional`不会构造对象
>2.`sizeof(optional<T>)通常比sizeof(T)大`
>3.代价是总是构造T那么大的空间，即便为空。对超大对象使用optional会使栈帧变大-----使用`optional<unique_ptr<T>>`
什么时候使用？
在需要使用”未设置/空“的语义时使用，
如果仅要二义性，使用bool即可


# 2.`std::variant<...>`替代`union`

```C++
#include <variant>

std::variant<int, double, std::string> v;   // 默认构造第一个类型（int = 0）
v = 42;                                      // 存 int
v = 3.14;                                    // 存 double（旧的 int 被正确析构）
v = std::string("hello");                    // 存 string
// 使用std::get<T>获取或者std::get<index>
std::variant<int, std::string> v = 42;

int i = std::get<int>(v);        // ✓
// std::string s = std::get<std::string>(v);  // ✗ 抛 std::bad_variant_access

if (auto p = std::get_if<int>(&v)) {   // 检查式：成功返回指针，失败返回 nullptr
    std::cout << *p;
}
// 不知道当前是什么类型使用std::visit+lambda
std::visit([](const auto&val){ std::cout<<val<<"\n";},v);

// 手动判断
if(std::holds_alternative<int>(v)){}
```
## 重点： `std::visit`
`visit`会对variant当前的那个类型调用你顶传入的可调用对象(lambda/函数/仿函数等)
例如：
```C++
struct ConfigValue {
    std::variant<int, double, bool, std::string> data;
};

// 把配置值转成字符串，用于打印 —— 一个 lambda 处理所有类型
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

>[!重要的规则]
>构造/赋值时，如果多个类型都能够匹配，二义性报错
>```C++
>std::variant<int,long,double>v;
>v=39; // 可以
>v=3.14; // 可以
>v={1}; // 报错，int/long?
>```
>重点缺陷：
>使用std::variant传参时，任何没有覆盖到的类型都会静默返回
>这是variant相对于虚函数最大的劣势

常见配合`overload`模板使用:
```C++
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

# 3.`std::string_view`：只读的字符串视图
`std::string_view`是双指针，`const char* data`+`size_t size`，不拷贝也不用有数据
```C++
#include <string_view>
using namespace std::literals;

void log(std::string_view msg);   // 零拷贝
log("hello");                     // 不构造 std::string，直接指过去
log(std::string("hello"));        // 也能接 std::string（隐式转换）
log("hello"sv);                   // 字面量后缀

```

## 对比表

| 类型                   | 大小     | 是否拥有    | 是否保证 `\0` 结尾 |
| -------------------- | ------ | ------- | ------------ |
| `const char*`        | 8      | 否       | 是（约定）        |
| `std::string`        | 32（典型） | **是**   | 是            |
| `std::string_view`   | 16     | 否       | **否**        |
| `const std::string&` | 8      | 传引用，不拷贝 | 是            |
>[!warning]
>1.不保证以`\0`结尾，不能传给C接口
>2.不能延长生命周期，注意悬空指针
>3.不能存为成员变量

安全写法示例：
```C++
// 去掉前缀（不拷贝）
std::string_view strip_prefix(std::string_view sv, std::string_view prefix) {
    if (sv.substr(0, prefix.size()) == prefix)
        return sv.substr(prefix.size());
    return sv;
}
// 分割（返回 view 的 vector，零拷贝）—— 经典的 tokenize
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


# 4.`std::span`连续内存的视图
可能出现以下情况
```C++
void process(const std::vector<int>& v);   // ✗ 只能是 vector，数组不行，C 数组更不行
void process(const int* p, size_t n);      // ✗ 两个参数，容易传错 n，类型不安全
void process(std::span<const int> data);   // ✓ 任何连续内存都能接，长度自带

```
使用`std::span`：
```C++
#include <span>

std::vector<int> v{1,2,3};
std::array<int, 3> a{4,5,6};
int c_array[3]{7,8,9};

process(v);        // ✓
process(a);        // ✓
process(c_array);  // ✓ 连 C 数组都能接
process({v.data() + 1, 2});   // ✓ 还能取子区间

```

常见操作：
```C++
std::span<int> s(vec);

s.size();      s.empty();       // 大小
s[0];          s.front();       // 元素
s.subspan(1, 3);                // 子区间视图（不拷贝）
s.first(2);    s.last(2);       // 前后 n 个
s.data();                       // 底层指针

// optional 化的版本（C++20 后期 / C++23 更完善）
if (auto sub = s.subspan(1); !sub.empty()) { }

```

常见应用：
```C++
// 处理一块图像数据：不关心它来自 cv::Mat、vector<uchar> 还是相机裸缓冲区
void process_roi(std::span<const uint8_t> pixels, int width, int height) {
    for (size_t i = 0; i < pixels.size(); ++i) {
        // ...
    }
}

// 各种来源都能直接喂进去
cv::Mat img = cv::imread("a.png", cv::IMREAD_GRAYSCALE);
process_roi({img.data, static_cast<size_t>(img.total())}, img.cols, img.rows);

std::vector<uint8_t> buf = read_from_camera();
process_roi(buf, w, h);

uint8_t raw[1024];
process_roi(raw, w, h);

```

总之，对于一块连续的内存来说，使用`std::span`比`std::vector`等要通用的多
但是注意：
1. span和string_view一致，都是只作为函数参数，不能作为返回值或者成员
2. span不会检查边界，例如不会检查是否越界，`[]`越界访问不会抛异常

# 四个类型选型决策

```
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