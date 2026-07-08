# InnoDB 存储引擎

## 逻辑存储结构

![[Pasted image 20260708021739.png]]

## 架构

![[Pasted image 20260708021803.png|697]]

## 磁盘结构

![[Pasted image 20260708021831.png]]

![[Pasted image 20260708021841.png]]

## 后台线程

![[Pasted image 20260708021852.png]]

![[Pasted image 20260708021902.png]]

![[Pasted image 20260708021911.png]]

## 事务

![事务](InnoDB_img1.png)

### 持久性保证：Redo Log

**Redo Log Buffer**（物理日志，记录修改的数据）：修改内存 buffer 的数据时写入到 redo log 日志，用于在数据脏页（对该页进行操作后与磁盘中的数据不一致的页）刷新到磁盘失败时，进行恢复操作（重放修改的操作），在磁盘中重新补上修改的数据。

**优点**：日志的顺序 IO 效率高于直接写入磁盘的随机 IO 效率。

![Redo Log](InnoDB_img2.png)

### 原子性保证：Undo Log

**Undo Log Buffer**（逻辑日志，记录修改的操作的反逻辑）：修改内存 buffer 数据时写入修改前的信息。

- **用途 1**：回滚
- **用途 2**：MVCC

### 隔离性：MVCC + 锁

> 详见 [[锁]]

#### MVCC（多版本并发控制）基本概念

1. **当前读**：读取的最新版本，在读取时加锁保证是最新版本
2. **快照读**：简单 SELECT 读取的数据

![MVCC基本概念](InnoDB_img3.png)

#### MVCC 实现原理

**1) 记录中的隐藏字段：**

![隐藏字段](InnoDB_img4.png)

**2) Undo Log 日志：**

用于进行回滚。

**3) Read View：**

![Read View 1](InnoDB_img5.png)

![Read View 2](InnoDB_img6.png)

![Read View 3](InnoDB_img7.png)

> **相关主题**：[[锁]]、[[SQL优化]]、[[为什么使用B+树而不使用其他数据结构]]
