---
title: InboundPage
type: 通用笔记
topic: 项目实战
status: 已完成
created: 2026-08-10
tags:
  - wms
  - qt
  - mysql
  - 实施记录
series: WMS 实施记录
canvas: 实施记录/白板/UI模块/Day19 InboundPage.canvas
---

# InboundPage

> 在入库事务和基础资料服务稳定后，准备入库页面的数据联动和真实 Repository 接入。

[[项目实战/计划/优化版详细实施计划]]

**目标：** 在入库事务和基础资料服务稳定后，准备入库页面的数据联动和真实 Repository 接入。

**重点任务：**

- 梳理入库页面所需 DTO：仓库、物资、单位、分类、草稿订单、明细行。

- 设计入库草稿创建、明细编辑、确认入库和结果刷新流程。

- 明确页面层只调用 Service，不直接接触 Repository 或 SQL。

- 为入库页面准备 Fake Service/Fake Repository 测试数据。

**验收标准：**

- 页面数据流和 Service 调用边界清晰。

- 入库确认成功后能刷新订单状态、库存变化和操作结果提示。

- 后续可以平滑接入真实 MySQL Repository。