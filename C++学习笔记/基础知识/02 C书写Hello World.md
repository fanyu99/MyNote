# 💻 C++ 书写 Hello World

> 第一个 C++ 程序，向世界打个招呼！

---

## 📝 完整代码

```cpp
#include <iostream>

using namespace std;

int main()
{
    cout << "hello world" << endl;

    system("pause");

    return 0;
}
```

---

## 🔍 逐行解析

| 代码 | 作用 |
|------|------|
| `#include <iostream>` | **预处理指令**：引入标准输入输出流头文件，让程序能使用 `cout`、`cin` 等功能 |
| `using namespace std;` | **命名空间声明**：告诉编译器使用标准 (`std`) 命名空间，这样写 `cout` 就不需要写 `std::cout` 了 |
| `int main()` | **主函数**：程序的入口，每个 C++ 程序必须有且仅有一个 `main` 函数 |
| `{ ... }` | **函数体**：花括号括起来的代码块，是函数真正执行的部分 |
| `cout << "hello world" << endl;` | **输出语句**：`cout` 向控制台输出内容，`endl` 表示换行 |
| `system("pause");` | **暂停控制台**：防止窗口一闪而过，等待用户按键后关闭 |
| `return 0;` | **返回值**：告诉操作系统程序正常结束（0 = 成功） |

---

## ⚠️ 常见错误

1. **漏掉分号 `;`** → 每行语句结尾必须有分号
2. **漏掉 `#`** → `include <iostream>` 缺少 `#` 会编译失败
3. **花括号不配对** → `{` 和 `}` 数量必须一致
4. **拼写错误** → `cout` 不是 `cot`，`endl` 不是 `end1`

---

## 💡 小结

- `main` 函数是程序入口，必不可少
- `cout <<` 用于输出内容到屏幕
- 每行 C++ 语句以分号 `;` 结尾
- `using namespace std;` 让代码更简洁
