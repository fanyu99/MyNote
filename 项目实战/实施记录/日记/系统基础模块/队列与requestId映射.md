---
title: 队列与requestId映射
type: 通用笔记
topic: 项目实战
status: 学习中
created: 2026-07-19
tags:
  - wms
  - qt
  - mysql
  - 实施记录
series: WMS 实施记录
canvas: 实施记录/白板/系统基础模块/Day4 队列与requestId映射.canvas
---

# 队列与requestId映射

> 记录「队列与requestId映射」当天的任务、实现过程与结论。

#超前完成

## 队列容量、requestId、队列满、无效任务、taskFinished 映射

**目标：** 让 `DatabaseExecutor` 在正常提交、非法提交和队列溢出时都有确定行为。

**知识点：**
- 有界队列设计
- `QUuid` 请求 ID 的生成与结果关联
- 在任务派发前拒绝非法任务
- 将 Worker 完成结果映射为 `taskFinished`
- “派发前被拒绝”和“Worker 执行失败”的区别

**实施任务：**
- 确保每个提交的任务都有非空 `requestId`
- 实现并校验队列容量限制
- 队列溢出时返回 `QueueFull`
- 任务结构非法时返回 `InvalidTask`
- 确保 `taskFinished` 总是携带原始请求 ID
- 增加队列边界测试

**验收标准：**
- 队列满行为稳定、可测试
- 无效任务不会进入 Worker
- 每个完成结果都能通过 requestId 对应到原始请求
- 被拒绝的任务不会破坏 Executor 后续运行