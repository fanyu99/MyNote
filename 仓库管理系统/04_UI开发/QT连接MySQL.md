#相关: [[C++_Qt_MySQL_仓库管理系统_学习计划#Step 2 启动 MySQL 并创建数据库]]
[[C++_Qt_MySQL_仓库管理系统_学习计划#2.1 数据库连接池设计 (Day 4, ~4h) ⭐]]
[[数据库连接池]]
[[2026-07-19 Day3 Worker初始化与ODBC连接]]
## 相关API
### 头文件
```C++
<QSqlDatabase> // 数据库连接
<QSqlQuery> // SQL语句
<QSqlError> // 错误信息
<QSqlRecord> // 一行记录
<QSqlResult> // 底层结果集
```
### 代码示例
```C++

// 连接数据库
bool openDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(
        "QMYSQL",          // 驱动名
        "MyConnection"     // 连接名（强烈建议写）
    );
    db.setHostName("127.0.0.1"); // 服务器IP地址
    db.setPort(3306); // 数据库端口号
    db.setDatabaseName("testdb"); // 数据库名
    db.setUserName("root"); // 用户名
    db.setPassword("123456"); // 用户密码

    if (!db.open()) {
        qDebug() << "DB Error:" << db.lastError().text();
        return false;
    }
    qDebug() << "MySQL connected!";
    return true;
}

// 查询
//...
QSqlQuery query(QSqlDatabase::database("MyConnection")); 
// 创建查询语句
query.prepare("SELECT id, name FROM user WHERE age > :age");
query.bindValue(":age", 18);
if (!query.exec()) { // 打印错误
    qDebug() << query.lastError().text();
    return;
}
// 获取查询结果
while (query.next()) {
    int id = query.value(0).toInt();
    QString name = query.value(1).toString();
    qDebug() << id << name;
}

// 事务
QSqlDatabase db = QSqlDatabase::database("MyConnection");
db.transaction(); // 开启事务
QSqlQuery query(db);
query.exec("UPDATE account SET balance = balance - 100 WHERE id = 1");
query.exec("UPDATE account SET balance = balance + 100 WHERE id = 2");
if (query.lastError().type() == QSqlError::NoError)
    db.commit(); // 提交事务
else {
    db.rollback(); // 回滚事务
    qDebug() << query.lastError().text();
}

// 关闭连接和清理
QSqlDatabase::database("MyConnection").close();
QSqlDatabase::removeDatabase("MyConnection");
```
