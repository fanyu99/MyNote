[[C++_Qt_MySQL_仓库管理系统_优化版详细实施计划]]
### MySQL ProductRepository 与数据库结果映射（当前任务）

  

**目标：** 实现 `IProductRepository` 的 MySQL 版本，让 ProductService 可以在不修改上层接口的情况下使用真实数据库查询和写入物资数据。

  

**知识点：**

  

- Repository Port 与 Infrastructure Adapter 的关系。

- Prepared Statement、命名参数与 SQL 注入防护。

- `DatabaseExecutor` 异步任务到 Repository callback 的结果映射。

- `DatabaseResult`、`QVariantMap/QVariantList` 到 `Product`/DTO 的字段校验。

- 分页查询中的 `LIMIT/OFFSET`、`COUNT(*)` 与排序稳定性。

- Repository 错误到 `AppError` 的分类：数据库失败、唯一键冲突、记录不存在。

  

**实施任务：**

  

- 在 infrastructure 层设计 `MySqlProductRepository` 的位置、CMake target 和依赖方向。

- 实现 `listProducts()`：筛选条件、分页、总数、字段映射和排序。

- 实现 `findByCode()`：用于 ProductService 的唯一编码判断。

- 实现 `createProduct()`、`updateProduct()`、`setProductActive()` 的参数化 SQL。

- 统一处理 owner 生命周期：Repository 内部也使用 `QPointer`，不向已销毁 owner 回调。

- 为真实 Repository 准备集成测试策略，优先覆盖 SQL 映射和错误映射。

  

**验收标准：**

  

- `ProductService` 不需要改接口即可切换到 MySQL Repository。

- 所有 SQL 均使用参数绑定，不拼接用户输入。

- 分页结果包含 `items/total/page/pageSize`，并且排序稳定。

- 数据库列缺失、类型错误、SQL 失败能映射为可理解的 `AppError`。

- 物资编码重复、产品不存在、停用失败有明确错误路径。

- owner 销毁后不会触发上层 callback。