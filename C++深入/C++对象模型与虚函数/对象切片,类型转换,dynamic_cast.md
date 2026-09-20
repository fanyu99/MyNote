---
tags:
  - CPP/对象模型
  - CPP/类型转换
  - CPP/核心
date: 2026-09-20
---

# 对象切片、类型转换与 dynamic_cast

承接 [[对象大小,内存对齐和继承布局|对象大小、内存对齐与继承布局]]，本篇聚焦多态对象最容易踩坑的两个环节：用派生类对象**按值**构造/赋值基类对象导致的**对象切片**，以及向下转型时 `static_cast` 与 `dynamic_cast` 的选择。菱形继承与虚继承另见 [[菱形继承与虚继承]]。

> [!important] 核心主线
> **多态对象必须通过指针或引用传递。**
> 一旦"按值"复制，派生类部分就被丢弃（对象切片）；而向下转型时，只有 `dynamic_cast` 会在**运行期**校验真实类型。

---

## 一、什么是对象切片

先看代码：

```cpp
class Animal {
public:
    virtual ~Animal() = default;
    virtual void speak() const {}
    int age = 1;
};

class Dog : public Animal {
public:
    void speak() const override {}
    int strength = 100;
};

Dog dog;
Animal animal = dog; // 注意：这里构造的是一个全新的 Animal 对象
```

此时 `dog` 是一个完整的 `Dog` 对象：

```text
Dog 完整对象
+----------------------+
| Animal 基类子对象     |
|   vptr               |
|   age                |
+----------------------+
| Dog 自己的部分        |
|   strength           |
+----------------------+
```

而 `animal` 会**只用 `dog` 中的 `Animal` 基类子对象**去构造一个新的 `Animal` 对象，`Dog` 自己那部分（`strength`）被直接丢弃——这就是**对象切片**。

> [!warning] 后果：新对象的多态消失
> 切片出来的 `animal` 是一个纯粹的 `Animal`，因此 `animal.speak()` 只会调用 `Animal::speak()`，而不会调用 `Dog::speak()`。
> 这一点很好理解：`animal` 仅仅复制了原 `dog` 的 **`Animal` 基类子对象**。

> [!warning] 什么时候发生对象切片？
> 对象切片发生在**用派生类对象去初始化/赋值基类对象**的复制操作中，包括：
> **初始化、赋值、按值传参、按值返回**。
> 而**基类指针或基类引用不会发生对象切片**（它们不复制对象本体）。
>
> ```cpp
> // 返回值：仅返回 dog 的 Animal 基类子对象
> Animal createAnimal() {
>     Dog dog;
>     return dog;
> }
>
> // 参数：按值传递，仅复制 Animal 基类子对象
> void makeSound(Animal animal) {
>     animal.speak(); // 调用 Animal::speak()
> }
>
> // 初始化
> Dog dog1;
> Animal a1 = dog1; // 仅复制 dog1 的 Animal 基类子对象
>
> // 赋值
> Animal a2;
> Dog dog2;
> a2 = dog2; // a2 不会变成 Dog，只是被赋值为 dog2 的 Animal 基类子对象部分
> ```

三种写法的对比：

| 写法 | 是否创建新对象 | 是否切片 | 动态类型 | 虚函数结果 |
|---|---|---|---|---|
| `Animal a = dog;` | 是 | 是 | `Animal` | `Animal::speak()` |
| `Animal* p = &dog;` | 否 | 否 | `Dog` | `Dog::speak()` |
| `Animal& r = dog;` | 否 | 否 | `Dog` | `Dog::speak()` |

---

## 二、函数参数中的对象切片

### 2.1 错误写法：按值传递

```cpp
void makeSound(Animal animal) {
    animal.speak();
}

Dog dog;
makeSound(dog); // 按值传递 → 对象切片，多态消失
```

它等价于：

```cpp
Animal animal = dog; // 发生对象切片
```

因此最终调用的结果是：

```text
Animal::speak()
```

### 2.2 正确写法：按引用或指针传递

```cpp
Dog dog;

// 不需要修改原对象 → 传 const 引用
void makeSound(const Animal& animal) {
    animal.speak();
}
makeSound(dog); // 调用 Dog::speak()

// 需要修改原对象 → 传指针
void makeSound(Animal* animal) {
    if (animal) {
        animal->speak();
    }
}
makeSound(&dog); // 调用 Dog::speak()
```

---

## 三、容器中的对象切片

### 3.1 问题：容器里存放的是值

```cpp
#include <vector>

class Cat : public Animal {
public:
    void speak() const override {}
};

std::vector<Animal> animals; // 容器元素类型是 Animal，不是 Animal*
Dog dog;
Cat cat;
animals.push_back(dog); // 发生对象切片
animals.push_back(cat); // 发生对象切片
```

`push_back` 会以**值**的方式拷贝对象，因此最终进入容器的只有：

```text
dog 的 Animal 基类子对象部分的复制
cat 的 Animal 基类子对象部分的复制
```

### 3.2 正确写法：容器里存放智能指针

多态容器通常保存**指针**，从而避免切片并保留运行时多态：

```cpp
#include <memory>
#include <vector>

std::vector<std::unique_ptr<Animal>> animals;

animals.push_back(std::make_unique<Dog>());
animals.push_back(std::make_unique<Cat>());

// 通过指针调用，多态不会消失
for (const auto& animal : animals) {
    animal->speak();
}
```

---

## 四、向下转型：`static_cast` 的风险

把基类指针转成派生类指针叫作**向下转型（downcast）**。如果用 `static_cast`，编译器**不会校验对象的真实动态类型**：

```cpp
Animal* animal = new Cat;             // 实际指向 Cat 对象
Dog* dog = static_cast<Dog*>(animal); // 编译期直接转换，不做运行期检查
dog->speak();                         // 却当成 Dog* 来使用
```

> [!warning] 注意：`static_cast` 向下转型可能是未定义行为
> `static_cast` 只依赖编译期已知的继承关系，**不检查动态类型**。
> 上例中 `animal` 实际指向 `Cat`，却被当作 `Dog*` 使用，属于**未定义行为**：程序可能看似正常，也可能崩溃，且难以排查。
> 需要运行期安全校验时，应改用 `dynamic_cast`。

---

## 五、`dynamic_cast` 与 RTTI

真正安全的向下转型依赖 `dynamic_cast`（RTTI 的一部分，参见 [[C++对象模型和虚函数#^d31090|对象模型与虚函数 · RTTI]]）。

### 5.1 用法与失败行为

```cpp
Animal* animal = new Cat;

Dog* dog = dynamic_cast<Dog*>(animal); // 运行期校验，此处转换失败
if (dog != nullptr) {
    dog->speak();
} else {
    // 走到这里：真实类型不是 Dog
}
```

失败时的行为：

```text
指针（转 D*）  → 得到 nullptr
引用（转 D&）  → 抛出 std::bad_cast 异常（定义于 <typeinfo>）
```

引用版本：

```cpp
#include <typeinfo>

try {
    Dog& dog = dynamic_cast<Dog&>(*animal); // 失败时抛出 std::bad_cast
    dog.speak();
} catch (const std::bad_cast&) {
    // 处理转换失败
}
```

> [!warning] 前提：基类必须是多态类型
> `dynamic_cast` 的向下转型要求**基类至少有一个虚函数**（即多态类型）。
> 如果基类完全没有虚函数，编译期就会报错。

### 5.2 `static_cast` 与 `dynamic_cast` 对比

| 对比项 | `static_cast` | `dynamic_cast` |
|---|---|---|
| 主要检查时间 | 编译期 | 运行期 |
| 是否验证真实动态类型 | 通常不验证 | 验证 |
| 错误向下转型 | 可能导致未定义行为 | 指针返回空，引用抛异常 |
| 是否依赖多态类型 | 一般不要求 | 向下转型通常要求 |
| 性能 | 通常开销较小 | 有运行时类型检查 |
| 适用场景 | 转换关系已由程序逻辑保证 | 运行时不确定真实类型 |
| 安全性 | 依赖程序员保证 | 更安全 |

---

## 六、核心总结

### 6.1 一句话主线

```text
传递多态对象 → 指针 / 引用（避免对象切片）
向下转型     → 优先 dynamic_cast（运行期校验真实类型）
```

### 6.2 速查表

| 问题 | 答案 |
| --- | --- |
| 什么是对象切片？ | 用派生类对象按值构造/赋值基类对象时，派生类部分被丢弃 |
| 切片后多态还在吗？ | 不在，新对象是纯基类对象 |
| 什么时候会切片？ | 初始化、赋值、按值传参、按值返回 |
| 指针/引用会切片吗？ | 不会，它们不复制对象本体 |
| 多态容器怎么写？ | 存放 `std::unique_ptr<Base>` 等指针，而非值对象 |
| `static_cast` 向下转型安全吗？ | 不安全，不校验动态类型，错误时是未定义行为 |
| `dynamic_cast` 失败怎么办？ | 指针得 `nullptr`，引用抛 `std::bad_cast` |
| `dynamic_cast` 有什么前提？ | 源类型必须是多态类型（含虚函数） |

### 6.3 易错点

- 函数形参写成 `void f(Animal animal)` 是最隐蔽的切片来源——改用 `const Animal&`。
- `std::vector<Animal>` 存值必然切片——需要多态就存指针。
- 只要用到 `static_cast` 向下转型，就要自己承担"类型确实正确"的举证责任。

---

> [!tip] 延伸
> 相关学习记录见 [[C++深入/学习进度.md]]，上一篇见 [[对象大小,内存对齐和继承布局|对象大小、内存对齐与继承布局]]，下一篇见 [[菱形继承与虚继承]]。
