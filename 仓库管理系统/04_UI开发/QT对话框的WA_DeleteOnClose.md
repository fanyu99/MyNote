# QT 对话框的 WA_DeleteOnClose

## BUG 根源
本质是 `WA_DeleteOnClose` 与对象的分配方式冲突产生的 BUG。

## 如何避免
1. **WA_DeleteOnClose 的法则：使用该对象的分配一定要使用堆分配！**

| 使用方式                 | 分配方式   | WA_DeleteOnClose          |
| -------------------- | ------ | ------------------------- |
| `dialog.exec()` 模态   | 一般为栈分配 | 不需要，局部变量自动销毁            |
| `dialog.open()` 非模态  | 必须要堆分配 | 需要，否则内存泄露                |

2. 对话框的构造函数中**不要**使用 `WA_DeleteOnClose`，除非确定只会使用 `new` 进行分配内存。

## 背景案例（Day25）
在每个 dialog 构造函数中写入 `this->setAttribute(Qt::WA_DeleteOnClose)`，同时使用 `InboundEditDialog dialog(...)` 在栈上创建对象，退出函数后栈对象自动销毁，加上 `WA_DeleteOnClose` 的 `deleteLater()` 造成双重释放，导致崩溃。

- 相关日记：[[2026-09-11 Day25 完成InventoryPage及其基础板块的编写]]
- 相关知识点：[[QT对象树的注意事项]]
