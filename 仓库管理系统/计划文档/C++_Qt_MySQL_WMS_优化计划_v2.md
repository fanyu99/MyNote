# C++ / Qt / MySQL 仓库管理系统优化计划 v2

> 制定日期：2026-07-17
>
> 项目目录：`D:\Desktop\仓库管理系统\WMS`
>
> 原计划：`D:\笔记\仓库管理系统\C++_Qt_MySQL_仓库管理系统_学习计划.md`
>
> 总体策略：先完成 1-2 个月可演示的 MVP，再进入求职增强阶段。

## 1. 计划目标

本项目不以“页面数量最多”为目标，而以以下四项可验证成果为目标：

1. 完成登录、物资、入库、出库、库存、审计和导出的完整业务闭环。
2. 展示 Qt 桌面开发的核心能力：对象模型、信号槽、线程、Model/View、数据库、网络或串口、部署。
3. 展示现代 C++ 工程能力：RAII、强类型、分层、CMake、自动化测试、静态分析和 CI。
4. 最终得到招聘者可以直接构建、运行、测试和观看演示的 GitHub 项目。

计划分为三层：

| 层级 | 周期 | 目标 |
|---|---:|---|
| 业务 MVP | 8 周目标 + 2 周缓冲 | 完整、稳定、可演示、具备最小自动化测试 |
| 求职增强 | 3-5 周 | 增加工程深度、Qt 技术覆盖和量化指标 |
| 可选进阶 | 不限 | C/S、多仓、多批次、嵌入式扫码等方向扩展 |

## 2. 当前项目基线

截至 2026-07-17，仓库并不是“从零开始”，已经具备数据库基础设施雏形：

- 数据库 core 已使用 Qt 6、局部 CMake 和 C++17；`WMS/CMakeLists.txt` 与 `WMS/src/CMakeLists.txt` 仍为空，仓库目前不能从顶层完整配置和构建。
- 已有 `DatabaseExecutor`、`DatabaseWorker`、`DatabaseTypes`。
- 数据库连接创建和 SQL 执行位于专用 `QThread`，UI 线程通过队列异步提交任务。
- 已有任务队列容量、事务任务、Prepared Statement、结果行数限制、健康检查和关闭排空。
- 旧的 `ConnectionPool` 和 `ScopedConnection` 仍保留在源码中，但当前架构已经转向单数据库工作线程。
- README 为空，SQL 目录当前没有可跟踪的 Schema 文件，尚无自动化测试、业务层和 UI。
- 原计划写的是 `D:\Desktop\WMS`，实际仓库是 `D:\Desktop\仓库管理系统\WMS`；后续命令、配置和 README 统一使用实际路径。

### 2.1 必须先统一的架构决策

原计划以“连接池 + 同步 DAO + UI/Service 获取连接”为主，现有代码以“单数据库工作线程 + 异步任务”为主，两者不能同时作为正式架构。

新版计划选择现有异步执行器作为唯一主线：

```text
UI / Page
    |
    v
Application Service（业务规则、权限、用例编排）
    |
    v
Repository Interface（面向领域对象的异步接口）
    |
    v
MySql Repository（SQL、参数绑定、结果映射）
    |
    v
DatabaseExecutor -> DatabaseWorker -> QSqlDatabase
                                      （专用线程）
```

这样做的原因：

- Qt SQL 连接具有线程归属，连接和查询都留在数据库线程更容易保证正确性。
- UI 不因查询、事务或网络抖动阻塞。
- 桌面单用户程序通常不需要连接池；串行数据库执行器更贴合真实负载。
- Repository 接口可以在求职增强阶段替换为 HTTP Repository，而不用重写 UI 和业务层。

旧连接池可以作为 `experiments/connection_pool` 学习成果保留，也可以在数据库执行器稳定后删除，但不要让生产代码同时依赖两套数据访问方式。

## 3. GitHub 项目调研

以下星标数为 2026-07-17 调研时的快照。真正同类的 Qt WMS 项目普遍是课程项目，工程质量有限，因此应采用“同类项目学业务 + 成熟 Qt 项目学工程”的组合方式。

### 3.1 同类业务参考

| 项目 | 可借鉴内容 | 不应照搬的地方 |
|---|---|---|
| [liolok/Inventory-Management](https://github.com/liolok/Inventory-Management)（21 stars） | 入库、出库、流水筛选、用户登录构成最小闭环；README 有数据字典和使用截图 | Qt 5/SQLite，数据模型简单；README 明确存在“同一秒多次操作丢日志”，说明时间戳不能作为流水主键 |
| [seigtm/Qt-WMS](https://github.com/seigtm/Qt-WMS)（5 stars，已归档） | 供应、发货、单据生成与文件保存，可参考业务页面组织 | 课程项目、SQLite、工程结构和安全方案较旧，只适合作为功能灵感 |
| [yanpeigong/imx6-warehouse-management-system](https://github.com/yanpeigong/imx6-warehouse-management-system)（2 stars） | Qt + QSerialPort 扫码、触摸端、操作日志、内置 HTTP 服务和 Web 看板，适合嵌入式 Qt 岗位扩展 | Qt 5/qmake/单进程综合实验，不适合作为本项目的基础架构 |
| [a-mo-xi-wei/userPrivilegeManagerSystem](https://github.com/a-mo-xi-wei/userPrivilegeManagerSystem)（5 stars） | Qt 6、C/S、QHttpServer、JSON、Model/View、Delegate、QThread、QXlsx、windeployqt | 仓库提交了大量构建产物，自动化测试与 CI 证据不足；只能按模块选择性学习 |

### 3.2 成熟 Qt 工程参考

| 项目 | 重点学习内容 |
|---|---|
| [sqlitebrowser/sqlitebrowser](https://github.com/sqlitebrowser/sqlitebrowser)（24k+ stars） | 表格型数据库 UI、导入导出、SQL 日志、自定义 Model、QTest、Windows/Linux/macOS CI、CodeQL 和安装包 |
| [dail8859/NotepadNext](https://github.com/dail8859/NotepadNext)（14k+ stars） | 多目标 CMake、跨平台构建、Windows/Linux/macOS 打包、发布工作流、翻译资源、应用级目录组织 |
| [QtExcel/QXlsx](https://github.com/QtExcel/QXlsx)（1.4k+ stars） | 正式 XLSX 读写、CMake 接入、示例、CI、`.clang-format` 和 `.clang-tidy` |

### 3.3 调研结论

1. 同类项目常见功能并不难，真正拉开简历差距的是数据一致性、异步架构、测试、部署和可复现性。
2. 不要以星标数判断课程项目的代码质量，要检查测试、CI、构建说明、Issue 和提交内容。
3. 本项目最有价值的差异点应是“库存流水可追溯 + 异步数据库线程 + Model/View 测试 + 一键打包”，而不是手写一个桌面场景不需要的连接池。

## 4. 原计划需要纠正的内容

### 4.1 连接池不再是主线

原计划多次要求 DAO 获取 `QSqlDatabase&` 或 `ScopedConnection`，这与当前数据库线程架构冲突。修改为：

- SQL 只能在 `DatabaseWorker` 所在线程执行。
- Repository 负责生成 `DatabaseTask`、提交任务、解析结果。
- Service 只看领域对象和应用错误，不看 SQL、连接名或 `QVariantList`。
- UI 只调用 Service，并通过信号接收加载中、成功、空数据和失败状态。

### 4.2 密码不能只做一次 SHA-256

一次 SHA-256 即使加盐也过快，不适合密码存储。MVP 使用 Qt 自带的 `QPasswordDigestor::deriveKeyPbkdf2`，采用 PBKDF2-HMAC-SHA256、随机盐和可配置迭代次数。OWASP 当前给出的 PBKDF2-HMAC-SHA256参考工作因子为 600,000 次；实际值需在目标机器上基准测试，使单次登录可接受。

数据库至少保存：

- `password_hash`
- `password_salt`
- `password_algorithm`
- `password_iterations`

求职增强阶段可通过成熟库升级 Argon2id。不要自研密码算法，不要把数据库密码和 pepper 提交到 Git。

参考：[Qt QPasswordDigestor](https://doc.qt.io/qt-6/qpassworddigestor.html)、[OWASP Password Storage Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html)。

### 4.3 Qt Charts 已不适合新项目作为首选

Qt 6.11 官方文档说明 Qt Charts 自 Qt 6.10 起已弃用，新项目推荐 Qt Graphs。

本项目建议：

- Widgets 继续承担表格、表单、导航等高密度业务 UI。
- 仪表盘用一个小型 Qt Quick/QML 组件承载 Qt Graphs，再通过 `QQuickWidget` 嵌入 Widgets。
- 如果混合栈影响 MVP 进度，先完成统计卡片和表格，将图表放到求职增强阶段。

参考：[Qt Charts](https://doc.qt.io/qt-6/qtcharts-index.html)、[Qt Graphs](https://doc.qt.io/qt-6/qtgraphs-index.html)。

### 4.4 自动化测试不能留到最后一天

原计划在 Day 26 才集中测试，届时架构错误的返工成本已经很高。改为每个阶段都有对应测试：

- 数据库执行器完成时测试状态机、队列、事务和关闭。
- Repository 完成时测试 SQL 映射和错误传播。
- Service 完成时测试业务规则和权限。
- Model 完成时立即接入 `QAbstractItemModelTester`。
- UI 完成时用 QTest 做关键流程冒烟测试。

### 4.5 “Excel 导出为 HTML `.xls`”应删除

HTML 表格伪装成 `.xls` 会产生格式警告，也不能证明真正的 Excel 文件能力。保留标准 CSV，正式 XLSX 使用 QXlsx；MVP 时间不足时只交付 CSV，求职增强阶段再接 QXlsx。

### 4.6 “所有表都软删除”应收缩

- 物资、分类、用户等主数据可软删除或停用。
- 已确认的入库单、出库单、库存流水和审计日志不应删除，只能冲销或作废并保留原因。
- 草稿单可以删除；已确认单必须通过反向流水纠正。

这比统一添加 `deleted_at` 更符合审计和库存可追溯性。

### 4.7 不要为了现代 C++ 标签强行使用 C++23

当前 CMake 已使用 C++17。MVP 保持 C++17，真正使用到 `std::jthread`、`std::span`、Concepts 等能力时再升级 C++20。简历上“正确使用 RAII、值语义、移动语义、`std::variant`、依赖注入”比只写 C++23 更可信。

## 5. 新版系统架构

### 5.1 模块边界

```text
WmsApp
├── presentation
│   ├── widgets       页面、对话框、导航
│   ├── models        QAbstractTableModel
│   └── delegates     状态、按钮、编辑器 Delegate
├── application
│   ├── services      用例、权限、事务编排
│   ├── dto           页面需要的数据
│   ├── ports         Repository 抽象接口和异步请求句柄
│   └── errors        可展示的应用错误
├── domain
│   ├── entities      Product、Order、StockMovement 等
│   ├── value_objects OrderNo、Quantity、Money 等
│   └── rules         不依赖数据库的领域规则
├── infrastructure
│   ├── database      现有 DatabaseExecutor/Worker
│   ├── repositories  MySQL 实现、SQL 映射
│   ├── config        QSettings、环境覆盖
│   └── export        CSV、QXlsx
└── shared
    ├── logging
    └── result
```

不需要一开始创建大量空类。每完成一个纵向业务切片，再补齐该切片需要的实体、Repository、Service、Model 和页面。

### 5.2 异步数据流

以“查询物资列表”为例：

```text
用户点击查询
  -> 页面进入 Loading，禁用重复提交
  -> ProductService::loadPage(filter, page)
  -> ProductRepository 提交 DatabaseTask
  -> DatabaseExecutor 返回 requestId
  -> DatabaseWorker 执行 Prepared Statement
  -> taskFinished(DatabaseResult)
  -> Repository 校验列并映射 QVector<ProductSummary>
  -> Service 发出成功或 AppError
  -> Model 使用 beginResetModel/endResetModel 更新
  -> 页面恢复 Ready、Empty 或 Error 状态
```

必须补充：

- 请求超时和取消语义。
- 页面销毁后的结果丢弃，避免回调悬空。
- 同一搜索框连续查询时只接受最新 requestId。
- 队列满、连接失败、SQL 失败和业务校验失败使用不同错误码。
- 日志记录 requestId、耗时、错误码，但不记录密码和完整敏感参数。

### 5.3 最小异步 Repository 契约

为了让新手能够落地，建议每次异步调用返回一个有 QObject 所有权的请求对象，而不是让所有页面监听全局完成信号：

```cpp
class ProductPageRequest : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
signals:
    void succeeded(const ProductPage& page);
    void failed(const AppError& error);
};

class IProductRepository {
public:
    virtual ~IProductRepository() = default;
    virtual ProductPageRequest* fetchPage(const ProductFilter& filter,
                                          QObject* owner) = 0;
};
```

所有权和职责：

- `owner` 通常是 Service，请求对象随 owner 销毁；Repository 内部用 `QPointer` 防止向已销毁对象回调。
- 请求对象保存 requestId，只消费对应数据库结果。
- 页面只监听 Service 的用例信号，不直接认识 Repository 或 DatabaseExecutor。
- 一笔业务事务由具体 Repository 生成一个事务任务，Service 不拼 SQL。
- `cancel()` 只保证取消尚未派发的任务或丢弃结果，不承诺强行中断正在执行的 `QSqlQuery::exec()`。

如果后续发现每个用例都创建请求类过于重复，可以再抽象通用 Operation；MVP 不要先做复杂模板框架。

### 5.4 依赖规则

- `domain` 不依赖 Qt Widgets、Qt SQL 或 MySQL。
- `application` 不依赖具体 Repository 实现；ports 不暴露 SQL、`DatabaseTask` 或原始 `QVariantList`。
- `presentation` 不直接包含 `QSqlQuery` 或 SQL 字符串。
- `infrastructure` 可以依赖 Qt SQL、ODBC 和第三方导出库。
- 依赖通过构造函数注入，在 `main.cpp` 统一组装。

Repository 的异步接口可使用 Qt Core 信号和请求句柄，但业务返回值必须是有类型的 DTO 或 `AppError`。Service 负责把 Repository 的完成信号转换为页面用例信号；不要让页面直接订阅全局 `DatabaseExecutor::taskFinished`。

### 5.5 条件事务契约

当前 `DatabaseTask` 只是静态 SQL 列表。SQL 执行成功不代表业务前置条件成立，例如 `UPDATE ... WHERE status='draft'` 影响 0 行时，Worker 仍会继续执行后面的库存更新。

MVP 必须扩展 `DatabaseStatement`，支持“预期影响行数”守卫：

```cpp
struct AffectedRowsExpectation {
    qint64 minimum;
    qint64 maximum;
    DatabaseErrorCode failureCode;
};

struct DatabaseStatement {
    StatementType type;
    QString sql;
    QVariantMap parameters;
    std::optional<AffectedRowsExpectation> affectedRows;
};
```

Worker 每执行一条语句都检查 `numRowsAffected()`；不满足期望时立即回滚整个任务，并返回明确业务错误。入库/出库确认至少要求：

- 单据从 draft 更新为 confirmed 必须恰好影响 1 行。
- 出库使用 `UPDATE stock_balance SET quantity=quantity-:qty WHERE ... AND quantity>=:qty`，每个库存维度必须恰好影响 1 行。
- 流水插入违反幂等唯一键时整笔回滚，并映射为“单据已处理”而不是通用 SQL 错误。

如果将来出现必须根据查询结果动态分支的复杂事务，再评估数据库存储过程或运行在数据库线程的专用 transaction command。不要把仓储领域逻辑硬编码进通用 Worker。

### 5.6 超时与取消的边界

MVP 明确区分四种行为：

- **排队取消**：任务尚未派发时从队列移除，并返回 Cancelled。
- **latest-wins**：旧搜索可以继续执行，但结果到达后被丢弃，不覆盖新搜索。
- **软超时**：页面到时恢复可操作并提示超时，底层查询可能仍在 Worker 中完成。
- **执行超时**：只能依赖 ODBC/MySQL 支持的登录、锁等待或语句超时选项；不能通过强杀 QThread 安全中断 `QSqlQuery::exec()`。

验收时分别测试这四种语义，不把“页面不再等待”误写成“数据库查询已经取消”。

## 6. 功能优先级

### 6.1 MVP：必须完成

| 模块 | 功能 | 验收标准 |
|---|---|---|
| 启动与配置 | QSettings、环境变量覆盖、连接诊断 | 配置缺失或连接失败时给出可操作错误，不崩溃 |
| 认证授权 | 登录、登出、角色权限、PBKDF2 | UI 隐藏按钮之外，Service 再次校验权限 |
| 主数据 | 物资、分类、单位 CRUD，停用 | 编码唯一；被业务单据引用的数据不能物理删除 |
| 入库 | 草稿、明细、确认 | 重复确认不会重复加库存；失败整体回滚 |
| 出库 | 草稿、库存校验、确认 | 并发或重复操作不能产生负库存 |
| 库存 | 当前余额、流水查询、分页筛选 | 任一余额可追溯到对应流水 |
| 审计 | 登录、主数据变更、单据确认、导出 | 审计记录不可由普通用户修改或删除 |
| 导出 | 标准 CSV | 中文和 RFC 4180 转义正确；防止 Excel 公式注入 |
| UI | Loading/Empty/Error/Ready 四态 | 慢查询期间窗口仍可拖动和响应 |
| 最小质量 | QTest、QSignalSpy、ModelTester、CTest | 核心事务和 Model 测试可从顶层 CMake 重复运行 |

### 6.2 求职增强：MVP 后完成

按推荐顺序实施：

1. **盘点闭环**：盘点任务、差异原因、审批和调整流水，禁止直接编辑库存余额。
2. **Model/View 深化**：`QSortFilterProxyModel`、自定义 Delegate 和增量更新；`QAbstractItemModelTester` 已属于 MVP。
3. **自动化质量门禁**：在 MVP 测试基础上增加 clang-format、clang-tidy、CodeQL、覆盖率和多平台 GitHub Actions。
4. **性能工程**：10 万条流水测试数据、SQL 索引与 `EXPLAIN`、分页查询、耗时日志、内存与 UI 响应基准。
5. **正式部署**：`qt_generate_deploy_app_script` 或 windeployqt、CPack/NSIS、版本信息、安装与卸载、示例配置。
6. **真正的 XLSX**：QXlsx 导入导出，提供导入预览、字段校验和错误报告。
7. **条码工作流**：先支持键盘模拟扫码枪；若目标是嵌入式岗，再增加 QSerialPort 设备层和可替换模拟器。
8. **Qt Graphs 仪表盘**：以小型 QML 组件嵌入 Widgets，展示 Widgets/QML 混合开发。
9. **日志与诊断包**：`QLoggingCategory`、滚动日志、脱敏、导出诊断信息。

### 6.3 可选进阶：只选一个方向

不要同时实现以下所有项目：

| 方向 | 新增能力 | 更适合的岗位 |
|---|---|---|
| C/S 化 | Qt HTTP Server、QNetworkAccessManager、JSON、认证、重试 | Qt 客户端、桌面网络应用 |
| 嵌入式仓储终端 | QSerialPort、扫码枪、触摸 UI、断线缓存 | 嵌入式 Qt、工控上位机 |
| 多仓与批次 | 仓库/库位、调拨、批次、保质期、FIFO/FEFO | 企业软件、MES/WMS |
| 插件化 | 稳定接口、Qt plugin、动态加载报表或导入器 | 桌面平台、工具链开发 |

推荐顺序是：完成 MVP -> 做质量与部署 -> 加扫码 -> 再根据目标岗位选择 C/S 或嵌入式。

## 7. 数据库设计优化

### 7.1 引入不可变库存流水

原计划只维护 `inventory.quantity`，发生错误时难以解释“库存为什么是这个数字”。新增 `stock_movements`：

```text
stock_movements
- id BIGINT AUTO_INCREMENT PRIMARY KEY
- movement_no VARCHAR(...) UNIQUE
- product_id
- warehouse_id
- location_id NULL
- lot_id NULL
- movement_type  INBOUND / OUTBOUND / ADJUST / REVERSAL / TRANSFER
- quantity_delta 正入负出，禁止为 0
- source_type
- source_id
- source_line_id
- movement_role  NORMAL / TRANSFER_OUT / TRANSFER_IN / REVERSAL
- operator_id
- reason
- created_at
- UNIQUE(source_type, source_id, source_line_id, movement_role)
```

幂等键只使用非空的来源类型、来源单据和来源明细。不要把可空 `lot_id` 放入唯一键来承担幂等约束，因为 MySQL 唯一索引允许多行包含 NULL。

`stock_balance` 保存当前余额以提高查询速度，但它是流水的投影。确认单据时在同一事务内：

1. 用 `WHERE status='draft'` 的状态更新作为第一道门槛，并要求恰好影响 1 行。
2. 入库使用原子 UPSERT 创建或增加余额；出库使用带 `quantity >= :qty` 条件的原子 UPDATE。
3. 每个余额更新都检查受影响行数，库存不足或维度不存在立即回滚。
4. 写入带幂等唯一键的库存流水。
5. 写入审计日志。
6. 提交；任一步失败则回滚。

### 7.2 防止重复确认

确认语句必须带状态条件：

```sql
UPDATE inbound_orders
SET status = 'confirmed', confirmed_at = CURRENT_TIMESTAMP
WHERE id = :id AND status = 'draft';
```

只有受影响行数为 1 才继续写库存。配合流水唯一约束，即使用户双击、超时重试或消息重复，也不会重复加减库存。

### 7.3 Schema 具体调整

- 增加 `warehouses`，MVP 可以只预置一个默认仓库。
- `stock_balance` 使用全部非空的库存维度组合唯一键；MVP 不做批次时键为 `(product_id, warehouse_id)`。批次版再升级为 `(product_id, warehouse_id, location_id, lot_id)`，并为“无批次”建立明确的非空规范表示。
- `supplier` 和 `recipient` 初期可保留文本；需要统计时再拆 `suppliers`、`departments`。
- 批次必须同时出现在入库、库存和出库链路，不能只在入库明细保存 `batch_no`。
- `inventory_alerts` 若只用于展示可改为查询结果；若要处理闭环，则增加唯一业务键、状态和处理人。
- 给常用组合筛选建立复合索引，而不是给每一列单独建索引。
- 技术主键使用 MySQL `AUTO_INCREMENT`；业务单号使用 UUID/ULID，或使用带事务锁的编号表生成，不用 `COUNT(*) + 1`。MySQL 8 没有 PostgreSQL/Oracle 风格的通用 sequence。
- 金额继续使用 `DECIMAL`，C++ 层不要用二进制浮点累计财务金额。
- 已确认单据不使用级联删除。
- Schema 采用版本化迁移文件，例如 `001_initial.sql`、`002_stock_movements.sql`。

## 8. 8 周业务 MVP 路线图

每天按 2-4 小时学习和实现估算，并额外预留最多 2 周缓冲。每周都要有可运行增量，不按“先写完所有 DAO，再写所有 UI”的横向方式推进。若延期，先移出盘点、图表、XLSX、扫码和高级 Delegate，不能牺牲事务正确性和测试。

### 第 1 周：统一数据库基础设施

- 固化 `DatabaseExecutor/Worker` 状态机和线程约束。
- 增加“预期影响行数”事务守卫，并补充排队取消、软超时、latest-wins 和错误分类。
- 删除正式构建对旧连接池的依赖，旧代码移入实验区或暂时保留但不链接。
- 建立 `tests/`、CTest 和首批 QTest。
- 测试初始化成功/失败、单语句、事务回滚、队列满、关闭排空。

交付：数据库执行器有自动化测试，UI 线程不会执行 SQL。

### 第 2 周：Schema、认证和物资纵向切片

- 编写版本化 Schema 和种子数据。
- 实现 PBKDF2、Session、权限枚举和应用错误。
- 完成 Product 的 Entity -> Repository -> Service -> Model -> 页面闭环。
- 增加分页、搜索、唯一编码和停用。

交付：可登录并完成物资 CRUD；测试覆盖密码、权限、Repository 映射和 Model。

### 第 3 周：库存领域与入库

- 实现订单、明细、库存流水和余额。
- 完成入库草稿、编辑、确认、重复确认保护。
- 增加事务失败注入测试，验证整体回滚。

交付：入库后余额和流水一致，可追溯操作人和来源单据。

### 第 4 周：出库与异常修正

- 实现出库库存校验、条件更新、负库存保护。
- 已确认单据通过冲销流水修正，不直接修改历史流水。
- 使用两个独立数据库连接测试库存不足、双击确认、重复请求、首次创建余额和并发出库。

交付：核心进销存闭环完成。

### 第 5 周：Model/View 和业务 UI

- 完成通用分页控件，但避免复杂的模板基类。
- 增加基础 Delegate、校验器和四态页面；筛选和排序由服务端分页查询负责。
- 使用 `QAbstractItemModelTester` 检查 Model 协议。
- 完成角色导航和键盘操作。

交付：物资、入库、出库、库存页面可稳定使用。

### 第 6 周：审计、预警、导出和看板

- 完成不可修改的审计查询。
- 低库存预警先采用实时查询，再决定是否持久化处理状态。
- 完成 CSV 导出。
- 完成统计卡片；有余力再加入 Qt Graphs。

交付：业务功能齐全，能生成演示数据和报表。

### 第 7 周：质量和性能

- 建立至少 1 万条流水的可重复测试数据生成器；10 万条压力测试放入求职增强阶段。
- 对分页、搜索、统计查询使用 `EXPLAIN` 并优化复合索引。
- 使用 QElapsedTimer 记录测试机器、数据规模、预热方式和重复次数，不凭主观感觉判断“不卡”。
- 开启严格编译警告；clang-tidy、Sanitizer 和覆盖率可在求职增强阶段补齐。
- 完成单元、集成和关键 UI 冒烟测试。

交付：形成可复现的性能报告和测试报告。

### 第 8 周：发布和作品集

- 使用 CMake Deploy API/windeployqt 收集 Qt 插件和运行库。
- 先生成 Windows ZIP，在未安装 Qt SDK 的干净环境验证；NSIS 安装器放入求职增强阶段。
- 完成 README、架构图、ER 图、截图、2 分钟演示视频或 GIF。
- 至少建立一个可稳定运行的 CI 构建；多平台、CodeQL 和自动 Release 放入求职增强阶段。
- 提供 Docker Compose MySQL、迁移/种子命令和 ODBC 驱动说明，记录从零初始化所需时间。
- 建立 `v1.0.0` Release，并附安装包和变更说明。

交付：招聘者可从 Release 下载应用，并按 README 在目标 15 分钟内完成 MySQL、ODBC、迁移和示例账号初始化。若未提供 SQLite 演示适配器，不宣称“下载即可使用”。

## 9. 测试与质量门禁

### 9.1 测试金字塔

| 层级 | 工具 | 重点 |
|---|---|---|
| 单元测试 | QTest | 值对象、密码、权限、Service 规则 |
| Qt 对象测试 | QSignalSpy | 异步信号、状态变化、超时、取消 |
| Model 测试 | QAbstractItemModelTester | 索引、行列变化、通知协议 |
| 数据库集成测试 | QTest + 独立测试库 | Prepared Statement、事务、锁、Schema 迁移 |
| UI 冒烟 | QTest | 登录、查询、创建单据、错误提示 |

参考：[QAbstractItemModelTester](https://doc.qt.io/qt-6/qabstractitemmodeltester.html)。

### 9.2 合并前门禁

- CMake configure/build 成功。
- 所有 CTest 通过。
- 编译器警告不新增。
- 密钥、密码、数据库文件、构建产物未进入 Git。

求职增强阶段再把格式检查、clang-tidy、CodeQL、多平台 CI 和覆盖率加入强制门禁。

不要把“覆盖率 80%”当作唯一目标。核心事务、错误路径、状态机和 Model 协议的覆盖比简单 getter 的行覆盖率更有价值。

### 9.3 必须写成测试的契约

- **库存不变量**：按产品和仓库聚合后，余额始终等于全部有效流水增量之和。
- **并发**：使用两个独立连接测试首次 UPSERT 余额、同时出库和锁等待超时，并记录 MySQL 隔离级别。
- **迁移**：空库初始化、从上一版本升级、迁移重复执行和失败回滚都有确定结果；测试库与生产库配置必须隔离。
- **密码**：随机盐至少 16 字节，使用 CSPRNG；记录算法、迭代数和派生长度；PBKDF2 最低迭代数和登录耗时上限可配置，并支持参数升级后登录重哈希。构建链接 `Qt6::Network`。
- **CSV**：覆盖 UTF-8 BOM、逗号、双引号、换行和空值；以 `= + - @` 开头的单元格按产品策略转义，避免 Excel 公式注入；大导出采用逐行写入而不是一次驻留内存。
- **服务端分页**：搜索、筛选和排序都作用于完整结果集；ProxyModel 若存在，只处理当前页的展示，不宣称全量筛选。
- **性能**：固定数据生成种子、测试机器、数据库版本、冷/热缓存、预热次数和统计口径，保存优化前后的 SQL 与执行计划。
- **UI 响应**：在指定测试机器和数据集上设置事件循环心跳或交互响应阈值，不只用“窗口还能拖动”作证据。
- **审计**：应用层拒绝普通用户修改；若声称“数据库层不可变”，还必须用最小权限数据库账号和权限测试证明。

## 10. CMake、CI 与部署

### 10.1 CMake

- 每层建立独立 target，并通过 `target_link_libraries` 表达依赖。
- 使用 `target_compile_features(... cxx_std_17)`，不要同时设置多个互相冲突的标准。
- 增加 `BUILD_TESTING`、`WMS_ENABLE_SANITIZERS` 等选项。
- 增加 `CMakePresets.json`：Windows Debug、Windows Release、Linux CI。
- 第三方库通过固定版本或 Git submodule/FetchContent 管理，并记录许可证。

### 10.2 CI

建议工作流：

1. `build-windows.yml`：MSVC 或 MinGW 构建、QTest、部署产物。
2. `test-linux.yml`：GCC/Clang、CTest、ASan/UBSan、覆盖率。
3. `codeql.yml`：C++ 安全扫描。
4. `release.yml`：Tag 触发打包并上传 Release。

### 10.3 部署

Qt 6.11 可使用 `qt_generate_deploy_app_script` 生成部署脚本。Windows 发布包必须验证：

- `platforms/qwindows.dll`
- SQL/ODBC 插件及系统 ODBC 驱动前置条件
- Qt Quick/QML 依赖（若使用 Qt Graphs）
- 配置模板、Docker Compose、迁移和数据库初始化说明
- VC Runtime 或 MinGW Runtime

桌面客户端直连 MySQL 时，Service 权限不是不可绕过的安全边界。应用数据库账号必须遵循最小权限，README 要明确本项目的信任模型；需要真正隔离数据库凭据和强制服务端授权时，应进入 C/S 版本。

参考：[qt_generate_deploy_app_script](https://doc.qt.io/qt-6/qt-generate-deploy-app-script.html)。

## 11. 更适配 C++/Qt 岗位的亮点

### 11.1 优先级排序

| 优先级 | 亮点 | 为什么有价值 | 如何证明 |
|---:|---|---|---|
| 1 | Qt 线程归属与异步数据库执行器 | 能讲清 QObject、QThread、QueuedConnection、生命周期 | 状态机图、QSignalSpy 测试、UI 响应演示 |
| 2 | QAbstractTableModel + Delegate | Qt Widgets 岗位高频核心能力 | 自定义 Model、ProxyModel、ModelTester 测试 |
| 3 | 事务与库存幂等 | 体现真实业务和数据库一致性 | 重复确认、失败回滚、负库存测试 |
| 4 | CMake + CI + 部署 | 证明项目不是只能在本人电脑运行 | Actions 徽章、Release 安装包、干净机验证 |
| 5 | 现代 C++ 边界设计 | 比堆砌语法更能体现工程能力 | RAII、强类型、`std::variant` 错误、构造注入 |
| 6 | 性能分析 | 能将“快”转化为数据 | 10 万行数据、P95 延迟、索引前后对比 |
| 7 | QML/Qt Graphs 或 QSerialPort | 增加 Qt 技术覆盖面 | 混合界面或真实/模拟扫码演示 |

### 11.2 不要作为主亮点的内容

- 桌面单用户场景下的数据库连接池。
- “使用了单例模式”本身。
- 只有界面截图、没有测试和可运行 Release。
- 仅写“使用 C++23”，但代码中没有对应设计收益。
- QSS 深色主题。它是完成度加分项，不是核心技术壁垒。
- SHA-256 加盐。它比明文好，但不是合格的现代密码存储方案。

## 12. 简历表达模板

以下内容只能在功能和指标实际完成后写入简历，方括号必须替换为真实测量结果：

> **仓库管理系统 WMS** | C++17、Qt 6.11、MySQL 8、CMake
>
> 设计并实现基于 Qt Widgets 的仓储桌面应用，覆盖物资、入库、出库、库存流水、权限和审计闭环。

- 设计专用数据库工作线程和有界异步任务队列，通过 Queued Connection 隔离 UI 与 Qt SQL 线程归属，并使用 QSignalSpy 验证初始化、超时和关闭信号序列。
- 基于 Repository/Application Service 分层组织业务，使用事务、条件状态更新和库存流水唯一约束；经双连接集成测试验证重复确认不重复记账、并发出库不产生负库存。
- 实现 `QAbstractTableModel` 和自定义 Delegate，使用 `QAbstractItemModelTester` 验证 Model 协议，通过 SQL 实现全量数据范围的服务端分页、排序和多条件筛选。
- 建立 CMake/CTest/GitHub Actions 工程链路，在 Windows 与 Linux 完成构建测试，并产出可直接安装的 Windows Release。
- 在 `[CPU/内存/MySQL版本]` 环境下，针对 `[真实数据量]` 条库存流水进行索引与查询优化，将 `[查询名称]` 的 P95 延迟由 `[A] ms` 降至 `[B] ms`，测量脚本和执行计划随仓库提供。

面试介绍顺序：业务问题 -> 架构决策 -> 最难的一致性问题 -> Qt 特有问题 -> 测试和量化结果 -> 仍可改进之处。

## 13. 主要挑战与解决策略

| 挑战 | 风险 | 解决策略 | 可形成的面试点 |
|---|---|---|---|
| QObject/QThread 生命周期 | 跨线程删除、死锁、退出崩溃 | 明确 owner thread；异步 shutdown；测试所有状态转换 | 为什么 Worker Object 优于在 UI 线程直接查询 |
| 异步结果对应页面 | 旧结果覆盖新搜索、页面销毁后回调 | requestId、latest-wins、QPointer、取消令牌 | 如何处理竞态和悬空回调 |
| 事务与重复提交 | 双击确认导致库存重复变化 | 状态条件更新、唯一约束、同事务流水和余额 | 幂等与数据库约束如何互补 |
| 负库存并发 | 校验后库存被其他操作修改 | 行锁或带条件的原子 UPDATE，并检查 affectedRows | 悲观锁与乐观并发的取舍 |
| 库存可追溯 | 余额正确但原因不可解释 | 不可变流水 + 余额投影 + 冲销 | 审计、事件记录与快照 |
| Model/View 正确性 | 数据已变但视图不刷新，索引越界 | 精确 begin/end 通知和 ModelTester | reset 与增量更新的取舍 |
| 大数据量 | 一次加载全部数据导致卡顿和内存膨胀 | 服务端分页、索引、延迟搜索、结果上限 | EXPLAIN、P95 和 UI 响应性 |
| ODBC 部署 | 开发机能运行，目标机缺驱动 | 启动诊断、安装说明、干净机测试 | Qt 插件与系统驱动的部署边界 |
| 密码与配置 | 快速哈希、凭据泄漏到 Git/日志 | PBKDF2/Argon2id、随机盐、环境覆盖、脱敏 | 威胁模型和安全权衡 |
| 范围失控 | 页面很多但核心流程不稳定 | 纵向切片、周交付、P0/P1/P2 门禁 | 如何管理个人项目需求 |

## 14. 完成定义

业务 MVP 只有同时满足以下条件才算完成：

- 登录 -> 物资 -> 入库 -> 出库 -> 库存流水 -> 审计 -> 导出流程可演示。
- SQL 不在 UI 线程执行，慢查询时窗口保持响应。
- 重复确认、库存不足、事务失败和断线都有自动化测试；并发测试使用两个独立数据库连接而不是同一串行 Executor。
- 自动对账测试验证每个库存维度都满足 `stock_balance.quantity = SUM(stock_movements.quantity_delta)`。
- Repository、Service、Model 和 UI 的依赖方向符合架构约束。
- 测试从干净构建目录可重复运行。
- Windows Release 在未安装 Qt SDK 的环境可启动，并在目标 15 分钟内按文档完成 MySQL、ODBC、迁移和种子初始化。
- README 包含构建、数据库初始化、测试、截图、架构和已知限制。
- 简历中的每个数字都能由脚本、测试输出或性能报告复现。

## 15. 下一步

立即执行顺序：

1. 完成并测试当前 `DatabaseExecutor/DatabaseWorker`，不要继续扩展旧连接池。
2. 建立版本化 Schema，优先加入库存流水、余额和幂等约束。
3. 用“登录 + 物资列表”完成第一个纵向切片，验证异步数据流。
4. 再进入入库、出库和库存流水；盘点放到求职增强阶段，不提前批量创建全部 DAO 或页面空壳。
5. 每周结束时必须有可运行演示、自动化测试和一条清晰的 Git 提交记录。

## 16. 优化点与挑战总览

| 优化方向 | 最终优化点 | 预期收益 | 主要挑战 |
|---|---|---|---|
| 数据库线程 | 单 Worker + 有界队列 + 明确状态机 | UI 不被 SQL 阻塞，Qt SQL 线程归属清晰 | QObject 生命周期、关闭排空、执行中查询不可强杀 |
| 数据访问 | 异步 Repository Operation，隐藏 SQL 和 QVariant 行 | 页面、业务和数据库解耦，可为 C/S 版本复用 | requestId 关联、latest-wins、所有权和错误映射 |
| 条件事务 | 每条语句支持 affectedRows 断言 | 重复确认或库存不足时自动回滚 | 当前静态任务模型需要扩展；错误必须区分业务失败和 SQL 失败 |
| 库存模型 | 不可变流水 + 余额投影 + 自动对账 | 库存来源可追溯，支持冲销和审计 | 幂等键、首次 UPSERT、多连接并发和余额一致性 |
| 安全 | PBKDF2 参数化存储、最小权限账号、配置脱敏 | 消除一次 SHA-256 和硬编码凭据问题 | 登录耗时基准、算法升级、直连 MySQL 的信任边界 |
| Model/View | 自定义 Model、Delegate、服务端分页、ModelTester | 展示 Qt 岗位核心能力并支持大数据量 | begin/end 通知、排序筛选边界、异步页面状态 |
| 测试 | QTest、QSignalSpy、ModelTester、双连接集成测试 | 核心主张有可重复证据 | 测试库隔离、迁移清理、并发测试稳定性 |
| 性能 | 固定数据集、EXPLAIN、P95 和 UI 心跳 | 简历可提供真实量化结果 | 控制缓存、硬件和测试方法，避免不可复现数字 |
| 工程化 | 顶层 CMake、CTest、CI、部署 ZIP、Release | 招聘者能构建、测试和运行 | Qt/ODBC/MySQL 依赖部署和干净机验证 |
| 业务扩展 | MVP 后选择扫码、C/S、多仓批次之一 | 形成与目标岗位匹配的差异点 | 控制范围，不让扩展破坏核心闭环 |

最重要的取舍是：不把连接池、深色主题或页面数量当作主亮点；优先证明异步线程模型、库存一致性、Model/View、测试和部署能力。最大的综合挑战是让“异步 UI、事务正确性和多实例并发”同时成立，并用自动化测试而不是口头描述证明它们。
