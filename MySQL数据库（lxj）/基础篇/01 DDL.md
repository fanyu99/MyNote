# 🏗️ DDL（数据定义语言）

> **DDL**（Data Definition Language）用来定义数据库对象：**数据库、表、字段**等。

---

## 一、DDL — 数据库操作

![[Pasted image 20260727195713.png]]

### 📊 数据库操作语法

| 操作 | 语法 | 说明 |
|------|------|------|
| 查询所有数据库 | `SHOW DATABASES;` | 查看当前服务器中所有数据库 |
| 创建数据库 | `CREATE DATABASE 数据库名;` | 创建新数据库 |
| 创建（防重复） | `CREATE DATABASE IF NOT EXISTS 数据库名;` | 推荐写法，存在则不报错 |
| 使用/切换数据库 | `USE 数据库名;` | 后续操作在此数据库中进行 |
| 查看当前数据库 | `SELECT DATABASE();` | 查看当前用的是哪个数据库 |
| 删除数据库 | `DROP DATABASE 数据库名;` | 谨慎操作！ |
| 删除（防报错） | `DROP DATABASE IF EXISTS 数据库名;` | 推荐写法，不存在也不报错 |

### ✍️ 实操示例

```sql
-- 1. 查看所有数据库
SHOW DATABASES;

-- 2. 创建数据库（推荐加上 IF NOT EXISTS）
CREATE DATABASE IF NOT EXISTS company;

-- 3. 切换到 company 数据库
USE company;

-- 4. 确认当前在哪个数据库
SELECT DATABASE();   -- 输出: company

-- 5. 删除数据库（谨慎！）
DROP DATABASE IF EXISTS company;
```

---

## 二、DDL — 表操作（查询 & 创建）

### 🔍 查询表

![[Pasted image 20260728151345.png]]

```sql
-- 查看当前数据库中所有表
SHOW TABLES;

-- 查看表结构（大致结构：字段名、类型、是否为空等）
DESC 表名;
-- 或
DESCRIBE 表名;

-- 查看建表语句（详细信息：包括存储引擎、字符集、注释等）
SHOW CREATE TABLE 表名;
```

| 命令 | 展示内容 |
|------|---------|
| `DESC` | 表的大致结构（字段、类型、NULL、Key、Default） |
| `SHOW CREATE TABLE` | 建表时的完整 SQL，包含引擎、字符集、注释等详细信息 |

---

### ✏️ 创建表

![[Pasted image 20260728151627.png]]

```sql
CREATE TABLE 表名 (
    字段1 数据类型 [COMMENT '注释'],
    字段2 数据类型 [COMMENT '注释'],
    ...
    字段n 数据类型 [COMMENT '注释']
) [COMMENT '表注释'];
```

#### 实际例子

![[Pasted image 20260728153132.png]]

```sql
CREATE TABLE tb_user (
    id       INT         COMMENT '编号',
    name     VARCHAR(50) COMMENT '姓名',
    age      INT         COMMENT '年龄',
    gender   CHAR(1)     COMMENT '性别'
) COMMENT '用户表';
```

> ⚠️ **注意**：最后一个字段后面**不要加逗号**，每个语句以分号 `;` 结尾。

---

## 三、DDL — 数据类型及案例

> MySQL 的数据类型主要分为三类：**数值类型**、**字符串类型**、**日期时间类型**。

![[Pasted image 20260728154135.png]]

---

### 🔢 数值类型

#### 整型

| 类型 | 大小 | 范围（有符号） | 用途 |
|------|:---:|------|------|
| `TINYINT` | 1 字节 | -128 ~ 127 | 小整数（如年龄、状态码） |
| `SMALLINT` | 2 字节 | -32768 ~ 32767 | 较小整数 |
| `MEDIUMINT` | 3 字节 | -8,388,608 ~ 8,388,607 | 中等整数 |
| `INT` / `INTEGER` | 4 字节 | -2.1×10⁹ ~ 2.1×10⁹ | 常用整数（**最常用**） |
| `BIGINT` | 8 字节 | -9.2×10¹⁸ ~ 9.2×10¹⁸ | 超大整数（如主键自增 ID） |

```sql
-- 示例
CREATE TABLE test_int (
    id       BIGINT COMMENT '主键ID',
    age      TINYINT COMMENT '年龄（0~255足够）',
    score    INT COMMENT '分数'
);
```

#### 浮点数 & 定点数

| 类型 | 大小 | 精度 | 说明 |
|------|:---:|------|------|
| `FLOAT` | 4 字节 | 单精度 | 大约 7 位有效数字 |
| `DOUBLE` | 8 字节 | 双精度 | 大约 15~16 位有效数字 |
| `DECIMAL(M,D)` | 可变 | 定点数 | **高精度**（金额计算用这个） |

```sql
-- FLOAT / DOUBLE
score DOUBLE(4,1);    -- 4 位总长，1 位小数。例: 99.5

-- DECIMAL(M,D)
-- M = 精度（总位数），D = 标度（小数位数）
-- 例: 123.45 → M=5, D=2
price DECIMAL(10,2);  -- 总长 10 位，小数占 2 位，如 99999999.99
```

| 例子 | M（精度） | D（标度） |
|:---:|:--------:|:--------:|
| `123.45` | 5 | 2 |
| `9999.9` | 5 | 1 |
| `0.001` | 4 | 3 |

---

### 📝 字符串类型

![[Pasted image 20260728160130.png]]

| 类型 | 大小 | 说明 |
|------|:---:|------|
| `CHAR(N)` | 0~255 字节 | **定长**字符串 → 性能较高，长度确定时使用 |
| `VARCHAR(N)` | 0~65535 字节 | **变长**字符串 → 性能较差，长度不确定时使用 |
| `TINYBLOB` | 0~255 字节 | 小型二进制数据 |
| `TINYTEXT` | 0~255 字节 | 小型文本 |
| `BLOB` | 0~65535 字节 | 二进制大对象（图片、文件等） |
| `TEXT` | 0~65535 字节 | 大文本 |
| `MEDIUMBLOB` | 0~16MB | 中型二进制数据 |
| `MEDIUMTEXT` | 0~16MB | 中型文本 |
| `LONGBLOB` | 0~4GB | 大型二进制数据 |
| `LONGTEXT` | 0~4GB | 大型文本 |

#### CHAR vs VARCHAR 对比

```sql
-- CHAR：定长，存不满也占满空间，性能好
name CHAR(10);    -- 存 "abc" 依然占用 10 个字符空间

-- VARCHAR：变长，用多少占多少，更省空间
name VARCHAR(10); -- 存 "abc" 只占用 3 个字符空间
```

| 对比维度 | `CHAR` | `VARCHAR` |
|---------|--------|-----------|
| 空间 | 定长，可能有浪费 | 变长，更省空间 |
| 性能 | ⚡ 较高 | 🐢 较差 |
| 适用场景 | 长度固定（如身份证号、性别、手机号） | 长度不定（如姓名、地址、备注） |

---

### 📅 日期时间类型

![[Pasted image 20260728161000.png]]

| 类型 | 格式 | 范围 | 说明 |
|------|------|------|------|
| `DATE` | YYYY-MM-DD | 1000-01-01 ~ 9999-12-31 | 日期值 |
| `TIME` | HH:MM:SS | -838:59:59 ~ 838:59:59 | 时间值 |
| `YEAR` | YYYY | 1901 ~ 2155 | 年份值 |
| `DATETIME` | YYYY-MM-DD HH:MM:SS | 1000-01-01 ~ 9999-12-31 | 混合日期时间值 |
| `TIMESTAMP` | YYYY-MM-DD HH:MM:SS | 1970-01-01 ~ 2038-01-19 | 时间戳（自动时区转换） |

```sql
-- 示例
CREATE TABLE test_date (
    birthday   DATE COMMENT '生日',
    login_time TIME COMMENT '登录时间',
    grad_year  YEAR COMMENT '毕业年份',
    created_at DATETIME COMMENT '创建时间',
    updated_at TIMESTAMP COMMENT '更新时间'
);
```

---

### 🧩 综合案例

> 根据以下需求设计一张员工表：

| # | 需求 | 分析 | 选型 |
|:--:|------|------|------|
| 1 | 编号（纯数字） | 序号，唯一标识 | `INT` |
| 2 | 员工工号（长度 ≤ 10 位） | 可能含字母，长度不定 | `VARCHAR(10)` |
| 3 | 员工姓名（长度 ≤ 10 位） | 中文姓名，长度不定 | `VARCHAR(10)` |
| 4 | 性别（男/女） | 固定 1 个汉字 | `CHAR(1)` |
| 5 | 年龄（不可能为负数） | 小整数 | `TINYINT UNSIGNED` |
| 6 | 身份证号（18 位） | 固定长度，可能含 X | `CHAR(18)` |
| 7 | 入职时间（取年月日） | 只要日期 | `DATE` |

![[Pasted image 20260728162508.png]]

```sql
CREATE TABLE employee (
    id         INT UNSIGNED   COMMENT '编号',
    work_no    VARCHAR(10)    COMMENT '员工工号',
    name       VARCHAR(10)    COMMENT '员工姓名',
    gender     CHAR(1)        COMMENT '性别',
    age        TINYINT UNSIGNED COMMENT '年龄',
    id_card    CHAR(18)       COMMENT '身份证号',
    entry_date DATE           COMMENT '入职时间'
) COMMENT '员工表';
```

---

## 四、DDL — 表操作（修改 & 删除）

### ➕ 添加字段

![[Pasted image 20260728162905.png]]

```sql
ALTER TABLE 表名 ADD 字段名 数据类型(长度) [COMMENT '注释'] [FIRST | AFTER 已有字段];
```

```sql
-- 在 employee 表中添加一个昵称字段
ALTER TABLE employee ADD nickname VARCHAR(20) COMMENT '昵称';

-- 添加到指定位置
ALTER TABLE employee ADD dept_id INT COMMENT '部门ID' AFTER name;  -- 在 name 后
ALTER TABLE employee ADD create_time DATETIME FIRST;               -- 在最前面
```

---

### ✏️ 修改数据类型 / 字段名和类型

![[Pasted image 20260728163623.png]]

```sql
-- 方式 1：MODIFY — 修改字段的数据类型（不改名）
ALTER TABLE 表名 MODIFY 字段名 新数据类型(长度);

-- 方式 2：CHANGE — 修改字段名 + 数据类型（改名）
ALTER TABLE 表名 CHANGE 旧字段名 新字段名 新数据类型(长度) [COMMENT '注释'];
```

```sql
-- MODIFY：只改类型
ALTER TABLE employee MODIFY nickname VARCHAR(30);   -- VARCHAR(20) → VARCHAR(30)

-- CHANGE：改名 + 改类型
ALTER TABLE employee CHANGE nickname nike_name VARCHAR(30) COMMENT '昵称改名为nike_name';
```

| 命令 | 改类型 | 改名字 | 使用场景 |
|------|:---:|:---:|------|
| `MODIFY` | ✅ | ❌ | 只调整数据类型 |
| `CHANGE` | ✅ | ✅ | 既改类型又改名 |

---

### ➖ 删除字段

![[Pasted image 20260728164025.png]]

```sql
ALTER TABLE 表名 DROP 字段名;
```

```sql
-- 删除 employee 表中的 nickname 字段
ALTER TABLE employee DROP nickname;
```

> ⚠️ **删除字段是不可逆的**，数据会丢失，操作前请确认！

---

### 🏷️ 修改表名

![[Pasted image 20260728164551.png]]

```sql
ALTER TABLE 旧表名 RENAME TO 新表名;
```

```sql
-- 将 employee 表改名为 staff
ALTER TABLE employee RENAME TO staff;
```

---

### 🗑️ 删除表

![[Pasted image 20260728164814.png]]

```sql
-- 删除表（如果存在）
DROP TABLE IF EXISTS 表名;

-- 清空表数据（保留表结构，重新创建一张空表）
TRUNCATE TABLE 表名;
```

| 命令 | 作用 | 表结构 | 数据 | 日志 |
|------|------|:---:|:---:|------|
| `DROP TABLE` | 删除整张表 | ❌ 消失 | ❌ 消失 | 逐行记录 |
| `TRUNCATE TABLE` | 清空表数据 | ✅ 保留 | ❌ 清空 | 不逐行记录（快） |

---

## 📊 DDL 速查总览

| 分类 | 操作 | 核心语法 |
|------|------|---------|
| 数据库 | 创建 | `CREATE DATABASE IF NOT EXISTS 库名;` |
| 数据库 | 切换 | `USE 库名;` |
| 数据库 | 删除 | `DROP DATABASE IF EXISTS 库名;` |
| 表-查询 | 查看所有表 | `SHOW TABLES;` |
| 表-查询 | 查看结构 | `DESC 表名;` |
| 表-查询 | 查看建表SQL | `SHOW CREATE TABLE 表名;` |
| 表-创建 | 建表 | `CREATE TABLE 表名 (...);` |
| 表-修改 | 添加字段 | `ALTER TABLE 表名 ADD 字段 类型;` |
| 表-修改 | 修改字段类型 | `ALTER TABLE 表名 MODIFY 字段 新类型;` |
| 表-修改 | 修改字段名+类型 | `ALTER TABLE 表名 CHANGE 旧名 新名 新类型;` |
| 表-修改 | 删除字段 | `ALTER TABLE 表名 DROP 字段;` |
| 表-修改 | 修改表名 | `ALTER TABLE 旧表名 RENAME TO 新表名;` |
| 表-删除 | 删除表 | `DROP TABLE IF EXISTS 表名;` |
| 表-删除 | 清空表数据 | `TRUNCATE TABLE 表名;` |

---

## 🎯 课后练习

> 根据以下需求，独立完成建库建表操作。

---

### 练习 1：学生管理系统

创建名为 `school` 的数据库，在其中创建 `student` 表：

| 需求 | 字段名 | 数据类型 | 备注 |
|------|--------|---------|------|
| 学号（纯数字） | `id` | 合适的整型 | 主键标识 |
| 姓名（最长 20 位） | `name` | 选 CHAR 还是 VARCHAR？ | |
| 性别（男/女） | `gender` | 固定 1 个字 | |
| 年龄（0~100） | `age` | 选最小够用的整型 | 不可为负数 |
| 手机号（11 位） | `phone` | 固定长度 | |
| 入学日期 | `enroll_date` | 只需要年月日 | |
| 身高（范围 100.0~250.0 cm） | `height` | 小数，精度自己判断 | |

---

### 练习 2：商品管理系统

创建数据库 `shop`，在其中创建 `product` 表：

| 需求 | 字段名 | 数据类型 | 备注 |
|------|--------|---------|------|
| 商品编号 | `id` | 超大整数 | 自动增长 |
| 商品名称（最长 100 位） | `name` | 长度不定 | |
| 价格 | `price` | 小数，精确到分 | 最大 99999999.99 |
| 库存数量 | `stock` | 整型 | |
| 是否上架 | `is_online` | 用 CHAR(1) 存 Y/N | |
| 创建时间 | `created_at` | 包含日期+时间 | |

---

### 练习 3：DDL 修改操作

在练习 1 的 `student` 表上完成以下操作：

```sql
-- ① 添加一个"邮箱"字段 email，类型 VARCHAR(50)
-- ② 把 phone 字段改成 VARCHAR(15)，因为国际号码可能超过 11 位
-- ③ 把 email 字段改名为 contact_email
-- ④ 删除 height 字段
-- ⑤ 把 student 表名改为 students
-- ⑥ 查看 students 的表结构，确认以上修改是否生效
```

---

### 练习 4：综合挑战

```sql
-- 自己设计一张"图书信息表"（books），包含至少 6 个字段
-- 要求：
--   ① 同时使用到 CHAR 和 VARCHAR（说明你为什么选）
--   ② 使用到至少一种日期类型
--   ③ 使用到 DECIMAL 类型
--   ④ 为每个字段写好 COMMENT 注释
-- 建完表后用 SHOW CREATE TABLE 查看完整建表语句
```

---

> 💡 **提示**：建表前先用 `DROP TABLE IF EXISTS` 确保表不存在，可以有效避免重复创建时报错。动手敲一遍比看十遍更有用！
