---
tags:
  - CPP/对象模型
  - CPP/虚函数
  - CPP/核心
date: 2026-09-18
---

# C++ 对象模型与虚函数

本篇梳理 C++ 多态的底层机制：对象内存布局、虚函数表（vtable）与虚函数指针（vptr）、RTTI，以及单/多继承下的对象布局与常见陷阱。

> [!important] 核心主线
> **虚函数是 C++ 实现运行时多态的核心机制。** 一条调用链贯穿全篇：
> `对象 → vptr → 虚函数表 → 函数地址 → 调用`
> 理解了这条链路，就理解了 C++ 对象模型。

---

## 一、对象不是只有数据

如果类中出现了虚函数：

```cpp
class Person {
public:
    virtual void speak(); // 虚函数
    int age;
};
```

那么对象通常会多出一个**隐藏成员**：

```text
p 对象内存：
+----------------+
| vptr           | 虚函数表指针
+----------------+
| age            |
+----------------+
```

> [!note] 说明
> 这里的 `vptr` 是**编译器自动添加**的概念性成员，不由程序员手动声明。

---

## 二、静态类型与动态类型

### 2.1 静态类型

```cpp
Animal* animal = &dog;
```

这时 `animal` 的**静态类型**是 `Animal*`（声明时写死的类型，编译期确定）。

### 2.2 动态类型

同样是这段代码：

```cpp
Animal* animal = &dog;
```

但：

```text
animal 的静态类型是: Animal*
animal 所指对象的动态类型为: Dog
```

> [!important] 重点
> **静态类型**看声明（编译期），**动态类型**看实际指向的对象（运行期）。
> 虚函数调用的意义，就是让调用能依据**动态类型**来选择函数版本。

---

## 三、虚函数

### 3.1 什么是虚函数

是在基类成员函数前加 `virtual` 关键字进行声明的函数，允许派生类**重写（override）**。通过基类指针或引用调用时，会根据**实际对象的类型**调用对应版本的派生类成员函数。

> [!important] 重点
> 虚函数是 C++ 实现 ***运行时多态*** 的核心机制。

### 3.2 虚函数调用的底层过程

```cpp
Animal* animal = &dog;
animal->speak();
```

底层调用过程：

```text
animal
  |
  v
+--------------------+
| vptr               | -----------+
+--------------------+            |
| Dog 的数据成员      |             |
+--------------------+            |
                                  |
                                  v
                         +----------------+
                         | 虚函数表       |
                         +----------------+
                         | Dog::speak()   |
                         +----------------+
```

通常理解为：

1. 通过 `animal` 找到 `Dog` 对象；
2. 从 `Dog` 对象中取出 `vptr`；
3. 通过 `vptr` 找到虚函数表；
4. 从虚函数表中取出 `speak` 对应的函数地址；
5. 调用 `Dog::speak()`。

> [!important] 重点：虚函数的开销
> 虚函数调用比普通函数调用**多一次间接寻址**。
> ```text
> 普通函数：直接调用函数地址
> 虚函数：对象 → vptr → 虚函数表 → 函数地址 → 调用
> ```

### 3.3 深入理解虚函数表

针对以下代码：

```cpp
class Animal {
public:
    virtual void speak() {
    }

    virtual void eat() {
    }
};

class Dog : public Animal {
public:
    void speak() override {
    }

    void eat() override {
    }
};
```

可以想象：

```text
Animal 的虚函数表：

+------------------+
| Animal::speak()  |
+------------------+
| Animal::eat()    |
+------------------+

Dog 的虚函数表：

+------------------+
| Dog::speak()     |
+------------------+
| Dog::eat()       |
+------------------+
```

而一个 `Dog` 对象的 `vptr` 通常指向 `Dog` 的虚函数表：

```text
Dog 对象：
+----------------+
| vptr           | --------> Dog 的虚函数表
+----------------+
| Dog 数据成员   |
+----------------+
```

> [!warning] 注意：虚函数表是共享的
> 虚函数表**不是每个对象一份**，而是多个对象**共享同一个虚函数表**；每个对象仅保存一个虚函数表指针 `vptr`。
> ```text
> 多个 Dog 对象共享同一张 Dog 虚函数表
>
> Dog 对象 1 vptr1----\
> Dog 对象 2 vptr2----- > Dog 虚函数表
> Dog 对象 3 vptr3----/
> ```

---

## 四、虚函数带来的内存开销

在常见的 64 位环境中，例如：

```cpp
class A {
public:
    int val;
};

class B {
public:
    int val;
    virtual int printVal();
};
```

`sizeof(A)` 通常是 **4 字节**，但 `sizeof(B)` 可能为 **8 + 4 + 4 字节**：

```text
8: vptr
4: int val
4: 内存对齐填充（可能为 4 字节）
```

> [!warning] 注意：大小不确定
> 具体大小和**编译器、平台以及 ABI** 有关。
> 但需要掌握的工程结论是：***虚函数通常会带来一个虚函数指针（vptr）的开销***。

---

## 五、为什么基类的析构函数要写成虚函数

使用基类指针或引用访问派生类时，如果通过基类进行删除/析构：

1. **基类析构函数不是虚函数**：析构时仅调用基类的析构函数，而**不会调用派生类的析构函数**，导致**内存泄露**。
2. **基类析构函数是虚函数**：析构时**优先调用派生类的析构函数**，之后调用基类的析构函数。

> [!important] 重点
> 如果一个类明确不允许通过基类指针删除对象，也可以不写虚析构函数。但要记住：
> ```text
> 只要类中存在虚函数，通常需要认真考虑虚析构函数
> ```

---

## 六、RTTI

### 6.1 RTTI 是什么

```text
Run-Time Type Information
运行时类型信息
```

C++ 中常见的 RTTI 工具有：

```text
dynamic_cast
typeid
```

### 6.2 dynamic_cast

```cpp
#include <iostream>

class Animal {
public:
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    void bark() {
        std::cout << "Dog bark\n";
    }
};

class Cat : public Animal {
public:
    void meow() {
        std::cout << "Cat meow\n";
    }
};

int main() {
    Animal* animal = new Dog;

    Dog* dog = dynamic_cast<Dog*>(animal); // 使用 dynamic_cast 将父类指针对象转换为派生类指针

    if (dog != nullptr) {
        dog->bark();
    }

    delete animal;

    return 0;
}
```

转换**成功**则得到指定类型的指针，**失败**则转换为 `nullptr`。

> [!warning] 注意：dynamic_cast 的前提
> 通常要求基类**至少有一个虚函数**。如果基类完全没有虚函数，某些 `dynamic_cast` 将无法使用。
>
> 即：**只有多态类型才能使用 `dynamic_cast` 做运行时安全转换。**

### 6.3 typeid 的用法

```cpp
#include <iostream>
#include <typeinfo>

class Animal {
public:
    virtual ~Animal() = default;
};

class Dog : public Animal {
};

int main() {
    Animal* animal = new Dog;

    std::cout << typeid(*animal).name() << '\n';

    delete animal;

    return 0;
}
```

这里输出应当为 `Dog`。

> [!warning] 注意
> `typeid()` 观察的是对象的**动态类型**，前提是 ***类具有多态性质***（即含有虚函数）。

---

## 七、继承下的对象布局

### 7.1 单继承

```cpp
class Base {
public:
    virtual void f();

    int baseValue;
};

class Derived : public Base {
public:
    void f() override;

    int derivedValue;
};
```

粗略画成：

```text
Derived 对象：

+----------------------+
| vptr                 |
+----------------------+
| baseValue            |
+----------------------+
| derivedValue         |
+----------------------+
```

派生类的对象一般**包含一个基类子对象**：

```text
Derived 对象
├── Base 子对象
│   ├── vptr
│   └── baseValue
└── derivedValue
```

而实际上：

```cpp
Base* p = new Derived;
```

> [!important] 重点
> `p` 指向 `Derived` 对象**内部的 `Base` 子对象部分**，**并不指向整体**。

### 7.2 多继承布局

```cpp
class A {
public:
    virtual void fa();
    int a;
};

class B {
public:
    virtual void fb();
    int b;
};

class C : public A, public B {
public:
    void fa() override;
    void fb() override;

    int c;
};
```

`C` 同时继承 `A`、`B`，因此 `C` 对象的布局大致如下：

```text
C 对象：

+----------------------+
| A 子对象              |
| vptr_A               |
| a                    |
+----------------------+
| B 子对象              |
| vptr_B               |
| b                    |
+----------------------+
| c                    |
+----------------------+
```

> [!warning] 注意
> 多继承下，一个对象可能包含**多个 `vptr`**（每个带虚函数的基类子对象各一个）。

### 7.3 多继承中的指针调整

假设内存地址为：

```text
C起始地址 : 1000
A子对象地址 : 1000
B子对象地址 : 1010
```

那么：

```cpp
C* pc = new C;
A* pa = pc;
B* pb = pc;
```

可能会得到：

```text
pa = 1000
pb = 1010
```

> [!important] 重点：指针调整
> 这是多继承中重要的一个概念：
> `基类指针不一定和派生类对象起始地址相同，编译器可能需要进行指针调整`

---

## 八、构造函数和析构函数中不要调用虚函数！

```cpp
#include <iostream>

class Base {
public:
    Base() {
        speak(); // 在构造的时候调用虚函数
    }
    // 这里定义虚函数
    virtual void speak() {
        std::cout << "Base::speak\n";
    }
};

class Derived : public Base {
public:
    // 这里对 speak 进行重写
    void speak() override {
        std::cout << "Derived::speak\n";
    }
};

int main() {
    Derived d; // 构造

    return 0;
}
```

> [!warning] 注意
> ``这时候构造函数仅会调用基类的虚函数``

原因为——构造派生类的顺序为：

```text
先构造基类部分
再构造派生类部分
```

因此，在进行基类部分构造时，派生类部分还未完成构造，**因此仅调用基类的虚函数**。

类似的，析构函数中，会：

```text
先析构派生类部分
再析构基类部分
```

在基类析构时，不会调用已经被析构的派生类部分。

> [!warning] 结论
> **构造 / 析构期间不构成多态**：此时对象的动态类型被"降级"为当前正在构造/析构的那一层，因此不要依赖虚函数实现多态。

---

## 九、总结

### 9.1 一次虚函数调用的完整流程

```cpp
Base* p = new Derived;
p->virtualFunction();
```

这时候的调用流程为：

```text
p 的静态类型是 Base*
        |
        v
指针指向 Derived 对象
        |
        v
对象内部保存 vptr
        |
        v
vptr 指向 Derived 的虚函数表
        |
        v
虚函数表中保存 Derived::virtualFunction
        |
        v
最终调用 Derived::virtualFunction()
```

### 9.2 核心知识点

1. **对象可能包含隐藏的虚函数表指针 `vptr`。**
2. **每种具有虚函数的类通常有对应的虚函数表。**
3. **虚函数调用可以根据对象的动态类型选择函数。**
4. **普通函数调用通常依据静态类型决定。**
5. **虚函数带来间接调用和对象内存开销。**
6. **多态基类通常需要虚析构函数。**
7. **`dynamic_cast` 可以进行运行时安全类型转换。**
8. **多继承可能产生多个虚函数指针和指针调整。**
9. **构造函数和析构函数中不要依赖虚函数实现多态。**
10. **使用 `override` 让编译器检查是否真的重写了虚函数。**

---

> [!tip] 延伸
> 相关学习记录见 [[C++深入/学习进度.md]]，下一步可做实验验证：`虚函数表实验、对象大小、继承布局与 sizeof`。
