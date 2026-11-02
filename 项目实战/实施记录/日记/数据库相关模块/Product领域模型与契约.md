---
title: Product领域模型与契约
type: 通用笔记
topic: 项目实战
status: 已完成
created: 2026-07-25
tags:
  - wms
  - qt
  - mysql
  - 实施记录
series: WMS 实施记录
canvas: 实施记录/白板/数据库相关模块/Day10 Product领域模型与契约.canvas
---

# Product领域模型与契约

> 开始物资纵向切片，稳定领域模型、分页请求和 Repository 边界。

[[项目实战/计划/优化版详细实施计划]]
Product Entity、DTO 与 Repository 契约

**目标：** 开始详细计划中的物资纵向切片，先稳定领域模型、分页请求和 Repository 边界。

**知识点：**

- Product Entity、DTO、分页结果和筛选条件的职责。

- Repository Port 与 MySQL Repository 实现的依赖方向。

- 异步 Operation 的 owner、requestId、`QPointer` 和 latest-wins。

- 唯一编码、分类、单位和停用规则应位于哪一层。

- Repository 结果到 `AppError` 的映射。

**实施任务：**

- 定义 Product、ProductFilter、ProductPage 和相关 DTO。

- 设计 `IProductRepository` 的分页、查询和写入契约。

- 明确异步请求对象的所有权和回调生命周期。

- 设计 ProductService 的业务校验和权限边界。

- 编写 Repository 契约与 Fake Repository 测试方案。

**验收标准：**

- Presentation、Service 不依赖 SQL、`QSqlQuery` 或原始数据库行。

- 分页结果包含记录、总数、页码和页大小。

- 物资编码唯一、分类单位有效和停用规则有明确责任层。

- 异步请求不会向已经销毁的 owner 回调。

- 后续 MySQL Repository 可以在不修改上层接口的情况下接入。