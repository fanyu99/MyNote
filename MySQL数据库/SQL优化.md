# SQL 优化

## 插入数据优化

按主键顺序进行插入。

### 小数据量：Insert 优化

- 批量插入
- 手动提交多个 INSERT 语句（分批次但统一提交）

### 大数据量：Load 优化

```sql
-- 开启 local_infile
SET GLOBAL local_infile = 1;

-- 使用 LOAD DATA 导入
LOAD DATA LOCAL INFILE '路径'
INTO TABLE 表名
FIELDS TERMINATED BY '...'
LINES TERMINATED BY '...';
```

## 主键优化

### 原则

1. 尽量减少主键长度
2. 插入数据尽量顺序插入（使用自增主键）
3. 减少使用自然（随机）的 ID 做主键（如身份证）
4. 减少修改主键

按主键顺序进行插入，乱序插入会产生**页分裂**现象。

### 页分裂

原本：乱序插入下，50 号数据无法插入到 P1 中（已满），只能分裂出新页，并将原本 P1 的 50% 数据转移到新页，重新定向指针。

![页分裂1](SQL优化_img1.png)

![页分裂2](SQL优化_img2.png)

### 页合并

![页合并](SQL优化_img3.png)

## 排序优化

### Order By 优化

尽量使用有序的索引：

- **Using filesort**：通过全表扫描 / 表索引读取数据，再进行排序。所有没经过索引的排序都是 filesort
- **Using index**：通过有序索引扫描直接返回数据，效率高

![排序优化](SQL优化_img4.png)

> **注意：**
> - 前提：使用覆盖索引，遵循最左前缀法则
> - 排序顺序尽量与索引对应的排序顺序一致
> - 例：索引 collation 为 A，而 `ORDER BY DESC, ASC` 那么会进行两次扫描，将会混合使用 index、filesort

**示例：**

![排序示例](SQL优化_img5.png)

### Group By 优化

- 选择 / 创建合适的索引
- 分组时分组的字段索引的使用需要满足最左前缀法则

## Count 优化

在记录总数时尽量选用 `COUNT(*)` / `COUNT(1)`，不取值。

## Limit 优化

通过创建覆盖索引和联合子查询进行优化。

![Limit优化](SQL优化_img6.png)

## Update 优化

尽量通过索引进行事务，且保证索引有效（索引会添加行锁而非表锁）。

否则会进行表锁，导致并发阻塞。

![Update优化](SQL优化_img7.png)

> **相关主题**：[[索引优化]]、[[InnoDB]]、[[锁]]、[[为什么使用B+树而不使用其他数据结构]]
