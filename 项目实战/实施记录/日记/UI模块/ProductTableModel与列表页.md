---
title: ProductTableModel与列表页
type: 通用笔记
topic: 项目实战
status: 学习中
created: 2026-07-28
tags:
  - wms
  - qt
  - mysql
  - 实施记录
series: WMS 实施记录
canvas: 实施记录/白板/UI模块/Day12 ProductTableModel与列表页.canvas
---

# ProductTableModel与列表页

> 记录「ProductTableModel与列表页」当天的任务、实现过程与结论。

[[项目实战/计划/优化版详细实施计划]]
[[Model View]]

**目标：** 建立 Product 列表的 Presentation 层基础，让 UI 通过 ProductService 查询分页数据，而不是直接接触 Repository 或 SQL。

**知识点：**

- `QAbstractTableModel` 的职责、`rowCount()`、`columnCount()`、`data()`、`headerData()`。

- `beginResetModel()/endResetModel()` 与视图刷新边界。

- Model 只保存 DTO，不保存 SQL 或数据库连接。

- 页面状态：Loading、Ready、Empty、Error。

- 查询按钮、筛选条件、分页控件和防重复提交。

- `QPointer`、latest-wins 与连续搜索只接受最新结果。

**实施任务：**

- 设计 `ProductTableModel` 的列枚举、数据角色和更新接口。

- 使用 `QAbstractItemModelTester` 或 QTest 验证 Model 基本契约。

- 设计 Product 页面查询流程：构造 filter/pageRequest，调用 Service，更新 Model。

- 处理空结果、错误提示、加载态和重复点击。

- 保持 Presentation 只依赖 application 层接口，不包含 SQL 和 `DatabaseExecutor`。

**验收标准：**

- Model 的行列数、表头、单元格数据和重置行为测试通过。

- 页面查询成功时能显示分页物资列表。

- 查询中页面有明确 Loading 状态，并避免重复请求造成错乱。

- 空数据和错误结果有独立状态，不与正常列表混淆。

- UI、Model 不直接认识 MySQL、`QSqlQuery` 或原始 `DatabaseResult`。