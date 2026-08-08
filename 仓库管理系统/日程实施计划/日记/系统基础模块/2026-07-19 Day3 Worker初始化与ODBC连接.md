[[C++_Qt_MySQL_仓库管理系统_优化版详细实施计划]]
Worker 初始化、ODBC 配置、健康检查、连接关闭

**目标：** 让 `DatabaseWorker` 完整拥有数据库连接，并确保连接只在 Worker 线程内创建、打开、查询和关闭。

**知识点：**

- Qt SQL 连接的线程归属。
- `QSqlDatabase::addDatabase(driver, connectionName)` 的使用方式。
- 唯一连接名与 `QSqlDatabase::removeDatabase`。
- ODBC 连接字符串的构造。
- 登录超时与基础连接诊断。
- 使用 `SELECT 1` 做健康检查。
- 定时器、查询对象、数据库连接、Worker 线程的安全关闭顺序。

**实施任务：**

- 确认 `DatabaseWorker` 只在 Worker 线程中创建、打开、查询和关闭 `QSqlDatabase`。
- 校验 ODBC 配置失败路径。
- 增加健康检查逻辑。
- shutdown 时关闭并移除具名数据库连接。
- 增加连接成功和连接失败测试。

**验收标准：**

- UI/main 线程不创建、不使用 `QSqlDatabase`。
- Worker 初始化时能发出成功信号或结构化失败信号。
- 连接失败能返回有用的 `DatabaseError`。
- Worker 关闭后，数据库连接已关闭并从 Qt 连接池中移除。