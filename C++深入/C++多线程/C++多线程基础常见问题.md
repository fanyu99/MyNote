---
tags:
  - CPP/多线程
  - CPP/并发
  - CPP/核心
date: 2026-10-06
---

# C++ 多线程基础常见问题

本篇整理并发编程中反复出现的三个核心问题、互斥锁与条件变量的正确使用方式、生产者-消费者模型、原子变量与内存序、线程生命周期、死锁与读写锁，以及“并发安全不等于线程安全”这一常见误区。

> [!important] 核心主线
> **并发编程的关键是明确“数据归属、原子边界、状态传递”。**
> 只要想清楚谁拥有数据、哪些操作必须整体完成、线程之间如何传递状态，大部分多线程错误都可以提前避免。

---

## 一、并发编程需要解决的三个问题

### 1.1 谁拥有数据

例如一个队列：

```cpp
std::queue<int> que;
```

必须明确：

- 哪个线程要修改它？
- 哪个线程要读取它？
- 读取和修改是否能够同时发生？
- 数据的生命周期由谁管理？
- 线程退出后，数据是否仍然有效？

### 1.2 哪些操作必须整体完成

例如：

```cpp
if (!queue.empty()) {
    int value = queue.front();
    queue.pop();
}
```

这个操作必须整体完成（原子操作），否则：

- 线程 A 判断了 `queue` 不为空；
- 线程 B `pop` 了，此时 `queue` 为空；
- 线程 A 访问 `front()` 并 `pop()`，导致崩溃。

这类问题的本质是判断某些连续操作是否必须“要么全做，要么不做”，这就是原子操作。

### 1.3 线程之间怎么传递状态

例如：

```text
生产者：放入数据后通知消费者进行处理
消费者：共享的队列为空时等待
```

因此要注意：条件变量的 `wait()` 必须围绕一个明确的共享状态进行等待和唤醒。例如生产者放入一个数据后调用 `notify_one()`，消费者 `wait()` 直到 `!queue.empty() || stopped()`。

---

## 二、互斥锁的正确使用方式

优先使用 RAII 加锁，例如标准库中的 `std::lock_guard`、`std::unique_lock` 等，实现自动资源回收。

`std::unique_lock` 适合：

- 需要和条件变量配合；
- 需要延迟加锁；
- 需要手动 `unlock` 和手动 `lock`；
- 需要转移锁的所有权。

`std::lock_guard` 适合：

- 进入作用域就加锁；
- 离开作用域自动解锁；
- 中途不需要手动 `unlock` / `lock`；
- 不需要配合条件变量。

> [!warning] 注意
> 如果使用传统的“加锁 + 解锁”方式而不做异常处理，可能出现**异常路径导致死锁**：
> ```cpp
> mutex_.lock();
> some_function();  // 这里可能抛出异常
> mutex_.unlock();   // 异常后永远不会执行，导致死锁
> ```
> 所以一般使用 `std::lock_guard` 等 RAII 加锁。

### 2.1 锁的作用范围要尽可能小

原则是：只锁需要写的共享数据，耗时操作放在锁外。

---

## 三、条件变量的使用方式

公式：

```text
条件变量 + 互斥锁 + 共享状态（例如 stopFlag 等）
```

常用的谓词形式：

```cpp
// 形式一：使用谓词（推荐）
cv.wait(lock, [this]() { return !queue.empty() || stopFlag; });

// 形式二：手写 while 循环
cv.wait(lock);
while (queue.empty()) {
    cv.wait(lock);
}
// 等待直到队列不为空或者线程需要停止
```

### 3.1 注意虚假唤醒

```cpp
if (queue.empty()) {
    cv.wait(lock);
}
```

上述写法是错误的：

- 条件变量返回并不代表条件已经满足；
- 可能唤醒后 `queue` 还是空的，后续线程无法继续处理；
- 发生惊群效应后，如果只使用 `if`，未拿到资源的线程可能会访问空队列。

正确做法是使用 `while` 或带谓词的 `wait`。

---

## 四、生产者-消费者的标准写法

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>

class BlockingQueue {
public:
    void push(int value) {
        {
            // 加锁保护共享队列
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(value);
        }
        // 在外面唤醒，减少锁竞争
        condition_.notify_one(); // 只需要一个消费者处理，不需要惊群
    }

    int pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this] {
            return !queue_.empty() || stopped_;
        });
        // 永远假设唤醒后的队列可能为空
        if (queue_.empty()) {
            throw std::runtime_error("queue stopped");
        }

        int value = queue_.front();
        queue_.pop();
        return value;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        condition_.notify_all(); // 系统停止，需要所有线程都知道要退出
    }

private:
    std::queue<int> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stopped_ = false;
};
```

---

## 五、高频陷阱

### 5.1 只通知并不修改状态

调用 `notify_one()` / `notify_all()` 前，必须先修改受锁保护的共享状态，否则被唤醒的线程可能再次进入等待。

### 5.2 等待条件不是共享状态而是局部状态

等待谓词必须反映不同线程之间共享的、受保护的状态。

### 5.3 析构对象时仍然由线程在等待

线程池通常的析构顺序为：

```text
1. 修改停止状态
2. 通知所有线程
3. 等待线程退出
4. 最后销毁互斥锁、条件变量和共享数据等
```

---

## 六、原子变量适合解决什么问题

原子变量适合解决：

- 一个独立变量的原子读写；
- 简单的计数器；
- 标志位；
- 状态发布；
- 无锁数据结构中的基础操作。

> [!warning] 单个操作是原子的，但复合逻辑不是原子的
> 例如：
> ```cpp
> if (count < limit) {
>     ++count;
> }
> ```
> 这里 `count < limit` 与 `++count` 合起来并不是原子的。多线程下 `count` 可能在判断前后被改变。
> 如果需要保证整体逻辑，需要使用互斥锁或 CAS 循环。

### 6.1 CAS 循环

```cpp
while (count < limit) {
    int current = count.load(); // 获取当前的值

    // 如果 count 等于 current（未被修改），就 +1；否则（被修改）就更新为当前值
    if (count.compare_exchange_weak(current, current + 1)) {
        break;
    }
}
```

### 6.2 `compare_exchange_weak` / `compare_exchange_strong`

- `weak` 允许伪失败，即原子变量等于 `expected` 但仍返回 `false`，因此常用于循环重试：
  ```cpp
  while (!atomic.compare_exchange_weak(expected, desired)) { }
  ```
- `strong` 只有在与 `expected` 不匹配时才失败：
  ```cpp
  atomic.compare_exchange_strong(expected, desired);
  ```

### 6.3 内存序

常见的内存序：

```cpp
std::memory_order_relaxed  // 仅保证该原子操作本身是原子的，不建立线程之间的同步关系
std::memory_order_acquire
std::memory_order_release
std::memory_order_acq_rel
std::memory_order_seq_cst
```

- `relaxed` 适合统计计数、访问次数、只关心最终数值的场景。
- 一般的发布/订阅场景使用 `release` 和 `acquire`：生产者 `release` 后，消费者通过 `acquire` 获取，保证 `release` 之前的所有写入能够被 `acquire` 之后的读取看到。

```cpp
// 发送
data = 42;
ready.store(true, std::memory_order_release); // 保证 data=42 发生在接收的 acquire 之前

// 接收
if (ready.load(std::memory_order_acquire)) {
    use(data); // 这里能够保证 data == 42
}
```

如果仅使用 `relaxed`，可能会导致最终不能正确获取到读取之前的所有写入：

```cpp
// 发送
data = 42;
ready.store(true, std::memory_order_relaxed);

// 接收
if (ready.load(std::memory_order_relaxed)) {
    use(data); // 注意：这时候并不能保证 data == 42！
}
```

`seq_cst` 提供更强的全局顺序保证，最容易理解，但可能限制编译器和 CPU 的优化空间。一般无法证明 `relaxed` 正确时，优先使用默认的 sequentially consistent。

---

## 七、线程的生命周期

### 7.1 joinable 的线程对象析构会终止程序

例如：

```cpp
{
    std::thread t(func);
} // 如果退出作用域，t 析构后仍然 joinable，会触发 std::terminate()
```

所以线程对象析构前一定要 `join`。

一般可以使用 `std::jthread`（C++20），支持析构时自动等待线程结束，支持协作式停止，减少忘记 `join` 的风险。

---

## 八、死锁分析

死锁的发生条件：互斥且占有并等待、不可被剥夺、循环等待。

```cpp
// 操作 1：
std::lock_guard<std::mutex> lockA(a.mutex);
std::lock_guard<std::mutex> lockB(b.mutex);

// 操作 2：
std::lock_guard<std::mutex> lockB(b.mutex);
std::lock_guard<std::mutex> lockA(a.mutex);
```

可能发生：

```text
线程 1 进行操作 1 持有 lockA，线程 2 进行操作 2 持有 lockB，
导致循环等待，死锁。
```

解决方法：使用 `std::scoped_lock` 安全地获取两个互斥锁。

---

## 九、读写锁的使用

`std::shared_mutex` 允许多个读线程同时读取，写线程独占访问。

```cpp
std::shared_mutex mutex;

// 读
std::shared_lock lock(mutex);
read();

// 写
std::unique_lock lock(mutex);
write();
```

适合读操作很多、写操作较少、读操作本身耗时的场景。

---

## 十、并发安全并不等于线程安全

例如下面这个 `Counter` 类本身是线程安全的：

```cpp
class Counter {
public:
    int get() {
        std::lock_guard lock(mutex_);
        return value_;
    }

    void set(int value) {
        std::lock_guard lock(mutex_);
        value_ = value;
    }

private:
    std::mutex mutex_;
    int value_ = 0;
};
```

但一旦使用复合逻辑：

```cpp
if (counter.get() == 0) {
    counter.set(1);
}
```

就可能发生：

```text
线程 1：get() == 0
线程 2：set(1)，线程 1 的判断逻辑失效，counter 并不等于 0 了
线程 1：set(1)
```

因此“线程安全”不等于“并发安全”，整体的业务逻辑需要进行封装。例如：

```cpp
bool try_set_if_zero() {
    std::lock_guard lock(mutex_); // 整体加锁

    if (value_ != 0) {
        return false;
    }

    value_ = 1;
    return true;
}
```

---

## 十一、核心总结

### 11.1 一句话主线

```text
并发三问    → 数据归属、原子边界、状态传递
锁         → RAII、最小粒度、不要在锁内做耗时操作
条件变量   → 必须与互斥锁和共享状态一起使用，注意虚假唤醒
原子变量   → 独立变量原子操作可用，复合逻辑仍需锁或 CAS
线程生命周期 → joinable 线程析构前要 join，可用 std::jthread(C++20)
死锁       → 统一锁顺序，或用 std::scoped_lock
读写锁     → 读多写少时考虑 std::shared_mutex
线程安全   → 单个接口安全不代表组合安全
```

### 11.2 易错点

- 用 `if` 判断条件后调用 `cv.wait(lock)`，应改为 `while` 或带谓词的 `wait`。
- 只 `notify` 不修改共享状态。
- 在锁内执行耗时任务或调用可能阻塞的 `future.get()`。
- 多个锁不按固定顺序获取，导致死锁。
- 认为“每个接口都线程安全”就等于“整体业务逻辑线程安全”。

---

> [!tip] 延伸
> 线程池的完整实现见 [[线程池]]，阻塞队列与线程安全队列设计见 [[C++多线程核心常见问题]]。相关学习记录见 [[C++深入/学习进度.md]]。
