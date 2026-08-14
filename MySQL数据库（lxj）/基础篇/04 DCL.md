---
tags:
  - MySQL/DCL
  - MySQL/基础
date: 2026-08-05
---

# 🔐 DCL（数据控制语言）

> **DCL**（Data Control Language）用来管理**数据库用户**和**控制数据库的访问权限**。用户信息和权限信息全部存放在 MySQL 系统库 `mysql` 中。

> [!important] 核心理解
> DCL 做两件事：**谁能登录**（用户管理） + **登录后能干什么**（权限控制）。

---

## 一、用户管理

![[Pasted image 20260805150505.png]]

### 1.1 用户信息存储在哪？

MySQL 的用户信息存储在**系统数据库 `mysql` 的 `user` 表**中：

```sql
-- 查看 mysql 数据库中的所有表
USE mysql;
SHOW TABLES;

-- 查看 user 表结构（核心字段：Host, User, 各种权限字段）
DESC user;

-- 查看当前有哪些用户
SELECT Host, User FROM mysql.user;
```

### 1.2 创建用户

```sql
-- 语法
CREATE USER '用户名'@'主机名' IDENTIFIED BY '密码';
```

| `@` 后面 | 含义 | 使用场景 |
|:----------|:-----|:---------|
| `'localhost'` | 只能从**本机**登录 | 开发环境、本地应用 |
| `'%'` | 可以从**任意主机**远程登录 | 允许远程连接 |
| `'192.168.1.%'` | 从指定网段登录 | 内网指定机器 |

```sql
-- ==================== 创建用户示例 ====================

-- 1. 创建只能在本机登录的用户
CREATE USER 'dev'@'localhost' IDENTIFIED BY '123456';

-- 2. 创建可以在任意主机登录的用户
CREATE USER 'app'@'%' IDENTIFIED BY 'app123456';

-- 3. 创建指定网段的用户
CREATE USER 'ops'@'192.168.1.%' IDENTIFIED BY 'ops123456';

-- 4. 创建用户（推荐：加上 IF NOT EXISTS 的思想）
-- MySQL 8.0 不支持 CREATE USER IF NOT EXISTS，可先尝试创建，或用下面的方法避免报错：
```

> [!tip] 开发环境 vs 生产环境
> - 开发环境：用简单密码（`123456`），允许 `localhost` 和 `%`
> - 生产环境：用**强密码**（大小写+数字+特殊字符），只给 `localhost` 或指定 IP

### 1.3 修改用户密码

```sql
-- ==================== 修改密码 ====================

-- MySQL 8.0：修改当前用户密码
ALTER USER '用户名'@'主机名' IDENTIFIED BY '新密码';

-- 示例：修改 dev 用户的密码
ALTER USER 'dev'@'localhost' IDENTIFIED BY 'dev666666';

-- 或者用 SET PASSWORD（MySQL 8.0 也支持）
SET PASSWORD FOR 'dev'@'localhost' = 'dev666666';
```

### 1.4 删除用户

```sql
-- 语法
DROP USER '用户名'@'主机名';

-- 示例
DROP USER 'dev'@'localhost';
DROP USER 'app'@'%';
```

> [!warning] 注意事项
> - 删除用户时必须指定 `'用户名'@'主机名'`，因为 MySQL 中 `'dev'@'localhost'` 和 `'dev'@'%'` 是**两个不同的用户**！
> - 删除后用户立即无法登录，无需重启 MySQL。

---

## 二、权限控制

![[Pasted image 20260805155749.png]]
![[Pasted image 20260805155918.png]]

> 用户创建好后，默认**没有任何权限**（连查看数据库都不行），需要管理员手动分配。

### 2.1 常用权限列表

| 权限 | 可执行的操作 |
|:-----|:------------|
| `ALL` / `ALL PRIVILEGES` | 所有权限（**除了 GRANT 本身**） |
| `SELECT` | 查询数据 |
| `INSERT` | 插入数据 |
| `UPDATE` | 修改数据 |
| `DELETE` | 删除数据 |
| `CREATE` | 创建数据库 / 表 |
| `DROP` | 删除数据库 / 表 |
| `ALTER` | 修改表结构 |
| `INDEX` | 创建 / 删除索引 |
| `GRANT OPTION` | 允许该用户将自己的权限授予其他用户 |

### 2.2 授予权限（GRANT）

```sql
-- 语法
GRANT 权限列表 ON 数据库名.表名 TO '用户名'@'主机名';
```

```sql
-- ==================== 授权示例 ====================

-- 1. 授予 dev 用户对所有数据库的所有权限（超级管理员）
GRANT ALL PRIVILEGES ON *.* TO 'dev'@'localhost';

-- 2. 授予 app 用户对 test_db 数据库的全部权限
GRANT ALL PRIVILEGES ON test_db.* TO 'app'@'%';

-- 3. 授予 app 用户对 test_db 库中 employee 表的查询和插入权限
GRANT SELECT, INSERT ON test_db.employee TO 'app'@'%';

-- 4. 授予只读权限（所有数据库只读）
GRANT SELECT ON *.* TO 'readonly'@'%';

-- 5. 授予所有权限 + 允许他转授权限给其他人（GRANT OPTION）
GRANT ALL PRIVILEGES ON *.* TO 'admin'@'localhost' WITH GRANT OPTION;
```

| 权限范围 | 写法 | 说明 |
|:---------|:-----|:-----|
| 所有数据库的所有表 | `*.*` | 全局权限 |
| 某个数据库的所有表 | `数据库名.*` | 数据库级权限 |
| 某个数据库的某张表 | `数据库名.表名` | 表级权限 |
| 某个表的某些列 | `GRANT SELECT(列1,列2) ON ...` | 列级权限（不常用） |

### 2.3 查看权限

```sql
-- 查看指定用户的权限
SHOW GRANTS FOR '用户名'@'主机名';

-- ==================== 示例 ====================

SHOW GRANTS FOR 'dev'@'localhost';
-- 输出类似：
-- GRANT ALL PRIVILEGES ON *.* TO 'dev'@'localhost'
-- GRANT GRANT OPTION ON *.* TO 'dev'@'localhost'
```

### 2.4 撤销权限（REVOKE）

```sql
-- 语法
REVOKE 权限列表 ON 数据库名.表名 FROM '用户名'@'主机名';

-- ==================== 撤销权限示例 ====================

-- 1. 撤销 app 用户对 test_db.employee 的 DELETE 权限
REVOKE DELETE ON test_db.employee FROM 'app'@'%';

-- 2. 撤销 app 用户对 test_db 数据库的所有权限
REVOKE ALL PRIVILEGES ON test_db.* FROM 'app'@'%';

-- 3. 撤销 readonly 用户对所有数据库的 SELECT 权限
REVOKE SELECT ON *.* FROM 'readonly'@'%';
```

### 2.5 刷新权限

```sql
-- 执行 GRANT 或 REVOKE 后，通常需要刷新使其立即生效
FLUSH PRIVILEGES;
```

> 大部分情况下，`GRANT`/`REVOKE` 即时生效；但直接修改 `mysql.user` 等系统表后，**必须**执行 `FLUSH PRIVILEGES` 才能生效。

---

## 三、完整操作流程

> 以下是一个从创建用户到分配权限的完整示例：

```sql
-- ==================== 完整流程示例 ====================

-- 步骤 1：创建数据库和测试表
CREATE DATABASE IF NOT EXISTS company;
USE company;

CREATE TABLE employee (
    id   INT PRIMARY KEY COMMENT '编号',
    name VARCHAR(20) COMMENT '姓名'
);

INSERT INTO employee VALUES (1, '张三'), (2, '李四');

-- 步骤 2：创建用户
-- 开发人员：本机登录
CREATE USER 'dev_user'@'localhost' IDENTIFIED BY 'dev123';

-- 应用服务：远程连接
CREATE USER 'app_user'@'%' IDENTIFIED BY 'app123';

-- 只读用户：只能看
CREATE USER 'guest'@'%' IDENTIFIED BY 'guest123';

-- 步骤 3：分配权限
-- 开发人员：对 company 库有全部权限
GRANT ALL PRIVILEGES ON company.* TO 'dev_user'@'localhost';

-- 应用服务：只需要数据的增删改查权限
GRANT SELECT, INSERT, UPDATE, DELETE ON company.* TO 'app_user'@'%';

-- 访客：只读权限
GRANT SELECT ON company.* TO 'guest'@'%';

-- 步骤 4：刷新权限
FLUSH PRIVILEGES;

-- 步骤 5：验证权限
SHOW GRANTS FOR 'dev_user'@'localhost';
SHOW GRANTS FOR 'app_user'@'%';
SHOW GRANTS FOR 'guest'@'%';

-- 步骤 6：用不同用户登录测试
-- 退出后用 mysql -u dev_user -p 登录，测试是否能操作 company 库

-- 步骤 7：清理（不再需要时）
REVOKE ALL PRIVILEGES ON company.* FROM 'dev_user'@'localhost';
REVOKE ALL PRIVILEGES ON company.* FROM 'app_user'@'%';
REVOKE ALL PRIVILEGES ON company.* FROM 'guest'@'%';
DROP USER 'dev_user'@'localhost';
DROP USER 'app_user'@'%';
DROP USER 'guest'@'%';
```

---

## 📊 DCL 速查总览

| 操作 | 核心语法 | 说明 |
|:-----|:---------|:-----|
| 创建用户 | `CREATE USER '用户名'@'主机' IDENTIFIED BY '密码';` | `@'%'` = 任意主机 |
| 修改密码 | `ALTER USER '用户名'@'主机' IDENTIFIED BY '新密码';` | MySQL 8.0 推荐写法 |
| 删除用户 | `DROP USER '用户名'@'主机';` | 必须指定主机 |
| 授予权限 | `GRANT 权限 ON 库.表 TO '用户名'@'主机';` | `*.*` = 全局 |
| 查看权限 | `SHOW GRANTS FOR '用户名'@'主机';` | 查看某个用户的权限 |
| 撤销权限 | `REVOKE 权限 ON 库.表 FROM '用户名'@'主机';` | 和 GRANT 的 ON 范围要一致 |
| 刷新权限 | `FLUSH PRIVILEGES;` | 手动修改系统表后必须执行 |

---

## 🎯 课后练习

### 练习 1：用户管理

```sql
-- ① 创建一个只在本地登录的用户 local_user，密码自定
-- ② 创建一个可以在任意主机登录的用户 remote_user，密码自定
-- ③ 查看 mysql.user 表，确认这两个用户是否存在
-- ④ 修改 local_user 的密码为新密码
-- ⑤ 删除 remote_user
```

### 练习 2：权限管理

```sql
-- ① 创建数据库 test_dcl 和一张表 users(id INT, name VARCHAR(20))
-- ② 创建用户 test_dcl_user@'%'，密码自定
-- ③ 给 test_dcl_user 分配对 test_dcl 库的全部权限
-- ④ 查看 test_dcl_user 的权限
-- ⑤ 撤销 test_dcl_user 对 test_dcl 库的 DELETE 权限
-- ⑥ 再次查看权限，确认 DELETE 权限已被撤销
```

### 练习 3：综合思考

```sql
-- 场景：你是一家公司的 DBA，需要管理三个角色

-- ① 开发人员 dev_user：需要本地登录，对开发库有全部权限
-- ② 后端服务 app_user：需要远程连接，对业务库有增删改查权限，没有改表结构的权限
-- ③ 数据分析师 anayst_user：需要远程连接，对业务库只有查询权限

-- 请写出完整的创建和授权脚本
```

---

> 💡 **提示**：生产环境中，DCL 操作通常由 **root 用户**或具有 `GRANT OPTION` 权限的管理员执行。永远不要给应用账号分配超出需要的权限——遵循**最小权限原则**。
