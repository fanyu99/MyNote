---
tags:
  - MySQL/DML
  - MySQL/基础
date: 2026-08-04
---

# ✏️ DML（数据操作语言）

> **DML**（Data Manipulation Language）用来对数据库表中的**数据记录**进行增、删、改操作。

| 操作 | 关键字 | 说明 |
|:-----|:------|:------|
| 增 | `INSERT` | 向表中插入新数据 |
| 改 | `UPDATE` | 修改表中已有数据 |
| 删 | `DELETE` | 删除表中数据 |

---

## 一、添加数据（INSERT）

![[Pasted image 20260731145026.png]]

### 1.1 基本语法

```sql
-- 方式 1：给指定字段添加数据（推荐）
INSERT INTO 表名 (字段1, 字段2, ...) VALUES (值1, 值2, ...);

-- 方式 2：给全部字段添加数据
INSERT INTO 表名 VALUES (值1, 值2, ...);

-- 方式 3：批量添加数据
INSERT INTO 表名 (字段1, 字段2, ...) VALUES
    (值1, 值2, ...),
    (值1, 值2, ...),
    (值1, 值2, ...);
```

### 1.2 语法说明

| 写法 | 适用场景 | 注意事项 |
|:-----|:---------|:---------|
| 指定字段 | **推荐写法**，字段和值一一对应 | 字段顺序可以打乱，值与字段顺序一致即可 |
| 全部字段 | 所有字段都要给值 | 值的顺序必须和表中字段**定义顺序完全一致** |
| 批量插入 | 一次插入多条数据 | 性能更高，减少与数据库的交互次数 |

### 1.3 实操示例

假设我们有以下员工表：

```sql
-- 准备测试表
CREATE TABLE employee (
    id        INT          COMMENT '编号',
    work_no   VARCHAR(10)  COMMENT '员工工号',
    name      VARCHAR(10)  COMMENT '员工姓名',
    gender    CHAR(1)      COMMENT '性别',
    age       TINYINT      COMMENT '年龄',
    id_card   CHAR(18)     COMMENT '身份证号',
    entry_date DATE        COMMENT '入职时间'
) COMMENT '员工表';
```

```sql
-- ==================== 插入示例 ====================

-- 1. 指定字段插入（推荐写法）
INSERT INTO employee (id, work_no, name, gender, age, id_card, entry_date)
VALUES (1, 'A001', '张三', '男', 25, '110101199001011234', '2020-03-15');

-- 2. 指定字段插入（字段顺序可以不同）
INSERT INTO employee (name, work_no, id, gender, age, id_card, entry_date)
VALUES ('李四', 'A002', 2, '女', 23, '110101199502022345', '2021-07-01');

-- 3. 全字段插入（值的顺序必须和建表时字段顺序一致！）
INSERT INTO employee VALUES
(3, 'A003', '王五', '男', 30, '110101198501013456', '2018-01-10');

-- 4. 批量插入多条数据
INSERT INTO employee (id, work_no, name, gender, age, id_card, entry_date) VALUES
(4, 'A004', '赵六', '男', 28, '110101199201016789', '2019-06-20'),
(5, 'A005', '孙七', '女', 26, '110101199401019876', '2022-04-01'),
(6, 'A006', '周八', '男', 32, '110101198801011112', '2016-11-10');
```

### 1.4 插入注意事项

| 注意事项 | 说明 |
|:---------|:-----|
| ⚠️ 值与字段一一对应 | 数据类型必须匹配（如 `INT` 不能插入 `'abc'`） |
| ⚠️ 字符串和日期 | 用**单引号** `''` 包裹，不能用双引号 |
| ⚠️ 自增字段 | 一般不用手动插入，设为 `NULL` 即可自动生成 |
| ⚠️ 批量插入 | 一条语句插入多条，性能远优于多次单条 `INSERT` |
| ⚠️ 非空约束 | 有 `NOT NULL` 约束的字段必须给值 |

```sql
-- ❌ 错误：值与字段数量不匹配
INSERT INTO employee (id, name, gender) VALUES (7, '小明');
-- Error: Column count doesn't match value count

-- ❌ 错误：字符串没加引号
INSERT INTO employee (name) VALUES (张三);
-- Error: Unknown column '张三'

-- ✅ 正确
INSERT INTO employee (id, name) VALUES (7, '小明');

-- ✅ 字符串中包含单引号时，用两个单引号转义
INSERT INTO employee (name) VALUES ('O''Brien');
```

---

## 二、修改数据（UPDATE）

![[Pasted image 20260804102644.png]]

### 2.1 基本语法

```sql
UPDATE 表名 SET 字段1 = 值1, 字段2 = 值2, ... [WHERE 条件];
```

> [!warning] 核心原则
> **修改数据一定要加 `WHERE` 条件**，否则会修改表中的**所有数据**！

### 2.2 实操示例

```sql
-- ==================== 修改示例 ====================

-- 1. 修改单个字段（修改 id=1 的员工姓名）
UPDATE employee SET name = '张三丰' WHERE id = 1;

-- 2. 修改多个字段（修改 id=2 的员工年龄和入职日期）
UPDATE employee
SET age = 24, entry_date = '2021-08-01'
WHERE id = 2;

-- 3. 修改所有数据（慎用！必须有充分理由）
UPDATE employee SET entry_date = '2024-01-01';
-- ⚠️ 不加 WHERE 条件 → 表中所有人的入职日期都被改了！

-- 4. 带条件的批量修改（将所有男员工的年龄 +1）
UPDATE employee SET age = age + 1 WHERE gender = '男';

-- 5. 使用表达式修改（给所有 2022 年以后入职的员工加备注，假设有 remark 字段）
UPDATE employee SET age = age + 1 WHERE entry_date >= '2022-01-01';
```

### 2.3 WHERE 条件一览

| 运算符 | 语法示例 | 说明 |
|:-------|:---------|:-----|
| `=` | `WHERE id = 1` | 等于 |
| `<>` 或 `!=` | `WHERE gender <> '男'` | 不等于 |
| `>` | `WHERE age > 25` | 大于 |
| `>=` | `WHERE age >= 30` | 大于等于 |
| `<` | `WHERE age < 25` | 小于 |
| `<=` | `WHERE age <= 30` | 小于等于 |
| `BETWEEN` | `WHERE age BETWEEN 20 AND 30` | 在某个范围内（闭区间） |
| `IN` | `WHERE id IN (1, 3, 5)` | 在列表中 |
| `LIKE` | `WHERE name LIKE '张%'` | 模糊匹配 |
| `IS NULL` | `WHERE entry_date IS NULL` | 为空 |
| `IS NOT NULL` | `WHERE entry_date IS NOT NULL` | 不为空 |
| `AND` | `WHERE age > 25 AND gender = '女'` | 多个条件同时满足 |
| `OR` | `WHERE age < 20 OR age > 50` | 满足任一条件 |

### 2.4 更新注意事项

| 注意事项 | 说明 |
|:---------|:-----|
| ⚠️ **必须加 WHERE** | 不加条件 = 修改全表，生产环境是灾难！ |
| ⚠️ 先查后改 | 先用 `SELECT` 确认 WHERE 条件选中的行是否正确 |
| ⚠️ 事务保护 | 重要数据修改前开事务，确认无误再 `COMMIT` |
| 💡 安全模式 | `SET SQL_SAFE_UPDATES = 0;` 关闭安全模式（开发环境） |

```sql
-- 安全做法：先用 SELECT 验证条件
SELECT * FROM employee WHERE id = 1;
-- 确认无误后再执行
UPDATE employee SET name = '张三丰' WHERE id = 1;
```

---

## 三、删除数据（DELETE）

![[Pasted image 20260804103716.png]]

### 3.1 基本语法

```sql
DELETE FROM 表名 [WHERE 条件];
```

> [!warning] 核心原则
> **删除数据一定要加 `WHERE` 条件**，否则会删除表中的**所有数据**！

### 3.2 实操示例

```sql
-- ==================== 删除示例 ====================

-- 1. 条件删除（删除 id=6 的员工）
DELETE FROM employee WHERE id = 6;

-- 2. 条件删除（删除所有女员工）
DELETE FROM employee WHERE gender = '女';

-- 3. 条件删除（删除 35 岁以上的员工）
DELETE FROM employee WHERE age > 35;

-- 4. 删除所有数据（慎用！）
DELETE FROM employee;
-- ⚠️ 不加 WHERE → 表中所有数据被删除！表结构还在！
```

### 3.3 DELETE vs TRUNCATE

| 对比项 | `DELETE` | `TRUNCATE` |
|:-------|:---------|:-----------|
| 语法 | `DELETE FROM 表名 [WHERE ...];` | `TRUNCATE TABLE 表名;` |
| 加 WHERE | ✅ 支持 | ❌ 不支持（只能清空整表） |
| 删除方式 | 逐行删除 | 整体删除（`DROP` 表 → 重建空表） |
| 速度 | 🐢 慢（记录日志） | ⚡ 快（不记录日志） |
| 自增 ID | **不会重置** | **重置**（重新从 1 开始） |
| 事务回滚 | ✅ 支持（InnoDB） | ❌ 不支持 |
| 触发器 | 触发 | 不触发 |
| 使用场景 | 精确删除部分数据 | 清空整表测试数据 |

```sql
-- 场景对比
DELETE FROM employee WHERE id = 1;
-- → 删除 id=1 这一条记录

TRUNCATE TABLE employee;
-- → 清空所有数据，自增 ID 重置，不可回滚！
```

### 3.4 删除注意事项

| 注意事项 | 说明 |
|:---------|:-----|
| ⚠️ **必须加 WHERE** | 不加条件 = 删除全表数据 |
| ⚠️ 先查后删 | 先用 `SELECT` 确认要被删除的数据 |
| ⚠️ 不可逆 | `DELETE` 可回滚（InnoDB），但 `TRUNCATE` 不行 |
| ⚠️ 外键约束 | 有外键关联时，需要先删子表数据或级联删除 |
| 💡 逻辑删除 | 生产环境常用 `is_deleted` 字段标记，而非物理删除 |

```sql
-- 生产环境推荐：逻辑删除
-- 加一个 is_deleted 字段，TINYINT，0=正常 1=已删除
ALTER TABLE employee ADD is_deleted TINYINT DEFAULT 0 COMMENT '0-正常 1-已删除';

-- "删除" 操作变成更新标记
UPDATE employee SET is_deleted = 1 WHERE id = 1;

-- 查询时过滤掉已删除的记录
SELECT * FROM employee WHERE is_deleted = 0;
```

---

## 四、综合实战

> 通过一个完整的例子练习 DML 三大操作。

```sql
-- ==================== 准备工作 ====================
-- 创建学生表
CREATE TABLE student (
    id          INT          COMMENT '学号',
    name        VARCHAR(20)  COMMENT '姓名',
    gender      CHAR(1)      COMMENT '性别',
    age         TINYINT      COMMENT '年龄',
    score       DOUBLE(4,1)  COMMENT '成绩',
    create_time DATETIME     COMMENT '录入时间'
) COMMENT '学生表';

-- ==================== 1. INSERT ====================
INSERT INTO student (id, name, gender, age, score, create_time) VALUES
(1, '张三',   '男', 20, 88.5, '2024-09-01 10:00:00'),
(2, '李四',   '女', 21, 92.0, '2024-09-01 10:00:00'),
(3, '王五',   '男', 19, 76.5, '2024-09-01 10:00:00'),
(4, '赵六',   '女', 20, 85.0, '2024-09-01 10:00:00'),
(5, '孙七',   '男', 22, 68.0, '2024-09-01 10:00:00'),
(6, '周八',   '女', 21, 95.5, '2024-09-01 10:00:00');

-- ==================== 2. UPDATE ====================
-- 给所有男生成绩 +5 分（加分奖励）
UPDATE student SET score = score + 5 WHERE gender = '男';

-- 将张三的年龄改为 21
UPDATE student SET age = 21 WHERE name = '张三';

-- ==================== 3. DELETE ====================
-- 删除成绩不及格的记录（假设 <60 分不及格）
DELETE FROM student WHERE score < 60;

-- ==================== 4. 验证结果 ====================
SELECT * FROM student ORDER BY score DESC;
```

---

## 📊 DML 速查总览

| 操作 | 关键字 | 核心语法 | 加 WHERE？ |
|:-----|:------|:---------|:----------:|
| 插入（指定字段） | `INSERT` | `INSERT INTO 表名 (字段1, 字段2) VALUES (值1, 值2);` | 不需要 |
| 插入（全字段） | `INSERT` | `INSERT INTO 表名 VALUES (值1, 值2, ...);` | 不需要 |
| 批量插入 | `INSERT` | `INSERT INTO 表名 VALUES (值1, 值2), (值3, 值4), ...;` | 不需要 |
| 修改 | `UPDATE` | `UPDATE 表名 SET 字段1=值1 WHERE 条件;` | ✅ **必须加** |
| 删除 | `DELETE` | `DELETE FROM 表名 WHERE 条件;` | ✅ **必须加** |

---

## 🎯 课后练习

### 练习 1：INSERT 操作

```sql
-- 在 employee 表中再插入 3 条数据（要求分别用指定字段、全字段、批量三种方式）
-- ① 用"指定字段"方式插入一条：id=7, work_no='B001', name='吴九', gender='女'
-- ② 用"全字段"方式插入一条：id=8, work_no='B002', name='郑十', gender='男'
-- ③ 用"批量"方式插入 2 条数据
```

### 练习 2：UPDATE 操作

```sql
-- 在 employee 表上完成以下修改：
-- ① 将 id=1 的员工的年龄修改为 26
-- ② 将所有女员工的年龄 +1
-- ③ 将工号为 'A003' 的员工的入职日期改为 '2019-01-15'
```

### 练习 3：DELETE 操作

```sql
-- 在 employee 表上完成以下删除：
-- ① 删除 id=7 的员工
-- ② 删除所有女员工（先 SELECT 看看会删几条）
-- ③ 删除年龄大于 30 岁的所有员工（先 SELECT 确认）
```

### 练习 4：综合挑战

```sql
-- ① 创建一张"商品表" product（至少包含：id, name, price, stock）
-- ② 插入 5 条商品数据
-- ③ 将库存 < 10 的商品价格打 8 折 (price = price * 0.8)
-- ④ 删除价格 > 1000 的商品
-- ⑤ 查询最终结果
```

---

> 💡 **提示**：`UPDATE` 和 `DELETE` 前一定先用 `SELECT` 配合相同的 `WHERE` 条件预习结果。宁可多一次查询，也不要误改误删数据！
