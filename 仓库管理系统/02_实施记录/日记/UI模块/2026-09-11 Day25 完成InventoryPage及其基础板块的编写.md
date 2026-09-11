# Day25 完成 InventoryPage 及其基础板块

## 完成内容

### 1. InventoryRepository → InventoryService → InventoryPage 功能闭环
Inventory 板块的架构：
- **InventoryRepository**：负责从数据库获取数据
- **InventoryService**：负责校验并派发任务给 repository 层
- **InventoryPage**：负责 UI 展示

其中编写了 `InventoryTableModel` 以及 `InventoryMovementsDetailDialog` 用于展示库存的流水记录。

### 2. 修复 outbound 的一个 bug
在 repository 层创建任务时，写入 `audit_logs` 的语句 `statement4` 未加入 task 的 `statements` 中，导致写入审计失败并显示创建失败。

## 出现一个重要且容易触发的 BUG：double free

在每个 dialog 构造函数中写了下面的代码：

```C++
// 关闭弹窗后自动销毁
this->setAttribute(Qt::WA_DeleteOnClose);
```

此时关闭弹窗时，会调用 dialog 类的 `deleteLater()` 函数删除弹窗对象。
但是在使用对象时：

```C++
void InboundPage::onCreateClicked()
{
    // ...
    // 创建编辑框
    InboundEditDialog dialog(InboundEditMode::Create, this); // 在栈上创建了dialog对象
    // ...
    reloadCurrentPage();
}
```

退出函数后，会自动销毁 `dialog` 这个局部栈上的对象，而前面的 `WA_DeleteOnClose` 已经析构了一次，造成了重复释放，导致崩溃！

> 详细分析及避免方法见 [[QT对话框的WA_DeleteOnClose]]
