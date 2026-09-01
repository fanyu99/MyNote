[[C++_Qt_MySQL_仓库管理系统_优化版详细实施计划]]
# MySqlInboundRepository 基础链路与确认事务设计（当前任务）

  

**计划依据：** 优化版详细计划 C.8“入库/出库条件事务”第 1 项练习，以及 F.2 第 3 周“库存流水与入库”阶段。当前代码已经完成 Fake Repository，但尚未建立真实 `MySqlInboundRepository`，因此先完成 Repository 到 `DatabaseExecutor` 的真实接入和事务边界设计。

  

**目标：** 建立真实入库 Repository 的实现骨架，先完成草稿创建、按 ID/订单号查询、分页查询的数据库结果映射，并明确确认入库事务中订单状态、明细、库存余额、库存流水和审计日志的执行顺序。

  

**知识点：**

  

- Application Service、Repository Port、MySQL Repository 三者的职责边界。

- `DatabaseStatement`、`DatabaseTask` 与 Repository 结果 DTO 的映射。

- 订单 Header/Line 的多表查询与组装方式。

- 为什么 `confirmOrder` 必须由一个 Repository 事务任务完成，而不能由 Service 分步调用多个方法。

- 事务边界：状态更新、库存余额、库存流水和审计日志必须使用同一个 Worker 连接。

- `affectedRows` 守卫与数据库错误、业务状态错误的映射关系。

  

**实施任务：**

  

- 创建 `MySqlInboundRepository`，实现 `IInboundRepository` 的基础接口。

- 先实现 `createDraft()`：生成订单号、插入订单 Header、插入订单明细，并在成功后返回完整 `InboundOrder`。

- 实现 `findById()`、`findByOrderNo()` 和 `listOrders()` 的查询与 DTO/Entity 映射。

- 梳理 `confirmOrder()` 的事务步骤和 SQL 参数，不在 Service 中拼接 SQL。

- 明确确认事务的第一条状态守卫：`WHERE id = :id AND status = 'draft'`，并记录预期影响行数为 1。

- 为真实 Repository 保留 owner 生命周期保护，避免数据库异步完成后回调已销毁对象。

  

**测试任务：**

  

- 使用 Fake 或可控数据库测试验证 Repository 接收到的参数和返回对象映射。

- 增加创建订单号非空、订单明细正确保存、按 ID/订单号查询和分页查询测试。

- 增加确认事务设计测试或最小数据库联调，先验证草稿状态守卫和重复确认不会被当作成功。

  

**验收标准：**

  

- `MySqlInboundRepository` 不把 SQL 暴露给 Service 或 UI。

- 创建、查询和分页接口能够通过真实 `DatabaseExecutor` 执行并映射为领域对象/DTO。

- `confirmOrder()` 的事务步骤、连接归属和 affectedRows 守卫已经明确。

- Repository 异步回调具备 owner 生命周期保护。

- 代码能够编译，新增单元测试或真实链路测试通过。
- 