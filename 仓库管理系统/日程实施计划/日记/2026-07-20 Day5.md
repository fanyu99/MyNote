[[C++_Qt_MySQL_仓库管理系统_优化版详细实施计划]]
### 事务任务、affectedRows 守卫、排队取消、软超时、异步 shutdown

**目标：** 为后续真实库存事务做准备，尤其是入库确认、出库扣减和库存一致性。
**知识点：**
- 单 SQL 任务与多语句事务任务。
- 数据库事务的 `begin`、`commit`、`rollback`。
- `affectedRows` 作为数据库层业务守卫的意义。
- 为什么不能强杀正在执行的 `QSqlQuery::exec()`。
- 排队取消、软超时、关闭排空超时的区别。
- 异步 shutdown 与待处理任务清理。

**实施任务：**
- 补齐事务任务执行逻辑。
- 为需要守卫的语句增加 affected-rows 期望值。
- 事务中任意语句失败时执行 rollback。
- shutdown 或排空超时时取消待派发任务。
- 明确定义软超时后调用方能收到什么结果。
- 增加事务回滚、affectedRows 守卫、shutdown drain 测试。

**验收标准：**
- 事务中任意语句失败时，前面已执行语句全部回滚。
- 带守卫的 UPDATE 在 affected row 数不符合预期时会让事务失败。
- 待派发任务可以取消，但不强杀正在执行的 SQL。
- shutdown 行为确定，并能发出完成信号。