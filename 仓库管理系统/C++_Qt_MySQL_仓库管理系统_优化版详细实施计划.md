# 优化版详细实施计划

> **本部分定位：** 这是《[优化版计划](C++_Qt_MySQL_仓库管理系统_优化版计划.md)》的展开，仿照原版计划的形式提供环境准备、技术专题、架构、Schema、分步实现、设计决策、陷阱与面试准备的细节。
>
> **改写规则：** 原版计划中在优化方案下仍然有效的内容（如 ODBC 路线、Model/View、QSS、CMake、多窗口导航）保留并补充；已被优化的内容（连接池、一次 SHA-256、Qt Charts、HTML 伪 `.xls`、全表软删除、27 天横向推进）替换为优化方案（单数据库 Worker、PBKDF2、Qt Graphs、标准 CSV/QXlsx、主数据停用、8 周纵向切片）。原版完整计划见《[原版计划](C++_Qt_MySQL_仓库管理系统_原版计划.md)》文件，仅作历史设计参考，不作为执行主线。
>
> **与优化版计划的关系：** 优化版计划第 1-8 节给出目标、取舍和八周路线的骨架；本部分给出落地所需的代码、DDL、目录结构、知识清单、练习、资源和验收标准。两者出现重叠时，以优化版计划的决策为准。

## 详细计划目录

- [[#A. 项目概述与定位]]
- [[#B. 环境准备（Day 0）]]
- [[#C. 技术专题学习与练习]]
- [[#D. 项目架构设计]]
- [[#E. 数据库 Schema 设计]]
- [[#F. 分步实现计划（8 周路线展开）]]
- [[#G. 关键设计决策]]
- [[#H. 常见陷阱与解决方案]]
- [[#I. 面试准备要点]]
- [[#附录：学习资源汇总]]

---

## A. 项目概述与定位

### A.1 项目定位

构建一个**作品集级、可测试、可部署的企业物资进销存管理系统**，覆盖多角色权限、物资主数据、入出库、库存流水与余额、审计和导出闭环。项目不以“页面数量最多”为目标，而以“库存一致性、异步线程模型、可复现测试与部署”作为核心证据。

- **技术栈：** C++17 · Qt 6.11 · MySQL 8.0 · CMake · QTest · GitHub Actions
- **项目名称：** WMS (Warehouse Management System)
- **工作目录：** `D:\Desktop\仓库管理系统\WMS\`（原计划写的 `D:\Desktop\WMS\` 为旧路径，已统一为实际仓库路径）

### A.2 核心功能清单

| 模块 | 功能 | 涉及技术点（优化版） |
|------|------|-----------|
| 🔐 用户认证 | 登录/登出、PBKDF2-HMAC-SHA256、角色权限（Admin/Manager/Operator） | `QPasswordDigestor`、参数化密码存储、Service 二次鉴权 |
| 📦 物资管理 | 物资/分类/单位 CRUD、编码唯一、停用 | 自定义 `QAbstractTableModel`、服务端分页 |
| 📥 入库管理 | 草稿+确认、明细、状态守卫、流水与余额 | 条件事务、affectedRows 守卫、幂等流水 |
| 📤 出库管理 | 草稿+确认、库存校验、原子扣减 | 带条件 `UPDATE ... AND quantity>=:qty`、无负库存 |
| 📊 库存管理 | 余额查询、流水追溯、低库存预警、自动对账 | `stock_movements`（事实）+ `stock_balance`（投影） |
| 📋 审计日志 | 登录/变更/确认/导出全追踪、不可修改 | 追加写入、最小权限账号、冲销而非删除 |
| 📑 报表导出 | 标准 CSV（MVP）/ QXlsx（增强） | UTF-8 BOM、RFC 4180、公式注入防护、流式写入 |
| 📈 数据看板 | 统计卡片（MVP）/ Qt Graphs（增强） | `QQuickWidget` 嵌入 QML、Widgets/QML 混合 |
| 🎨 UI | 四态（Loading/Ready/Empty/Error）、QStackedWidget 导航 | QSS、键盘操作、角色菜单过滤 |

### A.3 为什么这个项目能写入简历？

| 简历亮点 | 说明（优化版） |
|----------|------|
| **异步数据库线程** | 单 Worker + 有界队列 + 状态机，隔离 UI 与 Qt SQL 线程归属，避免桌面场景过度设计连接池 |
| **分层架构** | presentation → application → domain → infrastructure，Repository Port 隐藏 SQL，可为 C/S 版本复用 |
| **条件事务与幂等** | affectedRows 守卫 + 流水唯一键 + 原子扣减，保证重复确认不重复记账、并发不产生负库存 |
| **库存可追溯** | 不可变流水 + 余额投影 + 自动对账，余额可解释、可冲销 |
| **自定义 Model/View** | `QAbstractTableModel` + Delegate + 服务端分页 + `QAbstractItemModelTester` |
| **安全实践** | PBKDF2 参数化存储、随机盐、Prepared Statements、脱敏日志、最小权限 |
| **工程化** | 顶层 CMake/CTest/CMakePresets、GitHub Actions、Windows Release、Docker Compose、可复现 P95 |

> **设计取舍说明：** 旧方案把连接池、SHA-256、Qt Charts、10 个页面当作主亮点；优化版改为异步线程、PBKDF2、条件事务、库存对账、Model/View 测试与可复现部署。

---

## B. 环境准备（Day 0）

> ⏱ 预计耗时：3-4 小时  
> 🎯 目标：在 15 分钟内从空环境完成数据库初始化；任何连接失败都能给出明确下一步

### B.1 当前环境检查

| 组件 | 状态 | 路径/版本 |
|------|------|-----------|
| Qt 6.11.1 | ✅ 已安装 | `D:\QT\6.11.1\mingw_64\` (MinGW 64-bit) |
| CMake 3.30.5 | ✅ 已安装 | `D:\QT\Tools\CMake_64\cmake.exe` |
| MySQL 8.0 | ✅ 已安装 | `C:\Program Files\MySQL\MySQL Server 8.0\` |
| MySQL ODBC Connector | ❌ 需要安装 | [下载地址](https://dev.mysql.com/downloads/connector/odbc/) |
| Qt MySQL 原生驱动 | ❌ 不存在 | `qsqlmysql.dll` 未编译，走 ODBC 路线 |

### B.2 安装步骤

#### Step 1: 安装 MySQL ODBC Connector

1. 访问 [MySQL Connector/ODBC 下载页](https://dev.mysql.com/downloads/connector/odbc/)
2. 选择 **Windows (x86, 64-bit), MSI Installer** 版本
3. 安装时选择 **“Unicode”** 驱动（不要选 ANSI）
4. 验证安装：打开“ODBC 数据源管理器 (64-bit)” → “驱动程序”标签 → 应看到 `MySQL ODBC 8.0 Unicode Driver`

#### Step 2: 启动 MySQL 并创建数据库

```bash
# 以管理员身份启动 MySQL 服务
net start MySQL80

# 登录 MySQL
mysql -u root -p

# 创建项目数据库（统一 utf8mb4 与时区）
CREATE DATABASE wms DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

#### Step 3: 验证 Qt ODBC 连接（诊断程序）

创建一个只执行 `SELECT 1` 的 ODBC 诊断程序，输出驱动列表和脱敏错误：

```cpp
// tests/db_diag/main.cpp
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    qDebug() << "Available drivers:" << QSqlDatabase::drivers();

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("DRIVER={MySQL ODBC 8.0 Unicode Driver};"
                       "SERVER=127.0.0.1;PORT=3306;DATABASE=wms;"
                       "UID=root;PWD=******;");  // 凭据从环境变量读取，不硬编码

    if (db.open()) {
        qDebug() << "✅ Database connected!";
        QSqlQuery query(db);
        if (query.exec("SELECT 1")) {
            qDebug() << "✅ Query test passed!";
        }
    } else {
        qDebug() << "❌ Connection failed:" << db.lastError().text();
    }
    return 0;
}
```

#### Step 4: 用 Docker Compose 启动可复现 MySQL（推荐用于集成测试）

```yaml
# docker-compose.yml
services:
  mysql:
    image: mysql:8.0
    environment:
      MYSQL_ROOT_PASSWORD: ${WMS_DB_ROOT_PASSWORD}
      MYSQL_DATABASE: wms
    command: --character-set-server=utf8mb4 --collation-server=utf8mb4_unicode_ci
    ports: ["3306:3306"]
    volumes:
      - ./sql/migrations:/docker-entrypoint-initdb.d
      - wms-mysql:/var/lib/mysql
volumes:
  wms-mysql:
```

### B.3 环境排错表（四类故障必须能区分）

| 故障类型 | 现象 | 排查方向 |
|---|---|---|
| Qt SQL 插件缺失 | `QSqlDatabase::drivers()` 不含 `QODBC` | 检查 `sqldrivers/qsqlodbcd.dll` 是否随程序部署 |
| ODBC 驱动缺失 | “Data source name not found” | 打开 ODBC 数据源管理器确认 `MySQL ODBC 8.0 Unicode Driver` |
| 数据库服务未启动 | “Can't connect to MySQL server” | `net start MySQL80` 或 `docker compose up` |
| 凭据错误 | “Access denied for user” | 校验用户名/密码、来源环境变量、权限范围 |

### B.4 知识点与练习任务

**知识点清单：**

- [ ] 确认 Qt 6.11.1 MinGW、CMake 3.30.5、MySQL 8.0 和 64 位 ODBC 驱动版本。
- [ ] 使用 `QSqlDatabase::drivers()` 检查 `QODBC` 是否可用。
- [ ] 创建 `wms` 数据库，统一 `utf8mb4` 和时区设置。
- [ ] 区分 Qt SQL 插件缺失、ODBC 驱动缺失、数据库服务未启动、凭据错误四类故障。
- [ ] 为开发库、集成测试库和演示库设置不同连接配置，禁止测试连接生产库。

**练习任务：**

1. 写一个只执行 `SELECT 1` 的 ODBC 诊断程序，输出驱动列表和脱敏错误。
2. 通过 Docker Compose 启动 MySQL，执行版本化迁移和种子命令。
3. 在未安装 Qt SDK 的环境中记录从安装 ODBC 到首次启动所需时间。

**交付物：** `config.example.ini`、Docker Compose、ODBC 诊断程序、迁移入口和环境排错表。

**验收标准：** 能在 15 分钟内从空环境完成数据库初始化；任何连接失败都能给出明确的下一步操作。

**学习资源：**

- 📖 [Qt SQL Database Drivers 官方文档](https://doc.qt.io/qt-6/sql-driver.html)
- 📖 [Qt QSqlDatabase 使用指南](https://doc.qt.io/qt-6/qsqldatabase.html)
- 📖 [MySQL Connector/ODBC 下载](https://dev.mysql.com/downloads/connector/odbc/)

---

## C. 技术专题学习与练习

> 与原版“先学完所有技术再做项目”不同，优化版采用**纵向切片**：每完成一个业务切片，再补齐该切片需要的知识。本节按技术专题组织知识清单、代码、练习和资源，供对应周次按需查阅。

### C.1 Qt Model/View 深入（对应第 5 周）

**为什么重要：** 每个 CRUD 页面的数据绑定基础，Qt Widgets 岗位高频核心能力。

**知识点清单：**

- [ ] `QAbstractTableModel` 必须重写的纯虚函数：`rowCount()`、`columnCount()`、`data()`
- [ ] 可选重写：`headerData()`、`flags()`、`setData()`、`roleNames()`
- [ ] `Qt::ItemDataRole`：`DisplayRole`、`EditRole`、`ForegroundRole`、`BackgroundRole`、`UserRole`
- [ ] 重置通知：`beginResetModel()` / `endResetModel()`
- [ ] 增量通知：`beginInsertRows()` / `endInsertRows()`、`beginRemoveRows()` / `endRemoveRows()`、`dataChanged()`
- [ ] `QSortFilterProxyModel` 只负责**当前页**展示，不承担全量数据筛选（全量筛选由服务端 SQL 完成）
- [ ] `QAbstractItemModelTester` 在 Model 完成后立即接入，自动校验索引与通知协议

**练习任务：** 手写 `ProductTableModel`，用内存 DTO 显示 20 条物资，测试空数据和增量更新，并接入 `QAbstractItemModelTester`。

```cpp
void ProductTableModel::setPage(const ProductPage& page) {
    beginResetModel();
    rows_ = page.items;
    totalCount_ = page.totalCount;
    endResetModel();
}
```

**学习资源：**

- 📖 [Qt Model/View Programming](https://doc.qt.io/qt-6/model-view-programming.html) — 必读
- 📖 [QAbstractTableModel](https://doc.qt.io/qt-6/qabstracttablemodel.html)
- 📖 [QAbstractItemModelTester](https://doc.qt.io/qt-6/qabstractitemmodeltester.html)

### C.2 QSS 样式与四态交互（对应第 5 周）

**为什么重要：** 好看的 UI 是第一印象，但颜色不能承担唯一语义（无障碍）。

**知识点清单：**

- [ ] QSS 选择器、伪状态（`:hover`、`:pressed`、`:checked`、`:disabled`）、盒模型
- [ ] 常用控件样式：`QTableView`、`QPushButton`、`QLineEdit`、`QComboBox`、`QScrollBar`
- [ ] 成功/错误/预警/禁用四态的颜色语义，且颜色不承担唯一语义（配文字/图标）
- [ ] 高 DPI、键盘焦点、Tab 顺序、空数据和加载状态
- [ ] 页面对象树和缓存页面的析构时机

**色板参考（Catppuccin Mocha）：**

| 颜色名 | 用途 | 色值 |
|--------|------|------|
| Base | 页面背景 | `#1e1e2e` |
| Surface0 | 卡片/表格背景 | `#313244` |
| Surface2 | 输入框背景 | `#45475a` |
| Text | 正文 | `#cdd6f4` |
| Blue | 强调/选中 | `#89b4fa` |
| Green | 成功/入库 | `#a6e3a1` |
| Red | 错误/出库/预警 | `#f38ba8` |
| Yellow | 警告 | `#f9e2af` |

**交付物：** `style.qss`、登录页、主窗口导航、Product 页、Inventory 页和四态组件。

**验收标准：** 颜色不承担唯一语义；键盘能完成核心操作；错误信息包含原因和重试动作；页面切换不重复创建或泄漏对象。

**学习资源：**

- 📖 [Qt Style Sheets Reference](https://doc.qt.io/qt-6/stylesheet-reference.html)
- 📖 [Qt Style Sheets Examples](https://doc.qt.io/qt-6/stylesheet-examples.html)
- 🎨 [Catppuccin 色板](https://catppuccin.com/palette)

### C.3 多窗口导航架构（对应第 5 周）

**为什么重要：** 企业级应用不可能是单窗口，导航架构决定可维护性。

**知识点清单：**

- [ ] `QMainWindow` + `QStackedWidget` + 侧边 `QListWidget` 联动
- [ ] 页面工厂/缓存：按 key 创建并缓存，切换回来不重建
- [ ] 角色路由：菜单按 `minRole` 过滤，**且 Service 再次校验权限**（隐藏菜单不是授权边界）
- [ ] 跨页面刷新通过 Service/事件总线，而非 widget 直接互连

**菜单路由表设计：**

```cpp
struct PageRoute {
    QString key, title, icon;
    Role minRole;
};

const QVector<PageRoute> ROUTES = {
    {"dashboard", "看板",   "dashboard.png", Role::Operator},
    {"products",  "物资管理","box.png",       Role::Operator},
    {"inbound",   "入库管理","in.png",        Role::Operator},
    {"outbound",  "出库管理","out.png",       Role::Operator},
    {"inventory", "库存管理","warehouse.png", Role::Manager},
    {"audit",     "审计日志","log.png",       Role::Admin},
    {"users",     "用户管理","user.png",      Role::Admin},
};
```

**练习任务：** 做一个 3 页导航 Demo，切换页面时不重复创建已缓存页面；权限不足时菜单隐藏，且对应 Service 调用也被拒绝。

### C.4 异步数据库线程模型（对应第 1 周）⭐

**为什么重要：** 这是优化版最大的架构亮点，替代原版的连接池。Qt SQL 连接具有线程归属，连接和查询都留在数据库线程更容易保证正确性；桌面单用户程序通常不需要连接池。

**核心结构：** `DatabaseExecutor` + `DatabaseWorker` + `QThread`

- Worker 在线程内创建和使用**唯一** `QSqlDatabase`。
- Executor 提供有界任务队列、requestId、队列满错误、健康检查、异步关闭和排空超时。
- 正在执行的 `QSqlQuery::exec()` **不承诺可以被强杀**；只实现排队取消、latest-wins 和软超时。

**Executor 状态机：** `Starting → Ready → (Failed) → ShuttingDown → Stopped`

**四种取消/超时语义（必须分别测试）：**

| 行为 | 含义 |
|---|---|
| 排队取消 | 任务尚未派发时从队列移除，返回 Cancelled |
| latest-wins | 旧搜索可继续执行，但结果到达后被丢弃，不覆盖新搜索 |
| 软超时 | 页面到时恢复可操作并提示超时，底层查询可能仍在 Worker 完成 |
| 执行超时 | 只能依赖 ODBC/MySQL 的登录/锁等待/语句超时，不能强杀 QThread |

**知识点清单：**

- [ ] Qt SQL 连接必须在所属线程创建、打开、查询和关闭
- [ ] QThread Worker Object 模式、对象迁移、线程启动和退出顺序
- [ ] 有界队列、requestId、队列满、初始化失败、健康检查和关闭排空
- [ ] `QMetaType` 注册和跨线程信号传递的值语义
- [ ] Prepared Statement、参数绑定、SQL 错误分类和敏感信息脱敏

**练习任务：**

1. 为 `DatabaseExecutor` 画状态图，编写状态迁移表。
2. 编写任务队列满、无效任务、连接失败和 shutdown drain timeout 测试。
3. 用 QLoggingCategory 输出 requestId、耗时和错误码，确认日志不含密码。

**交付物：** 可测试的 `DatabaseExecutor`、`DatabaseWorker`、`ConfigManager`、`PasswordHasher`、`SessionManager`。

**验收标准：** UI 线程不创建 `QSqlDatabase`；Worker 退出后连接已关闭；队列满和取消有明确结果；所有错误均可在 UI 显示为可操作提示。

**学习资源：**

- 📖 [Qt Threads and the SQL Module](https://doc.qt.io/qt-6/threads-modules.html#threads-and-the-sql-module)
- 📖 [QThread](https://doc.qt.io/qt-6/qthread.html)、[QLoggingCategory](https://doc.qt.io/qt-6/qloggingcategory.html)

### C.5 密码安全 PBKDF2（对应第 2 周）

**为什么重要：** 一次 SHA-256 即使加盐也过快，不适合密码存储。这是安全面试高频考点。

**知识点清单：**

- [ ] `QPasswordDigestor::deriveKeyPbkdf2`（PBKDF2-HMAC-SHA256），链接 `Qt6::Network`
- [ ] 随机盐：CSPRNG 至少 16 字节
- [ ] 数据库保存 `password_hash`、`password_salt`、`password_algorithm`、`password_iterations`
- [ ] OWASP 参考 PBKDF2-HMAC-SHA256 工作因子 600,000 次；实际值在目标机基准，使单次登录可接受
- [ ] 参数升级后登录重哈希；密码和 pepper 不进 Git/日志
- [ ] 求职增强阶段可通过成熟库升级 Argon2id，不自研算法

**练习任务：** 编写密码基准测试，记录不同迭代次数下单次登录耗时；实现登录重哈希。

**学习资源：**

- 📖 [Qt QPasswordDigestor](https://doc.qt.io/qt-6/qpassworddigestor.html)
- 📖 [OWASP Password Storage Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html)

### C.6 配置与会话管理（对应第 2 周）

**知识点清单：**

- [ ] `QSettings` 保存非敏感配置，环境变量覆盖连接信息
- [ ] 默认值回退、配置文件路径策略
- [ ] `SessionManager`：当前登录用户、角色枚举、`hasPermission(action)`
- [ ] 权限必须在 Service 再校验，隐藏菜单不是授权边界

**学习资源：**

- 📖 [QSettings](https://doc.qt.io/qt-6/qsettings.html)

### C.7 Repository 与 Application Service（对应第 2-4 周）⭐

**为什么重要：** 替代原版“同步 DAO 双版本方法”。Repository 隐藏 SQL、`DatabaseTask` 和原始 `QVariantList`，把结果映射为 DTO/`AppError`；Service 不拼 SQL。

**最小异步 Repository 契约：**

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

**所有权与职责：**

- `owner` 通常是 Service，请求对象随 owner 销毁；Repository 内部用 `QPointer` 防止向已销毁对象回调。
- 请求对象保存 requestId，只消费对应数据库结果。
- 页面只监听 Service 的用例信号，不直接认识 Repository 或 `DatabaseExecutor`。
- 一笔业务事务由具体 Repository 生成一个事务任务，Service 不拼 SQL。

**知识点清单：**

- [ ] Repository Port 与 MySQL Repository 的职责边界
- [ ] DTO、Entity、Value Object 和 `AppError` 的区别
- [ ] 异步 Operation 的所有权、requestId、latest-wins 和 QPointer
- [ ] 单语句任务与事务任务的结果映射
- [ ] Service 权限校验、业务校验和审计事件编排
- [ ] 不让 UI、Model 或 Service 直接拼 SQL

**练习任务：**

1. 完成 `ProductRepository::fetchPage()`，返回带总数和分页信息的 DTO。
2. 用 Fake Repository 测试 Service，不启动数据库也能验证权限和业务规则。

**学习资源：**

- 📖 [Qt QSqlQuery](https://doc.qt.io/qt-6/qsqlquery.html) — Prepared Statements
- 📖 [OWASP SQL Injection Prevention](https://cheatsheetseries.owasp.org/cheatsheets/SQL_Injection_Prevention_Cheat_Sheet.html)

### C.8 入库/出库条件事务（对应第 3-4 周）⭐

**为什么重要：** 整个项目最复杂的业务逻辑，面试必问。优化版用 affectedRows 守卫 + 幂等流水，替代原版 `SELECT ... FOR UPDATE` 的悲观锁思路。

**扩展 `DatabaseStatement` 增加 affectedRows 期望：**

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

**入库确认事务（同一 Worker 连接内）：**

```text
begin transaction
  1. UPDATE inbound_orders SET status='confirmed', confirmed_at=NOW()
       WHERE id=:id AND status='draft'        -- 必须影响 1 行，否则回滚
  2. FOR EACH 明细:
       a. UPSERT stock_balance (product_id, warehouse_id) 增加余额
       b. INSERT stock_movements (幂等唯一键)
  3. INSERT audit_logs
commit (任一守卫失败立即回滚)
```

**出库原子扣减（无需 FOR UPDATE）：**

```sql
UPDATE stock_balance
SET quantity = quantity - :qty
WHERE product_id = :pid AND warehouse_id = :wid AND quantity >= :qty;
-- 必须影响 1 行；0 行 = 库存不足，回滚并映射为业务错误
```

**知识点清单：**

- [ ] `QSqlDatabase::transaction()` / `commit()` / `rollback()`
- [ ] 事务内所有语句在同一个 Worker 连接（同一个 `DatabaseTask`）
- [ ] affectedRows 守卫、原子条件 UPDATE、幂等唯一键互补
- [ ] 流水插入违反唯一键时映射为“单据已处理”而非通用 SQL 错误
- [ ] 已确认单据通过冲销流水纠正，不直接修改历史流水

**练习任务：**

1. 完成 `InboundRepository::confirm()`，验证状态守卫、明细、流水、余额、日志全部在同一事务。
2. 完成 `OutboundRepository::confirm()`，验证库存不足和重复请求的错误映射。
3. 注入明细失败、重复确认和连接断开，验证全事务回滚和幂等。

**学习资源：**

- 📖 [MySQL 事务](https://dev.mysql.com/doc/refman/8.0/en/commit.html)
- 📖 [MySQL INSERT ... ON DUPLICATE KEY](https://dev.mysql.com/doc/refman/8.0/en/insert-on-duplicate.html)

### C.9 库存流水与对账（对应第 3-4 周）

**知识点清单：**

- [ ] `stock_movements`（不可变事实）与 `stock_balance`（投影）的关系
- [ ] 来源单据、来源明细和 movement role 的幂等模型
- [ ] 幂等键只用非空字段，**不把可空 `lot_id` 放入唯一约束**（MySQL 唯一索引允许多 NULL）
- [ ] 自动对账：`stock_balance.quantity = SUM(stock_movements.quantity_delta)`
- [ ] 主数据停用、已确认单据冲销和审计不可变策略

**练习任务：** 生成 1 万条流水，比较索引前后的 `EXPLAIN` 和 P95 查询耗时；编写自动对账测试。

### C.10 导出 CSV（对应第 6 周）

**为什么重要：** 替代原版“HTML 伪 `.xls`”。MVP 只交付安全 CSV，求职增强阶段再接 QXlsx。

**知识点清单：**

- [ ] UTF-8 BOM（`\xEF\xBB\xBF`）确保 Excel 识别中文
- [ ] RFC 4180 转义：含逗号、双引号、换行的字段加双引号，内部双引号转义为 `""`
- [ ] 公式注入防护：以 `=`、`+`、`-`、`@` 开头的单元格按策略转义
- [ ] 大文件流式逐行写入，不全量驻留内存

**练习任务：** 实现 `CsvExporter`，覆盖 BOM、转义、公式注入和大文件流式写入测试。

### C.11 图表与扫码（增强阶段）

**为什么重要：** Qt Charts 自 Qt 6.10 起已弃用，新项目推荐 Qt Graphs。扫码先支持键盘模拟，嵌入式方向再加 QSerialPort。

**知识点清单：**

- [ ] MVP 先做统计卡片和表格；增强阶段用 Qt Graphs/QML，通过 `QQuickWidget` 嵌入 Widgets
- [ ] 扫码先支持键盘模拟扫码枪；目标嵌入式岗位再增加 `QSerialPort` 设备抽象与可替换模拟器

**学习资源：**

- 📖 [Qt Graphs](https://doc.qt.io/qt-6/qtgraphs-index.html)、[Qt Charts（已弃用）](https://doc.qt.io/qt-6/qtcharts-index.html)
- 📖 [QSerialPort](https://doc.qt.io/qt-6/qserialport.html)

---

## D. 项目架构设计

### D.1 分层架构（优化版）

```
┌─────────────────────────────────────────────────────┐
│  presentation（展示层）                              │
│  widgets · models(QAbstractTableModel) · delegates  │
│  QSS · Qt Signal/Slot · 四态 UI                      │
├─────────────────────────────────────────────────────┤
│  application（应用层）                               │
│  services(用例/权限/事务编排) · dto · ports · errors │
├─────────────────────────────────────────────────────┤
│  domain（领域层）                                    │
│  entities · value_objects · rules（不依赖数据库）    │
├─────────────────────────────────────────────────────┤
│  infrastructure（基础设施层）                        │
│  database(Executor/Worker) · repositories(MySQL)     │
│  config · security(PBKDF2) · export(CSV/QXlsx)       │
├─────────────────────────────────────────────────────┤
│  shared（共享）                                      │
│  logging(脱敏) · result(Result<T,AppError>)          │
├─────────────────────────────────────────────────────┤
│  MySQL 8.0（持久层）                                 │
│  wms 数据库 · 版本化迁移 · 索引 · 外键               │
└─────────────────────────────────────────────────────┘
```

### D.2 异步数据流（以“查询物资列表”为例）

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

**必须补充的边界：** 请求超时和取消语义；页面销毁后的结果丢弃（QPointer）；同一搜索框连续查询只接受最新 requestId；队列满、连接失败、SQL 失败和业务校验失败使用不同错误码；日志记录 requestId、耗时、错误码，但不记录密码和完整敏感参数。

### D.3 分层依赖规则（严格单向）

```
presentation ──> application ──> domain
                                    ^
infrastructure ──> application ──────┘  (实现 ports)
```

**禁止事项：**

- ❌ `presentation` 直接包含 `QSqlQuery` 或 SQL 字符串
- ❌ `application` 依赖具体 Repository 实现；ports 不暴露 SQL、`DatabaseTask` 或原始 `QVariantList`
- ❌ `domain` 依赖 Qt Widgets、Qt SQL 或 MySQL
- ❌ UI、Model 或 Service 直接拼 SQL
- ❌ 页面订阅全局 `DatabaseExecutor::taskFinished`（应只订阅 Service 用例信号）

依赖通过构造函数注入，在 `main.cpp` 统一组装。

### D.4 完整目录结构

```
D:\Desktop\仓库管理系统\WMS\
├── CMakeLists.txt                  # 顶层: project, cxx_std_17, find_package(Qt6), 选项, add_subdirectory
├── CMakePresets.json               # Windows Debug / Windows Release / Linux CI
├── config.example.ini              # 非敏感配置模板(凭据用环境变量覆盖)
├── docker-compose.yml              # MySQL 8.0 + 卷映射
├── README.md
├── sql/
│   ├── migrations/
│   │   ├── 001_initial.sql         # users/categories/units/products/orders/details
│   │   ├── 002_stock_movements.sql # warehouses/stock_movements/stock_balance/audit_logs
│   │   └── 003_seed.sql            # 默认仓库、单位、分类、Admin(PBKDF2)
│   └── rollback/                   # 各版本回滚脚本
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp                    # 组装依赖、加载 QSS、启动 MainWindow
│   ├── presentation/
│   │   ├── widgets/                # MainWindow, LoginWidget, ProductPage, InboundPage...
│   │   ├── models/                 # ProductTableModel, InventoryTableModel...
│   │   └── delegates/              # StatusDelegate, QuantityDelegate...
│   ├── application/
│   │   ├── services/               # AuthService, ProductService, InboundService...
│   │   ├── dto/                    # ProductPage, ProductSummary, AppError...
│   │   ├── ports/                  # IProductRepository, ProductPageRequest...
│   │   └── errors/                 # AppError, ErrorCode
│   ├── domain/
│   │   ├── entities/               # Product, InboundOrder, StockMovement...
│   │   ├── value_objects/          # OrderNo, Quantity, Money, SkuCode...
│   │   └── rules/                  # 不依赖数据库的领域规则
│   ├── infrastructure/
│   │   ├── database/               # DatabaseExecutor, DatabaseWorker, DatabaseTypes, DatabaseTask
│   │   ├── repositories/           # MySqlProductRepository, MySqlInboundRepository...
│   │   ├── config/                 # ConfigManager(QSettings + env override)
│   │   ├── security/               # PasswordHasher(PBKDF2), SessionManager
│   │   └── export/                 # CsvExporter (QXlsx 在增强阶段)
│   └── shared/
│       ├── logging/                # QLoggingCategory, 脱敏
│       └── result/                 # Result<T, AppError>
├── tests/
│   ├── domain/                     # 值对象、领域规则 QTest
│   ├── infrastructure/             # DatabaseExecutor 状态机/队列/事务测试
│   ├── application/                # Service 用 Fake Repository 测试
│   ├── integration/                # 独立 MySQL: 迁移/事务/双连接并发
│   └── ui/                         # QTest 登录/单据冒烟
└── cmake/
    └── Deploy.cmake                # qt_generate_deploy_app_script / windeployqt
```

> **设计取舍说明：** 不一开始创建大量空类。每完成一个纵向业务切片，再补齐该切片需要的实体、Repository、Service、Model 和页面。旧连接池可作为 `experiments/connection_pool` 学习成果保留，但生产代码不能同时依赖两套数据访问方式。

---

## E. 数据库 Schema 设计

### E.1 ER 关系概览（优化版）

```
users ──1:N── audit_logs
categories ──1:N── products
units ──1:N── products
warehouses ──1:N── stock_movements
warehouses ──1:N── stock_balance
products ──1:N── inbound_details / outbound_details
products ──1:N── stock_movements / stock_balance
inbound_orders ──1:N── inbound_details
outbound_orders ──1:N── outbound_details
(stock_movements.source_type/source_id/source_line_id 指向入库/出库明细)
```

### E.2 版本化迁移交付格式

每个 migration 必须包含：版本号、前置条件、变更内容、回滚策略和验证 SQL。不要继续维护一份容易与迁移不一致的手写“最终 DDL”。

### E.3 001_initial.sql（用户、主数据、订单与明细）

```sql
-- 版本: 1
-- 前置条件: 空库
-- 变更: 用户、分类、单位、物资、入出库单与明细
-- 回滚: 001_initial.down.sql (DROP TABLE ...)
-- 验证: SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='wms';

CREATE DATABASE IF NOT EXISTS wms DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE wms;

-- 用户表（PBKDF2 参数化密码存储）
CREATE TABLE users (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARBINARY(255) NOT NULL COMMENT 'PBKDF2 派生密钥',
    password_salt VARBINARY(64)  NOT NULL COMMENT 'CSPRNG 随机盐, >=16 字节',
    password_algorithm VARCHAR(20) NOT NULL DEFAULT 'pbkdf2_hmac_sha256',
    password_iterations INT UNSIGNED NOT NULL,
    real_name VARCHAR(50) NOT NULL,
    role ENUM('admin','manager','operator') NOT NULL DEFAULT 'operator',
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    INDEX idx_users_active (is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 主数据：分类 / 单位 / 物资（停用而非删除）
CREATE TABLE categories (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    code VARCHAR(30) NOT NULL UNIQUE,
    name VARCHAR(50) NOT NULL,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE units (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    code VARCHAR(20) NOT NULL UNIQUE,
    name VARCHAR(20) NOT NULL,
    is_active TINYINT(1) NOT NULL DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE products (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    code VARCHAR(30) NOT NULL UNIQUE COMMENT '物资编码',
    name VARCHAR(100) NOT NULL,
    category_id INT UNSIGNED NOT NULL,
    unit_id INT UNSIGNED NOT NULL,
    specification VARCHAR(200),
    safety_stock INT NOT NULL DEFAULT 0,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    FOREIGN KEY (category_id) REFERENCES categories(id),
    FOREIGN KEY (unit_id) REFERENCES units(id),
    INDEX idx_products_category (category_id),
    INDEX idx_products_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 入库单与明细
CREATE TABLE inbound_orders (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    order_no VARCHAR(40) NOT NULL UNIQUE COMMENT 'UUID/ULID 或带锁编号表',
    supplier VARCHAR(100),
    status ENUM('draft','confirmed','cancelled') NOT NULL DEFAULT 'draft',
    operator_id INT UNSIGNED NOT NULL,
    confirmed_at DATETIME(3) NULL,
    remark TEXT,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    FOREIGN KEY (operator_id) REFERENCES users(id),
    INDEX idx_inbound_status (status),
    INDEX idx_inbound_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE inbound_details (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    order_id INT UNSIGNED NOT NULL,
    product_id INT UNSIGNED NOT NULL,
    quantity INT NOT NULL,
    unit_price DECIMAL(12,2) NOT NULL DEFAULT 0.00,
    FOREIGN KEY (order_id) REFERENCES inbound_orders(id),
    FOREIGN KEY (product_id) REFERENCES products(id),
    INDEX idx_inbound_detail_order (order_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 出库单与明细（结构对称，省略冗余注释）
CREATE TABLE outbound_orders (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    order_no VARCHAR(40) NOT NULL UNIQUE,
    recipient VARCHAR(100),
    status ENUM('draft','confirmed','cancelled') NOT NULL DEFAULT 'draft',
    operator_id INT UNSIGNED NOT NULL,
    confirmed_at DATETIME(3) NULL,
    remark TEXT,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    FOREIGN KEY (operator_id) REFERENCES users(id),
    INDEX idx_outbound_status (status),
    INDEX idx_outbound_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE outbound_details (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    order_id INT UNSIGNED NOT NULL,
    product_id INT UNSIGNED NOT NULL,
    quantity INT NOT NULL,
    unit_price DECIMAL(12,2) NOT NULL DEFAULT 0.00 COMMENT '出库成本价',
    FOREIGN KEY (order_id) REFERENCES outbound_orders(id),
    FOREIGN KEY (product_id) REFERENCES products(id),
    INDEX idx_outbound_detail_order (order_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### E.4 002_stock_movements.sql（仓库、流水、余额、审计）

```sql
-- 版本: 2
-- 前置条件: 001_initial.sql 已应用
-- 变更: 仓库、不可变库存流水、库存余额投影、审计日志
-- 回滚: 002_stock_movements.down.sql
-- 验证: 自动对账 SELECT 检查 balance = SUM(delta)

USE wms;

CREATE TABLE warehouses (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    code VARCHAR(30) NOT NULL UNIQUE,
    name VARCHAR(50) NOT NULL,
    is_active TINYINT(1) NOT NULL DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 不可变库存流水（事实来源）
CREATE TABLE stock_movements (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    movement_no VARCHAR(40) NOT NULL UNIQUE,
    product_id INT UNSIGNED NOT NULL,
    warehouse_id INT UNSIGNED NOT NULL,
    lot_id INT UNSIGNED NULL COMMENT '批次, MVP 为 NULL',
    movement_type ENUM('inbound','outbound','adjust','reversal','transfer') NOT NULL,
    quantity_delta INT NOT NULL COMMENT '正入负出, 禁止为 0',
    source_type VARCHAR(20) NOT NULL COMMENT 'inbound/outbound/adjust/count',
    source_id INT UNSIGNED NOT NULL,
    source_line_id BIGINT UNSIGNED NOT NULL,
    movement_role ENUM('normal','transfer_out','transfer_in','reversal') NOT NULL DEFAULT 'normal',
    operator_id INT UNSIGNED NOT NULL,
    reason VARCHAR(200),
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    FOREIGN KEY (product_id) REFERENCES products(id),
    FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
    FOREIGN KEY (operator_id) REFERENCES users(id),
    -- 幂等键: 只用非空字段, 不含可空 lot_id
    UNIQUE KEY uk_movement_idem (source_type, source_id, source_line_id, movement_role),
    INDEX idx_movement_product_warehouse (product_id, warehouse_id),
    INDEX idx_movement_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 库存余额（投影, 提高查询性能）
CREATE TABLE stock_balance (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    product_id INT UNSIGNED NOT NULL,
    warehouse_id INT UNSIGNED NOT NULL,
    quantity INT NOT NULL DEFAULT 0,
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    FOREIGN KEY (product_id) REFERENCES products(id),
    FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
    -- MVP 无批次: 非空维度组合唯一; 批次版再升级为 (product_id, warehouse_id, location_id, lot_id)
    UNIQUE KEY uk_balance (product_id, warehouse_id),
    CHECK (quantity >= 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 审计日志（追加写入, 不可修改）
CREATE TABLE audit_logs (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    operator_id INT UNSIGNED NULL COMMENT 'NULL 表示系统操作',
    username VARCHAR(50) NOT NULL COMMENT '冗余, 防用户删除后失追溯',
    action VARCHAR(30) NOT NULL COMMENT 'login/logout/create/update/confirm/cancel/export',
    target_type VARCHAR(30),
    target_id VARCHAR(64),
    detail JSON,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    INDEX idx_audit_username (username),
    INDEX idx_audit_action (action),
    INDEX idx_audit_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### E.5 V2 核心表清单与约束

| 表 | 责任 | 必须约束 |
|---|---|---|
| `users` | 用户、角色、PBKDF2 参数 | username 唯一、active 状态、密码字段不存明文 |
| `categories/units/products` | 主数据 | 编码唯一、外键、停用而非删除 |
| `warehouses` | 仓库维度 | name/code 唯一，MVP 预置默认仓库 |
| `inbound_orders/inbound_details` | 入库草稿和明细 | 状态流转、数量大于 0、来源明细 ID |
| `outbound_orders/outbound_details` | 出库草稿和明细 | 状态流转、数量大于 0、库存条件扣减 |
| `stock_movements` | 不可变库存事实 | 非空来源幂等键、正负数量、movement role |
| `stock_balance` | 当前余额投影 | 非空库存维度组合唯一、数量不小于 0 |
| `audit_logs` | 审计记录 | 追加写入、操作人/对象/时间完整 |

### E.6 单据与主数据规则

- 已确认入库/出库单、库存流水和审计日志**不可删除**，只能冲销或作废；主数据才允许停用。
- 技术主键使用 MySQL `AUTO_INCREMENT`；业务单号使用 UUID/ULID 或带事务锁的编号表，**不使用 `COUNT(*) + 1`**（MySQL 8 无通用 sequence）。
- 金额使用 `DECIMAL`，C++ 层不用二进制浮点累计财务金额。
- 已确认单据不使用级联删除。
- 给常用组合筛选建立复合索引，而不是给每一列单独建索引。

### E.7 种子数据（PBKDF2 Admin）

```sql
-- 003_seed.sql
INSERT INTO warehouses (code, name) VALUES ('WH01', '默认仓库');
INSERT INTO categories (code, name) VALUES
  ('RAW','原材料'),('WIP','半成品'),('FG','成品'),('AUX','辅料'),('PKG','包装材料');
INSERT INTO units (code, name) VALUES
  ('PCS','个'),('BOX','箱'),('KG','千克'),('M','米'),('L','升');

-- Admin 密码由 PasswordHasher 生成, 此处占位; 真实 hash/salt/iterations 由种子工具写入
INSERT INTO users (username, password_hash, password_salt, password_algorithm,
                   password_iterations, real_name, role) VALUES
('admin', 0x<hash>, 0x<salt>, 'pbkdf2_hmac_sha256', 600000, '系统管理员', 'admin');
```

### E.8 Schema 知识清单与练习

- [ ] InnoDB 事务、隔离级别、行锁、锁等待超时和死锁重试边界。
- [ ] 外键、非空组合唯一键、复合索引和 `EXPLAIN`。
- [ ] `stock_movements` 与 `stock_balance` 的事实来源/投影关系。
- [ ] 来源单据、来源明细和 movement role 的幂等模型。
- [ ] 主数据停用、已确认单据冲销和审计不可变策略。
- [ ] `DECIMAL` 金额、时区、utf8mb4、迁移版本和种子数据安全。

**练习任务：**

1. 编写 `001_initial.sql` 与 `002_stock_movements.sql`，统一 utf8mb4、时区、外键和迁移入口。
2. 为入库、出库、盘点分别定义 `source_type/source_line_id/movement_role`。
3. 写迁移 runner，测试空库、重复执行、升级失败回滚和测试库清理。
4. 生成 1 万条流水，比较索引前后的 `EXPLAIN` 和 P95 查询耗时。

**交付物：** 版本化迁移、种子工具、ER 图、索引说明、库存自动对账 SQL/测试。

**验收标准：** 首次入库能原子创建余额；两连接同时出库最多一个成功；重复确认不增加流水；余额与流水总和自动对账通过；确认单据不能被删除。

---

## F. 分步实现计划（8 周路线展开）

> 每天按 2-4 小时估算，额外预留最多 2 周缓冲。每周都要有可运行增量，按纵向切片推进。**每日详细任务见上方主计划第 7.1 节**；本节给出周级阶段卡（目标、重点、交付、验收、风险）。

### F.1 总览时间线

```
W1 ███░░░░ 数据库执行器与事务守卫
W2 █████░░ Schema/PBKDF2/物资纵向切片
W3 ███████ 库存流水与入库
W4 ███████ 出库/冲销/双连接并发
W5 ███████ Model/View 与主窗口
W6 ███████ 审计/预警/CSV/统计
W7 ███████ 质量与性能
W8 ███████ 部署与作品集
Buffer ░░░░░ 只修阻塞 bug, 不新增页面
```

### F.2 周级阶段卡

#### 第 1 周：顶层工程与数据库执行器
- **目标：** 固化 `DatabaseExecutor/Worker` 状态机和线程约束。
- **重点：** affectedRows 事务守卫、排队取消、软超时、latest-wins、错误分类；删除正式构建对旧连接池的依赖。
- **交付：** 数据库执行器可构建测试，UI 线程不执行 SQL。
- **验收：** 初始化成功/失败、单语句、事务回滚、队列满、关闭排空测试通过。
- **风险：** QObject 生命周期、关闭排空、执行中查询不可强杀。

#### 第 2 周：Schema、认证与物资纵向切片
- **目标：** 完成 Product 的 Entity → Repository → Service → Model → 页面闭环。
- **重点：** 版本化 Schema、PBKDF2、Session/Permission、分页搜索、唯一编码、停用。
- **交付：** 可登录并完成物资 CRUD。
- **验收：** 测试覆盖密码、权限、Repository 映射和 Model。

#### 第 3 周：库存流水与入库
- **目标：** 实现订单、明细、库存流水和余额；入库草稿/编辑/确认/重复确认保护。
- **重点：** 状态守卫、余额 UPSERT、幂等流水、审计，全部在同一事务。
- **交付：** 入库后余额和流水一致，可追溯操作人和来源单据。
- **验收：** 事务失败注入测试验证整体回滚；重复确认不增加流水。

#### 第 4 周：出库与异常修正
- **目标：** 出库库存校验、条件更新、负库存保护；已确认单据冲销。
- **重点：** 带条件的原子 UPDATE + affectedRows 检查；冲销流水而非修改历史。
- **交付：** 核心进销存闭环完成。
- **验收：** 使用两个独立数据库连接测试库存不足、双击确认、重复请求、首次余额、并发出库。

#### 第 5 周：Model/View 与业务 UI
- **目标：** 通用分页控件、基础 Delegate、四态页面、角色导航、键盘操作。
- **重点：** `QAbstractItemModelTester`、服务端分页/排序/筛选、latest-wins 覆盖。
- **交付：** 物资、入库、出库、库存页面可稳定使用。
- **验收：** ModelTester 通过；搜索旧请求返回不覆盖新请求；页面销毁无悬空回调。

#### 第 6 周：审计、预警、导出与看板
- **目标：** 不可修改审计查询、低库存实时查询、CSV 导出、统计卡片。
- **重点：** CSV 的 BOM/RFC 4180/公式注入/流式；审计权限；统计与数据库对账。
- **交付：** 业务功能齐全，能生成演示数据和报表。
- **验收：** 普通用户不能修改审计；Excel 能正确打开 CSV；跨页面刷新与断线恢复正常。

#### 第 7 周：质量与性能
- **目标：** 可复现性能报告和测试报告。
- **重点：** 1 万条流水数据生成器（固定种子）、`EXPLAIN` 与复合索引、QElapsedTimer 记录机器/数据规模/预热/重复次数、严格编译警告。
- **交付：** 优化前后 SQL 报告、集成/UI 测试全量通过、性能基准表。
- **验收：** 不凭主观感觉判断“不卡”；clang-tidy/ASan/UBSan/CodeQL 可留到增强阶段。

#### 第 8 周：部署与作品集
- **目标：** 干净环境可初始化的 Release。
- **重点：** Windows ZIP、Qt SQL/ODBC/平台插件部署、Docker Compose MySQL、迁移/种子、README、CI。
- **交付：** 可启动 ZIP、15 分钟初始化流程、CI 绿色记录、v1.0.0 Release。
- **验收：** 未安装 Qt SDK 的环境可启动；README 含构建、初始化、测试、截图、架构、已知限制；简历每个数字可复现。

#### 缓冲与求职增强
- **Buffer Day 1-5：** 只修复阻塞 MVP 的 bug，不新增页面。
- **Enhancement A：** 盘点/冲销审批和 QXlsx。
- **Enhancement B：** Qt Graphs/QML 或 QSerialPort 扫码。
- **Enhancement C：** 多平台 CI、安装器、Sanitizer、CodeQL 和 10 万条性能报告。
- **Enhancement D：** 根据目标岗位只选择 C/S、嵌入式、多仓批次或插件化其中一个。

> **设计取舍说明：** 延期时移出盘点、图表、XLSX、扫码和高级 Delegate，不牺牲事务和测试。

---

## G. 关键设计决策

### 决策 1: ODBC vs 原生 MySQL 驱动（保留，仍有效）

| 方案 | 优点 | 缺点 |
|------|------|------|
| **ODBC 桥接（本项目选择）** | 5 分钟安装 Connector 即可用；不依赖 Qt 源码编译 | 多一层桥接，桌面应用性能可忽略 |
| 编译 `qsqlmysql.dll` | 原生连接，少一层桥接 | 需要 Qt 源码 + 重新编译插件，耗时复杂 |

**结论：** 选 ODBC。环境无原生驱动也无 Qt 源码。

### 决策 2: 自定义 Model vs QSqlTableModel（保留，仍有效）

| 方案 | 优点 | 缺点 |
|------|------|------|
| **自定义 QAbstractTableModel（本项目选择）** | 完全可控（分页、自定义显示、高亮）；与 Service 层解耦 | 需要手写更多代码 |
| `QSqlTableModel` | 开箱即用，自动 CRUD | 分页困难；不支持复杂 JOIN；自定义显示受限 |

**结论：** 选自定义 Model。项目需要分页、自定义高亮、JOIN 显示，且要在 UI 线程之外取数。

### 决策 3: 异步 Repository Operation vs 同步 DAO 双版本（替换原版）

原版“独立版/事务版双版本 DAO + Service 获取 `QSqlDatabase&`”会破坏 Qt SQL 线程归属。优化版改为异步 Repository：

```cpp
// Repository 返回有 QObject 所有权的请求对象, 隐藏 SQL 与连接
ProductPageRequest* IProductRepository::fetchPage(const ProductFilter& filter,
                                                  QObject* owner);
```

**为什么：** SQL 只能在 Worker 线程执行；Service 只看领域对象和 `AppError`；UI 只调用 Service 并通过信号接收四态。Repository 接口还可在求职增强阶段替换为 HTTP Repository，而不重写 UI 和业务层。

### 决策 4: 单 Worker 数据库线程 vs 连接池（替换原版）

| 方案 | 适合场景 | 取舍 |
|------|---------|------|
| **单 Worker + 有界队列（本项目选择）** | 桌面单用户 | Qt SQL 线程归属清晰；UI 不被 SQL 阻塞；避免过度设计 |
| 连接池 | 多线程服务端/高并发 | 桌面场景徒增复杂度，且连接健康检查、扩缩容是额外负担 |

**结论：** 桌面单用户程序用串行数据库执行器更贴合真实负载。旧连接池作为 `experiments/` 学习成果保留，不进正式构建。

### 决策 5: 条件事务 affectedRows 守卫（新增）

`DatabaseTask` 只是静态 SQL 列表时，`UPDATE ... WHERE status='draft'` 影响 0 行 Worker 仍会继续执行后续库存更新。扩展 `DatabaseStatement` 支持 `AffectedRowsExpectation`，Worker 每条语句检查 `numRowsAffected()`，不满足立即回滚并返回明确业务错误。这把“状态前置条件”从应用层 if 判断下沉为数据库原子守卫，与幂等唯一键互补。

### 决策 6: PBKDF2 vs 一次 SHA-256（替换原版）

一次 SHA-256 即使加盐也过快，不适合密码存储。MVP 使用 `QPasswordDigestor::deriveKeyPbkdf2`（PBKDF2-HMAC-SHA256），参数化存储算法名、迭代数、盐。OWASP 参考 600,000 次；实际值在目标机基准。求职增强阶段可通过成熟库升级 Argon2id，不自研算法，不把数据库密码和 pepper 提交到 Git。

### 决策 7: 库存流水 + 余额投影 vs 单一 inventory（替换原版）

原版只维护 `inventory.quantity`，发生错误时难以解释“库存为什么是这个数字”。优化版新增不可变 `stock_movements`（事实来源）与 `stock_balance`（投影）：流水负责可追溯性，余额负责查询性能。确认单据时在同一事务内完成状态守卫 → 余额原子更新 → 幂等流水 → 审计。自动对账验证 `stock_balance.quantity = SUM(stock_movements.quantity_delta)`。

### 决策 8: 主数据停用 vs 全表软删除（替换原版）

- 物资、分类、用户等主数据可停用（`is_active`）。
- 已确认的入库单、出库单、库存流水和审计日志**不应删除**，只能冲销或作废并保留原因。
- 草稿单可以删除；已确认单必须通过反向流水纠正。

这比统一添加 `deleted_at` 更符合审计和库存可追溯性。

### 决策 9: Qt Graphs vs Qt Charts（替换原版）

Qt Charts 自 Qt 6.10 起已弃用，新项目推荐 Qt Graphs。MVP 先做统计卡片和表格；增强阶段用 Qt Graphs/QML 通过 `QQuickWidget` 嵌入 Widgets，展示 Widgets/QML 混合开发。若混合栈影响 MVP 进度，图表整体后移。

### 决策 10: 版本化迁移 vs 手写最终 DDL（替换原版）

使用 `001_initial.sql`、`002_stock_movements.sql` 等版本化迁移文件，每个包含版本号、前置条件、变更、回滚和验证 SQL。不维护一份容易与迁移不一致的手写“最终 DDL”。

### 决策 11: 信号-槽跨页面同步（保留，更新为经 Service）

页面间刷新经 Service/事件总线，而非 widget 直接互连：

```cpp
// MainWindow 作为信号中枢, 但刷新由 Service 状态驱动
connect(productService, &ProductService::dataChanged,
        inventoryPage, &InventoryPage::refresh);
```

各页面在保存后通过 Service 发出用例信号，监听者刷新。这保持页面间解耦，且刷新逻辑可测试。

---

## H. 常见陷阱与解决方案

### 陷阱 1: ODBC 连接字符串不匹配（保留，仍有效）

```
❌ DRIVER={MySQL ODBC 8.0 ANSI Driver}   // 32位 Qt
✅ DRIVER={MySQL ODBC 8.0 Unicode Driver} // 64位 Qt
```

**现象：** “Data source name not found”  
**解决：** 打开 ODBC 数据源管理器 (64-bit) 确认驱动名。

### 陷阱 2: QAbstractTableModel 视图不刷新（保留，仍有效）

**原因：** 忘记 `beginResetModel()` / `endResetModel()` 或增量通知。

```cpp
void ProductTableModel::setPage(const ProductPage& page) {
    beginResetModel();
    rows_ = page.items;
    endResetModel();
}
```

### 陷阱 3: 异步旧结果覆盖新结果（新增）⭐

**现象：** 快速连续搜索，旧请求返回覆盖新结果。  
**原因：** 未用 requestId 做 latest-wins。  
**解决：** 每次请求记录 requestId，只消费匹配的结果；旧结果到达即丢弃。

### 陷阱 4: 页面销毁后回调悬空（新增）⭐

**现象：** 切走页面后崩溃。  
**原因：** Worker 完成时回调已销毁对象。  
**解决：** Repository 用 `QPointer` 持有 owner；请求对象随 owner 销毁；不要让页面订阅全局 `taskFinished`。

### 陷阱 5: UI 线程创建 QSqlDatabase（新增）⭐

**现象：** 跨线程使用连接报错或崩溃。  
**原因：** Qt SQL 连接有线程归属。  
**解决：** 连接只在 Worker 线程内创建、打开、查询、关闭；UI 线程只提交任务。

### 陷阱 6: affectedRows 守卫误判（新增）⭐

**现象：** 库存不足时仍继续执行。  
**原因：** 未检查 `numRowsAffected()` 或把 0 行当成功。  
**解决：** `UPDATE ... AND quantity>=:qty` 必须 affectedRows=1；0 行映射为业务错误并回滚。

### 陷阱 7: 流水幂等键含可空 lot_id（新增）⭐

**现象：** 重复确认仍产生重复流水。  
**原因：** MySQL 唯一索引允许多 NULL，把可空 `lot_id` 放入唯一键导致幂等失效。  
**解决：** 幂等键只用非空 `source_type + source_id + source_line_id + movement_role`。

### 陷阱 8: 事务连接不统一（更新原版）

**现象：** 事务内某操作用了不同连接，数据不一致。  
**原因（优化版）：** 把多步拆成多个 `DatabaseTask` 而非一个事务任务。  
**解决：** 一笔业务事务由具体 Repository 生成**一个**事务任务，所有语句在同一个 Worker 连接内执行；Service 不拼 SQL、不持有 `QSqlDatabase&`。

### 陷阱 9: MySQL “server has gone away”（更新原版）

**现象：** 运行一段时间后数据库操作失败。  
**原因：** MySQL `wait_timeout` 默认 8 小时空闲断开。  
**解决：** Worker 健康检查（定时 `SELECT 1`）；连接失效时重建。

### 陷阱 10: PBKDF2 迭代次数过高导致登录卡顿（新增）

**现象：** 登录耗时过长，UI 卡顿。  
**解决：** 在目标机基准测试迭代次数；PBKDF2 在 Worker 线程执行；参数可迁移，支持登录重哈希。

### 陷阱 11: 单号用 COUNT(*)+1 并发冲突（新增）

**现象：** 并发创建单号重复。  
**解决：** 业务单号使用 UUID/ULID，或带事务锁的编号表，不用 `COUNT(*) + 1`。

### 陷阱 12: 软删除查询遗漏（更新原版）

**现象：** 停用数据仍显示。  
**原因：** SQL 忘加 `WHERE is_active = 1`。  
**解决：** 主数据查询统一封装 `is_active` 过滤；已确认单据不删除，无需软删除过滤。

### 陷阱 13: 图表/页面切换内存泄漏（更新原版）

**现象：** 页面切换后旧对象未释放。  
**解决：** 利用 QObject 父子对象树；缓存页面统一由父对象管理；Qt Graphs/QQuickWidget 在页面析构时清理。

---

## I. 面试准备要点

### I.1 技术面试常见问题（结合本项目，优化版回答）

| 面试问题 | 如何用本项目回答 |
|----------|----------------|
| “介绍一下你的项目” | 先讲业务闭环，再讲分层架构，重点提异步数据库线程和条件事务 |
| “为什么选择 ODBC 而不是 MySQL 原生驱动？” | Qt 6 默认不编译 MySQL 驱动；ODBC 安装 Connector 即可用；桌面应用性能无差异 |
| “数据库访问是怎么设计的？” | 单 Worker + 有界异步队列，Queued Connection 隔离 UI 与 Qt SQL 线程归属；Repository 隐藏 SQL |
| “为什么不用连接池？” | 桌面单用户场景串行执行器更贴合真实负载；连接池是服务端并发场景的方案 |
| “如何处理并发出库？” | 带条件的原子 `UPDATE ... AND quantity>=:qty` + affectedRows 守卫；双连接集成测试验证无负库存 |
| “重复确认会怎样？” | 状态守卫（`WHERE status='draft'` 必须 1 行）+ 流水幂等唯一键，双保险不重复记账 |
| “为什么要手写 Model？” | QSqlTableModel 不支持分页/复杂 JOIN/自定义高亮；自定义 Model 与 Service 解耦并支持服务端分页 |
| “密码是怎么存储的？” | PBKDF2-HMAC-SHA256 + 随机盐 + 参数化迭代数；存储算法名支持升级；登录重哈希 |
| “如何防止 SQL 注入？” | 全部 Prepared Statement + 绑定参数；Repository 是唯一拼 SQL 的地方 |
| “库存怎么保证可追溯？” | 不可变 `stock_movements`（事实）+ `stock_balance`（投影）+ 自动对账 + 冲销流水 |
| “如果让你重做，会怎么改进？” | C/S 化以隔离数据库凭据；多仓批次/FIFO/FEFO；插件化报表；Qt Graphs 看板 |

### I.2 项目中的技术亮点（面试主动提及）

1. **异步数据库线程模型** — “Qt SQL 连接有线程归属，我用单 Worker + 有界队列把所有 SQL 隔离在专用线程，UI 通过 Queued Connection 收四态。正在执行的 `QSqlQuery::exec()` 不能强杀，所以我区分了排队取消、latest-wins、软超时和执行超时四种语义，并用 QSignalSpy 测试状态机。”

2. **条件事务与库存幂等** — “确认入库/出库不是应用层 if 判断，而是数据库原子守卫：`UPDATE ... WHERE status='draft'` 必须 1 行，出库用 `UPDATE ... AND quantity>=:qty` 检查 affectedRows。配合流水幂等唯一键，双击、超时重试、消息重复都不会重复记账。我用两个独立连接测试并发出库不产生负库存。”

3. **库存可追溯** — “我拆成不可变 `stock_movements` 和 `stock_balance` 投影，余额可由流水对账。已确认单据不删除，只通过冲销流水纠正，符合审计要求。”

4. **Repository/Application Service 分层** — “UI 只认 Service 用例信号，Service 不碰 SQL，Repository 实现可替换为 HTTP 版本而不动 UI。依赖构造注入，在 main 组装。”

5. **工程化与可复现** — “CMake/CTest/CMakePresets/GitHub Actions；Windows Release 在干净机验证；性能用固定种子数据集 + EXPLAIN + P95，而不是主观‘不卡’。”

### I.3 简历写入验收

只有满足证据条件才写入对应简历句子：

| 简历表述 | 必须具备的证据 |
|---|---|
| 异步数据库线程 | Worker 线程测试、状态机日志、UI 响应演示 |
| 事务保证库存一致性 | 两连接并发测试、失败回滚测试、库存自动对账 |
| 自定义 Model/View | Model 源码、ModelTester 输出、分页测试 |
| 跨平台构建/部署 | Windows/Linux CI、干净机 ZIP 验证 |
| 性能优化 | 固定机器/数据集、EXPLAIN、优化前后 P95 |
| 安全实践 | PBKDF2 参数测试、脱敏日志、最小权限配置 |

**推荐模板（来自主计划第 8.4 节）：**

> **仓库管理系统 WMS** | C++17、Qt 6.11、MySQL 8、CMake  
> 设计并实现基于 Qt Widgets 的仓储桌面应用，覆盖物资、入库、出库、库存流水、权限和审计闭环。通过专用数据库线程和有界异步队列隔离 UI 与 Qt SQL 线程归属；使用条件事务、幂等流水和双连接集成测试验证重复确认不重复记账、并发出库不产生负库存；实现自定义 QAbstractTableModel/Delegate、服务端分页、CTest 和可复现 Windows Release。

**面试介绍顺序：** 业务问题 → 架构决策 → 最难的一致性问题 → Qt 特有问题 → 测试和量化结果 → 仍可改进之处。

**验收标准：** 模板中的每个数字和“保证”都能由仓库中的测试报告、CI 记录或性能脚本复现；未完成的增强项不写进简历。

---

## 附录：学习资源汇总

### 官方文档（首选）

| 资源 | 链接 | 用途 |
|------|------|------|
| Qt SQL Module | https://doc.qt.io/qt-6/sql-programming.html | SQL 编程完整指南 |
| Qt Model/View | https://doc.qt.io/qt-6/model-view-programming.html | Model/View 框架必读 |
| QAbstractItemModelTester | https://doc.qt.io/qt-6/qabstractitemmodeltester.html | Model 协议自动测试 |
| Qt Style Sheets | https://doc.qt.io/qt-6/stylesheet-reference.html | QSS 属性速查 |
| Qt QSqlDatabase | https://doc.qt.io/qt-6/qsqldatabase.html | 数据库连接类 |
| Qt QSqlQuery | https://doc.qt.io/qt-6/qsqlquery.html | Prepared Statements |
| Qt Threads and SQL | https://doc.qt.io/qt-6/threads-modules.html#threads-and-the-sql-module | 线程归属约束 |
| QPasswordDigestor | https://doc.qt.io/qt-6/qpassworddigestor.html | PBKDF2 密码派生 |
| QLoggingCategory | https://doc.qt.io/qt-6/qloggingcategory.html | 分类/脱敏日志 |
| Qt Graphs | https://doc.qt.io/qt-6/qtgraphs-index.html | 新图表模块（替代 Qt Charts） |
| QSerialPort | https://doc.qt.io/qt-6/qserialport.html | 扫码枪/串口设备 |
| qt_generate_deploy_app_script | https://doc.qt.io/qt-6/qt-generate-deploy-app-script.html | 部署脚本生成 |
| MySQL Connector/ODBC | https://dev.mysql.com/downloads/connector/odbc/ | ODBC 驱动下载 |
| OWASP Password Storage | https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html | 密码存储行业标准 |
| OWASP SQL Injection Prevention | https://cheatsheetseries.owasp.org/cheatsheets/SQL_Injection_Prevention_Cheat_Sheet.html | 防注入 |

### GitHub 参考项目（来自调研）

| 项目 | 重点学习内容 |
|------|------|
| [sqlitebrowser/sqlitebrowser](https://github.com/sqlitebrowser/sqlitebrowser) | 表格型数据库 UI、自定义 Model、QTest、多平台 CI、CodeQL、安装包 |
| [dail8859/NotepadNext](https://github.com/dail8859/NotepadNext) | 多目标 CMake、跨平台构建与发布工作流 |
| [QtExcel/QXlsx](https://github.com/QtExcel/QXlsx) | 正式 XLSX 读写、CMake 接入、CI、clang-format/tidy |
| [liolok/Inventory-Management](https://github.com/liolok/Inventory-Management) | 入库/出库/流水最小闭环业务参考（注意其时间戳丢日志问题，本项目用幂等键规避） |

### 视频教程（B站关键词）

| 关键词 | 内容 |
|-----------|------|
| “Qt Model View 框架” | Model/View/Delegate 三角关系 |
| “Qt QSS 样式表” | 深色主题 QSS 编写 |
| “Qt 多线程 SQL” | Qt 线程与数据库交互要点 |
| “Qt QStackedWidget 多页面” | 多窗口导航 |

### 书籍推荐

| 书名 | 说明 |
|------|------|
| 《Qt 5.9 C++开发指南》 | 国内系统 Qt 教程，含数据库章节 |
| 《C++ Concurrency in Action》（第2版） | 并发编程，线程模型参考 |
| 《MySQL 是怎样运行的》 | 深入 MySQL 内部机制 |
| 《C++17 完全指南》 | 现代 C++ 特性速查 |

> 📌 **最后建议：** 不要试图一次性做完美。先跑通最小闭环（登录 → 物资 CRUD → 入库确认 → 出库确认 → 库存对账），再叠加审计、预警、导出和看板。每周一个可运行增量，每天 commit，让 Git 记录成长过程——这也是面试时展示的“工程习惯”。
