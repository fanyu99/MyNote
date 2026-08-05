---
tags:
  - MySQL/DQL
  - MySQL/基础
date: 2026-08-05
---

# 🔍 DQL（数据查询语言）

> **DQL**（Data Query Language）用来查询数据库表中的**数据记录**，是 SQL 中使用频率最高、内容最丰富的部分。

![[../图片/Pasted image 20260804110019.png]]

---

## 📋 测试数据准备

> 后续所有查询示例基于以下数据，请先执行建表语句。

```sql
-- ==================== 创建员工表 ====================
DROP TABLE IF EXISTS employee;
CREATE TABLE employee (
    id          INT          COMMENT '编号',
    work_no     VARCHAR(10)  COMMENT '工号',
    name        VARCHAR(20)  COMMENT '姓名',
    gender      CHAR(1)      COMMENT '性别',
    age         TINYINT      COMMENT '年龄',
    id_card     CHAR(18)     COMMENT '身份证号',
    work_address VARCHAR(50) COMMENT '工作地址',
    entry_date  DATE         COMMENT '入职日期'
) COMMENT '员工表';

-- ==================== 批量插入数据 ====================
INSERT INTO employee (id, work_no, name, gender, age, id_card, work_address, entry_date) VALUES
(1,  'A001', '张三',   '男', 25, '110101199901011234', '北京',   '2020-03-15'),
(2,  'A002', '李四',   '女', 23, '110101200102022345', '上海',   '2021-07-01'),
(3,  'A003', '王五',   '男', 30, '110101199401013456', '北京',   '2018-01-10'),
(4,  'A004', '赵六',   '男', 28, '110101199601016789', '广州',   '2019-06-20'),
(5,  'A005', '孙七',   '女', 26, '110101199801019876', '上海',   '2022-04-01'),
(6,  'A006', '周八',   '男', 32, '110101199201011112', '深圳',   '2016-11-10'),
(7,  'A007', '吴九',   '女', 24, '110101200001012222', '北京',   '2023-01-05'),
(8,  'A008', '郑十',   '男', 35, '110101198901013333', '杭州',   '2014-08-20'),
(9,  'A009', '王小明', '男', 27, '110101199701014444', '广州',   '2019-12-01'),
(10, 'A010', '刘小红', '女', 29, '110101199501015555', '北京',   '2017-05-15'),
(11, 'A011', '张伟',   '男', 31, '110101199301016666', NULL,     '2015-09-10'),
(12, 'A012', '李丽',   '女', 22, '110101200201017777', '上海',   '2023-07-20'),
(13, 'A013', '陈强',   '男', 33, '110101199101018888', '深圳',   '2015-03-01'),
(14, 'A014', '刘洋',   '男', 26, '110101199801019999', '北京',   '2021-11-15'),
(15, 'A015', '杨柳',   '女', 28, '110101199601011111', '杭州',   '2018-06-10');
```

---

## 一、基础查询

![[../图片/Pasted image 20260804110323.png]]

### 1.1 查询指定字段

```sql
-- 语法
SELECT 字段1, 字段2, ... FROM 表名;

-- ==================== 示例 ====================

-- 查询姓名和性别
SELECT name, gender FROM employee;

-- 查询姓名、年龄、工作地址
SELECT name, age, work_address FROM employee;
```

### 1.2 查询全部字段

```sql
-- 语法
SELECT * FROM 表名;

-- 示例：查询所有员工信息
SELECT * FROM employee;
```

> [!tip] 建议
> 实际开发中**尽量不要使用 `SELECT *`**，因为它会返回所有字段，数据量大时影响性能。应明确写出需要的字段名。

### 1.3 字段起别名（AS）

```sql
-- 语法
SELECT 字段1 AS 别名, 字段2 AS 别名 FROM 表名;

-- AS 可以省略
SELECT 字段1 别名 FROM 表名;
```

```sql
-- ==================== 示例 ====================

-- 给字段起别名
SELECT name AS 姓名, age AS 年龄 FROM employee;

-- 省略 AS（效果相同）
SELECT name 姓名, age 年龄 FROM employee;

-- 别名中有空格或特殊字符时，用引号包裹
SELECT name AS '员工姓名', work_address AS '工作地址' FROM employee;
```

### 1.4 去重（DISTINCT）

```sql
-- 语法
SELECT DISTINCT 字段列表 FROM 表名;

-- ==================== 示例 ====================

-- 查询所有不重复的工作地址
SELECT DISTINCT work_address FROM employee;
-- 结果：北京、上海、广州、深圳、杭州、NULL

-- DISTINCT 对多个字段同时去重（组合不重复）
SELECT DISTINCT gender, work_address FROM employee;
```

---

## 二、条件查询（WHERE）

![[../图片/Pasted image 20260804152206.png]]

```sql
-- 语法
SELECT 字段列表 FROM 表名 WHERE 条件;
```

### 2.1 比较运算符

| 运算符 | 含义 | 示例 |
|:------:|:-----|:-----|
| `=` | 等于 | `WHERE age = 25` |
| `<>` 或 `!=` | 不等于 | `WHERE gender <> '男'` |
| `>` | 大于 | `WHERE age > 25` |
| `>=` | 大于等于 | `WHERE age >= 30` |
| `<` | 小于 | `WHERE age < 25` |
| `<=` | 小于等于 | `WHERE age <= 30` |

```sql
-- ==================== 示例 ====================

-- 查询年龄等于 30 的员工
SELECT * FROM employee WHERE age = 30;

-- 查询年龄不等于 30 的员工
SELECT * FROM employee WHERE age <> 30;

-- 查询年龄大于 25 岁的员工
SELECT * FROM employee WHERE age > 25;

-- 查询年龄大于等于 30 岁的员工
SELECT * FROM employee WHERE age >= 30;
```

### 2.2 逻辑运算符

| 运算符 | 含义 | 示例 |
|:------:|:-----|:-----|
| `AND` | 与（多个条件同时满足） | `WHERE age > 25 AND gender = '男'` |
| `OR` | 或（满足任一即可） | `WHERE age < 20 OR age > 50` |
| `NOT` | 非 | `WHERE NOT age = 25` |

```sql
-- ==================== 示例 ====================

-- 查询年龄在 25~30 岁之间的员工
SELECT * FROM employee WHERE age >= 25 AND age <= 30;

-- 查询 20 岁以下或 33 岁以上的员工
SELECT * FROM employee WHERE age < 20 OR age > 33;

-- 查询不是北京的员工
SELECT * FROM employee WHERE NOT work_address = '北京';
```

### 2.3 BETWEEN ... AND ...

```sql
-- 查询年龄在 25 到 30 岁之间的员工（闭区间，包含边界值）
SELECT * FROM employee WHERE age BETWEEN 25 AND 30;

-- 等价于
SELECT * FROM employee WHERE age >= 25 AND age <= 30;
```

### 2.4 IN（列表匹配）

```sql
-- 查询北京、上海、深圳的员工
SELECT * FROM employee WHERE work_address IN ('北京', '上海', '深圳');

-- 等价于
SELECT * FROM employee
WHERE work_address = '北京' OR work_address = '上海' OR work_address = '深圳';
```

### 2.5 LIKE（模糊匹配）

| 通配符 | 含义 | 示例 |
|:------:|:-----|:-----|
| `%` | 匹配**任意个**字符（含 0 个） | `'张%'` → 张三、张伟、张小龙 |
| `_` | 匹配**单个**字符 | `'张_'` → 张三、张伟（不匹配张小龙） |

```sql
-- ==================== 示例 ====================

-- 查询姓"张"的员工
SELECT * FROM employee WHERE name LIKE '张%';

-- 查询名字中带"小"的员工
SELECT * FROM employee WHERE name LIKE '%小%';

-- 查询姓名为两个字且姓"张"的员工
SELECT * FROM employee WHERE name LIKE '张_';

-- 查询身份证号以 110101 开头的员工
SELECT * FROM employee WHERE id_card LIKE '110101%';
```

### 2.6 空值判断（IS NULL / IS NOT NULL）

```sql
-- 查询没有填写工作地址的员工
SELECT * FROM employee WHERE work_address IS NULL;

-- 查询填写了工作地址的员工
SELECT * FROM employee WHERE work_address IS NOT NULL;
```

> [!warning] 注意
> 判断 `NULL` 不能用 `=` 或 `!=`，必须用 `IS NULL` 或 `IS NOT NULL`。
>
> ```sql
> -- ❌ 错误：查不出任何结果
> SELECT * FROM employee WHERE work_address = NULL;
>
> -- ✅ 正确
> SELECT * FROM employee WHERE work_address IS NULL;
> ```

---

## 三、聚合函数

![[../图片/Pasted image 20260804155522.png]]

> 聚合函数将一列数据作为整体进行**纵向统计计算**，返回**单个值**。

| 函数 | 含义 | 说明 |
|:-----|:-----|:-----|
| `COUNT()` | 统计数量 | 不统计 NULL 值（`COUNT(*)` 除外） |
| `SUM()` | 求和 | 忽略 NULL 值 |
| `AVG()` | 平均值 | 忽略 NULL 值 |
| `MAX()` | 最大值 | 忽略 NULL 值 |
| `MIN()` | 最小值 | 忽略 NULL 值 |

```sql
-- ==================== 示例 ====================

-- 1. COUNT：统计员工总数
SELECT COUNT(*) AS 总人数 FROM employee;              -- 15
SELECT COUNT(id) AS 总人数 FROM employee;             -- 15（不包含 NULL）
SELECT COUNT(work_address) AS 有地址人数 FROM employee; -- 14（NULL 不统计）

-- 2. SUM：所有员工的年龄之和
SELECT SUM(age) AS 年龄总和 FROM employee;            -- 419

-- 3. AVG：所有员工的平均年龄
SELECT AVG(age) AS 平均年龄 FROM employee;            -- 27.9333

-- 4. MAX：最大年龄
SELECT MAX(age) AS 最大年龄 FROM employee;            -- 35

-- 5. MIN：最小年龄
SELECT MIN(age) AS 最小年龄 FROM employee;            -- 22
```

> [!important] COUNT 的三兄弟
> | 写法 | 统计范围 | 使用场景 |
> |:-----|:---------|:---------|
> | `COUNT(*)` | 所有行（含 NULL） | 统计总行数（**最常用**） |
> | `COUNT(字段)` | 该字段不为 NULL 的行 | 统计某字段有值的行数 |
> | `COUNT(1)` | 所有行 | 和 `COUNT(*)` 等价，性能一致 |
>
> **推荐**：统计总行数就用 `COUNT(*)`，语义最清晰。

---

## 四、分组查询（GROUP BY）

![[../图片/Pasted image 20260804160752.png]]

```sql
-- 语法
SELECT 字段列表 FROM 表名 [WHERE 条件]
GROUP BY 分组字段
HAVING 分组后的条件;
```

### 4.1 基本分组

```sql
-- ==================== 示例 ====================

-- 示例 1：按性别分组，统计人数
SELECT gender, COUNT(*) AS 人数
FROM employee
GROUP BY gender;

-- 结果：
-- | gender | 人数 |
-- |--------|------|
-- | 男     |  9   |
-- | 女     |  6   |

-- 示例 2：按工作地址分组，统计每个城市的平均年龄
SELECT work_address, AVG(age) AS 平均年龄
FROM employee
GROUP BY work_address;

-- 示例 3：按性别分组，统计各组平均年龄、最大年龄
SELECT gender, AVG(age) AS 平均年龄, MAX(age) AS 最大年龄, MIN(age) AS 最小年龄
FROM employee
GROUP BY gender;
```

### 4.2 WHERE 与 HAVING 的区别

![[../图片/Pasted image 20260804162248.png]]

| 对比项 | `WHERE` | `HAVING` |
|:-------|:--------|:---------|
| 过滤时机 | **分组之前**过滤 | **分组之后**过滤 |
| 能否用聚合函数 | ❌ 不能 | ✅ 能 |
| 执行顺序 | WHERE → GROUP BY → HAVING | WHERE → GROUP BY → HAVING |
| 适用场景 | 对**原始行**进行过滤 | 对**分组后的结果**进行过滤 |

```sql
-- ==================== 示例 ====================

-- ❌ 错误：WHERE 中不能用聚合函数
SELECT gender, COUNT(*) AS 人数
FROM employee
WHERE COUNT(*) > 3               -- 报错！
GROUP BY gender;

-- ✅ 正确：用 HAVING 过滤聚合结果
SELECT gender, COUNT(*) AS 人数
FROM employee
GROUP BY gender
HAVING COUNT(*) > 3;              -- 人数 > 3 的分组

-- ✅ WHERE + HAVING 组合使用
-- 查询平均年龄 > 25 的城市，且只统计年龄 > 20 的员工
SELECT work_address, AVG(age) AS 平均年龄
FROM employee
WHERE age > 20                    -- 分组前：过滤年龄 > 20 的员工
GROUP BY work_address
HAVING AVG(age) > 25;             -- 分组后：过滤平均年龄 > 25 的城市
```

> [!tip] 记忆口诀
> **WHERE 过滤行，HAVING 过滤组。**
> 能用 WHERE 优先用 WHERE（先过滤后分组，数据量小，效率高）。

---

## 五、排序查询（ORDER BY）

![[../图片/Pasted image 20260805111314.png]]

```sql
-- 语法
SELECT 字段列表 FROM 表名
[GROUP BY ...]
ORDER BY 字段1 [ASC | DESC], 字段2 [ASC | DESC], ...;
```

| 关键字 | 含义 |
|:------:|:-----|
| `ASC` | 升序排列（**默认**，从小到大） |
| `DESC` | 降序排列（从大到小） |

```sql
-- ==================== 示例 ====================

-- 1. 按年龄升序排列（默认）
SELECT name, age FROM employee ORDER BY age ASC;
SELECT name, age FROM employee ORDER BY age;         -- 等价写法

-- 2. 按年龄降序排列
SELECT name, age FROM employee ORDER BY age DESC;

-- 3. 多字段排序：先按年龄升序，年龄相同再按入职日期降序
SELECT name, age, entry_date
FROM employee
ORDER BY age ASC, entry_date DESC;

-- 4. 排序 + 聚合：统计各城市人数，按人数降序排列
SELECT work_address, COUNT(*) AS 人数
FROM employee
GROUP BY work_address
ORDER BY COUNT(*) DESC;
```

---

## 六、分页查询（LIMIT）

![[../图片/Pasted image 20260805114552.png]]

```sql
-- 语法
SELECT 字段列表 FROM 表名 LIMIT 起始索引, 查询记录数;

-- 起始索引从 0 开始！
```

| 场景 | SQL | 说明 |
|:-----|:----|:-----|
| 第 1 页，10 条 | `LIMIT 0, 10` | 起始索引 = 0 |
| 第 2 页，10 条 | `LIMIT 10, 10` | 起始索引 = 10 |
| 第 3 页，10 条 | `LIMIT 20, 10` | 起始索引 = 20 |
| 第 n 页，每页 m 条 | `LIMIT (n-1)*m, m` | **通用公式** |

```sql
-- ==================== 示例 ====================

-- 1. 查询前 5 条数据
SELECT * FROM employee LIMIT 0, 5;

-- 2. 查询第 2 页，每页 5 条（即第 6~10 条）
SELECT * FROM employee LIMIT 5, 5;

-- 3. 查询年龄最大的前 3 名员工
SELECT name, age FROM employee ORDER BY age DESC LIMIT 0, 3;

-- 4. 只写一个数字 = LIMIT 0, n（取前 n 条）
SELECT * FROM employee LIMIT 3;  -- 前 3 条
```

> [!tip] 分页公式
> 起始索引 = (页码 − 1) × 每页记录数

---

## 七、DQL 书写顺序 & 执行顺序

这是面试必考的知识点，书写语句的顺序和执行顺序**完全不同**！

### 7.1 书写顺序

```sql
SELECT   字段列表           -- 7
FROM     表名               -- 1
[WHERE   条件]              -- 2
[GROUP BY 分组字段]         -- 3
[HAVING  分组后过滤条件]    -- 4
[ORDER BY 排序字段]         -- 5
[LIMIT   分页参数];         -- 6
```

### 7.2 执行顺序

```sql
-- 实际的执行顺序（底层）
1. FROM           -- 确定数据来自哪张表
2. WHERE          -- 过滤行数据
3. GROUP BY       -- 对过滤后的数据分组
4. HAVING         -- 过滤分组后的数据
5. SELECT         -- 选择要显示的列
6. ORDER BY       -- 对结果集排序
7. LIMIT          -- 截取分页数据
```

> [!important] 为什么理解执行顺序很重要？
> 写 SQL 时按**书写顺序**，数据库执行按**执行顺序**，这就是为什么：
> - `SELECT` 中定义的别名**不能**在 `WHERE` 中使用（WHERE 先执行）
> - `SELECT` 中定义的别名**可以**在 `ORDER BY` 中使用（ORDER BY 后执行）

```sql
-- ❌ 错误：别名不能在 WHERE 中使用（WHERE 先于 SELECT 执行）
SELECT work_address AS addr, COUNT(*) AS cnt
FROM employee
WHERE addr = '北京'            -- 报错！此时别名还未定义
GROUP BY work_address;

-- ✅ 正确：别名可以在 ORDER BY 中使用
SELECT name, age AS 年龄
FROM employee
ORDER BY 年龄 DESC;             -- 正确！ORDER BY 在 SELECT 之后执行
```

---

## 📊 综合练习

![[../图片/Pasted image 20260805120910.png]]

```sql
-- ==================== 综合练习 ====================

-- 1. 查询年龄为 22、25、28、30 岁的员工信息
SELECT * FROM employee
WHERE age IN (22, 25, 28, 30);

-- 2. 查询男性且年龄 > 25 的员工
SELECT * FROM employee
WHERE gender = '男' AND age > 25;

-- 3. 统计男、女员工的平均年龄
SELECT gender, AVG(age) AS 平均年龄
FROM employee
GROUP BY gender;

-- 4. 查询每个城市的男女人数（透视表效果）
SELECT work_address, gender, COUNT(*) AS 人数
FROM employee
GROUP BY work_address, gender;

-- 5. 查询人数 >= 2 的城市
SELECT work_address, COUNT(*) AS 人数
FROM employee
WHERE work_address IS NOT NULL     -- 排除 NULL
GROUP BY work_address
HAVING COUNT(*) >= 2;

-- 6. 查询 30 岁以下员工人数最多的城市
SELECT work_address, COUNT(*) AS 人数
FROM employee
WHERE age < 30 AND work_address IS NOT NULL
GROUP BY work_address
ORDER BY 人数 DESC
LIMIT 1;

-- 7. 查询"张"姓员工，按年龄降序排列，取前 3 名
SELECT name, age FROM employee
WHERE name LIKE '张%'
ORDER BY age DESC
LIMIT 3;

-- 8. 查询入职最早的 5 名员工姓名和入职日期
SELECT name, entry_date FROM employee
ORDER BY entry_date ASC
LIMIT 5;
```

---

## 📊 DQL 速查总览

| 分类 | 关键字 | 核心语法 | 说明 |
|:-----|:------|:---------|:-----|
| 基础查询 | `SELECT` | `SELECT 字段 FROM 表名;` | 查询指定字段 |
| 全字段 | `*` | `SELECT * FROM 表名;` | 返回所有列 |
| 别名 | `AS` | `SELECT 字段 AS 别名 FROM 表名;` | `AS` 可省略 |
| 去重 | `DISTINCT` | `SELECT DISTINCT 字段 FROM 表名;` | 去除重复行 |
| 条件 | `WHERE` | `WHERE 条件` | 比较、逻辑、IN、BETWEEN、LIKE、IS NULL |
| 聚合 | `COUNT/SUM/AVG/MAX/MIN` | `SELECT COUNT(*) FROM 表名;` | 纵向统计 |
| 分组 | `GROUP BY` | `GROUP BY 字段` | 按字段分组 |
| 分组过滤 | `HAVING` | `HAVING 聚合条件` | 分组后过滤（能用聚合函数） |
| 排序 | `ORDER BY` | `ORDER BY 字段 [ASC\|DESC]` | 默认升序 |
| 分页 | `LIMIT` | `LIMIT 起始, 条数` | 起始索引从 0 开始 |

---

## 🎯 课后练习

### 练习 1：基础查询

```sql
-- ① 使用别名查询所有员工的姓名、年龄、工号（别名分别为"姓名"、"年龄"、"工号"）
-- ② 查询所有不重复的性别（去重）
-- ③ 查询前 5 条记录
```

### 练习 2：条件查询

```sql
-- ① 查询年龄在 25~30 岁之间的员工（用 BETWEEN 和 AND 各写一次）
-- ② 查询住址为空或不在北京、上海的员工
-- ③ 查询姓"刘"且名字只有两个字的员工
-- ④ 查询身份证号以 110101199 开头的员工
```

### 练习 3：聚合和分组

```sql
-- ① 分别统计男员工和女员工的平均年龄
-- ② 统计每个城市的员工人数，按人数降序排列
-- ③ 查询员工人数大于等于 3 的城市（HAVING）
```

### 练习 4：综合查询

```sql
-- ① 查询北京地区年龄最大的男员工信息
-- ② 查询入职时间最晚的 3 名女员工
-- ③ 查询每个城市 30 岁以上员工的人数，只显示人数 ≥ 2 的城市，按人数升序
-- ④ 统计男员工的平均年龄和女员工的平均年龄，只显示一行两列的结果
```

### 练习 5：书写与执行顺序

```sql
-- 根据下面的 DQL 语句，写出其执行顺序（按序号标注）

SELECT work_address, gender, COUNT(*) AS cnt, AVG(age) AS avg_age
FROM employee
WHERE age > 25
GROUP BY work_address, gender
HAVING cnt >= 2
ORDER BY avg_age DESC
LIMIT 5;
```

---

> 💡 **提示**：遇到复杂查询时，先想清楚执行顺序：FROM 拿表 → WHERE 筛行 → GROUP BY 分组 → HAVING 筛组 → SELECT 选列 → ORDER BY 排序 → LIMIT 截取。按这个思路拆解，任何查询都不难！
