---
tags:
  - CPP/多线程
  - CPP/并发
  - CPP/核心
date: 2026-10-06
---

# C++ 多线程核心常见问题

本篇围绕“线程安全队列”这一核心并发组件展开：先看一个朴素实现的缺陷，再用条件变量改写为阻塞队列，并总结设计线程安全接口时的关键原则。

> [!important] 核心主线
> **线程安全接口应该封装完整的状态转换逻辑。**
> 不要把“判断队列是否为空”和“取元素”拆成两个公开接口让调用方组合，否则调用方仍需自己加锁；单个成员函数的线程安全不代表多个成员函数组合后仍然线程安全。

---

## 一、设计线程安全队列

先看下面的代码：

```cpp
#include <queue>
#include <mutex>

class SafeQueue {
private:
    std::queue<int> queue_;
    std::mutex mutex_;

public:
    void push(int value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
    }

    bool try_pop(int& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return false;
        }

        value = queue_.front();
        queue_.pop();
        return true;
    }
};
```

### 1.1 `try_pop` 的问题

如果使用这个函数，队列为空时会直接返回，调用方往往会写成 `while` 循环反复调用，导致忙等和 CPU 空转。

更好的方式：使用条件变量进行等待，队列不为空时再唤醒。

### 1.2 使用条件变量解决

```cpp
std::unique_lock<std::mutex> lock(mutex);
condition.wait(lock, [this]() { return !queue.empty(); });
```

`wait()` 的流程为：

```text
1. 当前线程持有 mutex_
2. 检查谓词，发现 queue_ 为空，谓词为 false
3. 原子地释放 mutex_
4. 当前线程进入等待状态
5. 其他线程可以获取 mutex_
6. 生产者放入任务
7. 生产者调用 notify_one()
8. 消费者被唤醒
9. 消费者重新获取 mutex_
10. 再次检查谓词
11. 队列非空后继续执行
```

关键点：`wait` 等待时会释放锁，被唤醒后会重新持有锁。

---

## 二、完整示例

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>

template <typename T>
class BlockingQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool closed_ = false;

public:
    bool push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (closed_) {
                return false;
            }

            queue_.push(std::move(value));
        }

        condition_.notify_one();
        return true;
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this] {
            return closed_ || !queue_.empty();
        }); // 等待直到停止或者队列不为空

        if (queue_.empty()) {
            // 这里说明 closed_ == true
            return false;
        }

        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }

        condition_.notify_all();
    }
};
```

> [!warning] 注意一个重要的并发原则
> 单个成员函数的线程安全，不代表多个成员函数组合后线程安全，复合逻辑也不一定线程安全。
> ```cpp
> if (!queue.empty()) {
>     auto value = queue.pop();
> }
> ```
> 并不是线程安全的！
> 更好的设计是直接创建表达完整操作的接口。
> 例如这里的 `pop` 是一个线程安全的完整逻辑函数，已经包含了等待队列不为空：
> ```cpp
> auto value = queue.pop();
> ```

---

## 三、重要知识点

1. 线程安全接口应该封装完整的状态转换逻辑。
2. `wait` 等待时会释放锁。
3. 多锁操作必须统一锁顺序。

---

## 四、核心总结

### 4.1 一句话主线

```text
朴素线程安全队列  →  try_pop 导致忙等
阻塞队列          →  条件变量 + 谓词 + 关闭标志
设计原则          →  接口封装完整操作，不要暴露需要调用方组合的状态检查
```

### 4.2 易错点

- 把 `empty()` 和 `pop()` 拆成两个接口让调用方组合，仍然会出现竞态。
- 使用条件变量时忘记使用谓词，导致虚假唤醒后访问空队列。
- `wait` 被唤醒后默认条件已经满足，没有再次检查谓词。
- 多锁场景下各个线程获取锁的顺序不一致，导致死锁。

---

> [!tip] 延伸
> 更多互斥锁、条件变量、原子变量、死锁与读写锁的内容见 [[C++多线程基础常见问题]]；一个完整的线程池实现见 [[线程池]]。相关学习记录见 [[C++深入/学习进度.md]]。
