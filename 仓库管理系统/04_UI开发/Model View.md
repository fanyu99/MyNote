#相关: 
[[C++_Qt_MySQL_仓库管理系统_学习计划#阶段 1 Qt 核心技术补齐 (Day 1-4)]]
[[C++_Qt_MySQL_仓库管理系统_学习计划#1.1 QAbstractTableModel 深入 (Day 1, ~3h)]]
[[2026-07-28 Day12 ProductTableModel与列表页]]
# QT 中的重要基础

***QAbstractTableModel深入***
-  `QAbstractTableModel` 必须重写的纯虚函数：`rowCount()`, `columnCount()`, `data()`

-  可选重写：`headerData()`, `flags()`, `sort()`, `roleNames()`

- `Qt::ItemDataRole` 的理解（`DisplayRole`, `ForegroundRole`, `BackgroundRole`, `UserRole`）

- 通知视图刷新：`beginResetModel()` / `endResetModel()`

-  增量通知：`beginInsertRows()` / `endInsertRows()`, `beginRemoveRows()` / `endRemoveRows()`

-  `QTableView` 的基本配置：交替行颜色、列宽模式、选择行为
***Qt::ItemDataRole***

| 枚举                   | 值       | 期望类型                      | 用途                                           |
| -------------------- | ------- | ------------------------- | -------------------------------------------- |
| `Qt::DisplayRole`    | 0       | `QString`                 | **视图默认读的显示文本**，QLabel 那种纯文字                  |
| `Qt::EditRole`       | 2       | `QString`/`int`/`double`… | 编辑器中初始化用的值，常与 DisplayRole 同值，但可返回"未格式化的原始数据" |
| `Qt::DecorationRole` | 1       | `QIcon`/`QPixmap`         | 项前的图标（QListView/QTreeView 左边小图标）             |
| `Qt::ToolTipRole`    | 3       | `QString`                 | 鼠标悬停 tooltip                                 |
| `Qt::StatusTipRole`  | 4       | `QString`                 | 项被选中时状态栏显示的提示                                |
| `Qt::WhatsThisRole`  | 5       | `QString`                 | Shift+F1 "这是什么"帮助文本                          |
| ......               | ....... | .........                 | ..........                                   |
***相关示例代码:***
```C++
//必须重载: 返回行数
int StudentTableModel::rowCount(const QModelIndex& parent) const
{
    return m_students.size();
}
//必须重载: 返回列数
int StudentTableModel::columnCount(const QModelIndex& parent) const
{
    return static_cast<int>(Column::ExamDate) + 1;
}
// 必须重载: 返回数据(QVariant)
QVariant StudentTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const Student& stu = m_students[index.row()];
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (static_cast<Column>(index.column())) {
        case Column::Id:
            return stu.id;
        case Column::Student_Name:
            return stu.name;
        case Column::Subject:
            return stu.subject;
        case Column::Score:
            return stu.score;
        case Column::ExamDate:
            return stu.exam_date;
        default:
            return {};
        }
    }
    return {};
}
// 重写: 设置数据setData(const QModelIndex& index, const QVariant& value, int role)
// index结构体 value值 role角色()
bool StudentTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;
    Student& stu = m_students[index.row()];
    switch (static_cast<Column>(index.column())) {
    case Column::Student_Name:
        stu.name = value.toString();
        break;
    case Column::Subject:
        stu.subject = value.toString();
        break;
    case Column::Score:
        stu.score = value.toInt();
        break;
    case Column::ExamDate:
        stu.exam_date = value.toString();
        break;
    default:
        return false;
    }
    // 一定要释放信号: 通知View更新数据
    emit dataChanged(index, index, { role });
    return true;
}
// 重写: headerData(int section, Qt::Orientation orientation, int role):
QVariant StudentTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (static_cast<Column>(section)) {
        case Column::Id:
            return "ID";
        case Column::Student_Name:
            return "姓名";
        case Column::Subject:
            return "科目";
        case Column::Score:
            return "成绩";
        case Column::ExamDate:
            return "考试日期";
        default:
            return {};
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
// 用于控制表格中的每个单元格的交互行为:
// 默认: 可交互(Qt::ItemIsEnabled)
// 可编辑(Qt::ItemIsEditable)
// 可选中(Qt::ItemIsSelectable)
// 可拖动(Qt::ItemIsDragEnabled)
// 可拖动并放置(Qt::ItemIsDropEnabled)
Qt::ItemFlags StudentTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    //设置ID不可编辑
    if (index.column() == static_cast<int>(Column::Id)) {
        return QAbstractTableModel::flags(index);
    }
    //其他列在此基础上添加可编辑标志ItemIsEditable
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
// 增量通知 beginInsertRows,removeInsertRows等
void StudentTableModel::addStudent(const Student& stu)
{
    beginInsertRows(QModelIndex(), m_students.size(), m_students.size());// 开始增加行
    m_students.append(stu);
    endInsertRows();//结束增加行
}
```
