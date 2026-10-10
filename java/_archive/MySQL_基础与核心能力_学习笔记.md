# MySQL 基础与核心能力｜学习笔记

> 学习主题：MySQL 是什么、能解决哪些问题、SQL 基础、Java 后端中的作用，以及初学阶段应该学到什么程度。
>
> 核心认识：**SQL 语法可以按需查阅，但数据库设计、索引、事务和并发一致性的思想必须掌握。**

## 一、MySQL 是什么

- **MySQL**：关系型数据库管理系统（RDBMS），负责持久化存储和管理结构化数据。
- **SQL**：与数据库交互的结构化查询语言。MySQL 是数据库软件，SQL 是操作数据库的语言。
- 数据层次：`MySQL 实例 → 数据库（Database）→ 表（Table）→ 行（Row）与列（Column）`。
- 一个应用通常有多张表，例如 `user`（用户）、`product`（商品）、`orders`（订单）、`order_item`（订单明细）。

### 与 Java 的类比

| Java | MySQL | 说明 |
| --- | --- | --- |
| `class User` | `user` 表 | 结构上的近似类比，不是严格一一对应 |
| `User` 对象 | 一行记录 | 表中的一条用户数据 |
| 成员变量 | 列 | 数据字段 |
| `List<User>` | 查询结果的多行记录 | 查询出的数据集合 |

Java 后端通常按下面的路径访问数据库：

```text
浏览器/客户端
    ↓
Controller（接收请求）
    ↓
Service（业务逻辑）
    ↓
Mapper / Repository（数据访问）
    ↓
MySQL（持久化存储）
```

Spring Boot、JDBC、MyBatis、MyBatis-Plus 等只是连接和访问数据库的不同方式，**不代表可以不懂 SQL**。

## 二、MySQL 能做到哪些事

| 能力 | 典型应用 | 相关知识 |
| --- | --- | --- |
| 持久化存储 | 保存用户、订单、评论；服务重启后数据还在 | 建库、建表、类型 |
| 增删改查 | 注册账号、编辑信息、删除评论、查订单 | `INSERT` / `DELETE` / `UPDATE` / `SELECT` |
| 条件查询 | 按时间、价格、关键词筛选 | `WHERE`、`LIKE`、范围条件 |
| 排序与分页 | 最新评论、订单列表 | `ORDER BY`、`LIMIT` |
| 统计分析 | 用户数、总消费、商品销量 | `COUNT`、`SUM`、`AVG`、`GROUP BY` |
| 管理数据关联 | 用户和订单、订单和商品 | 主键、外键概念、`JOIN` |
| 约束数据合法性 | 用户名不重复、关键字段不能为空 | `PRIMARY KEY`、`UNIQUE`、`NOT NULL` |
| 事务处理 | 创建订单 + 扣库存需要协调成功或失败 | `COMMIT`、`ROLLBACK`、ACID |
| 并发控制 | 多人抢购最后一件商品 | 事务隔离、锁、MVCC |
| 查询优化 | 在百万行数据中快速定位用户 | 索引、B+ 树、`EXPLAIN` |
| 安全管理 | 不同数据库账号有不同权限 | `GRANT`、用户与权限 |
| 备份、恢复、复制 | 故障恢复、容灾、读写分离 | 备份、binlog、复制（进阶） |

> MySQL 是**数据管理与一致性系统**，不负责全部业务功能。例如 HTTP 接口、JWT 登录校验、WebSocket 通信主要由 Java 应用及其框架负责。

## 三、基础 SQL：要能自己写

以下示例在 MySQL 中执行。

### 1. 数据库和表

```sql
CREATE DATABASE java_app;
USE java_app;

CREATE TABLE user (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    age INT,
    email VARCHAR(100),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

- `PRIMARY KEY`：主键，唯一标识一行，并且不允许 `NULL`。
- `AUTO_INCREMENT`：自动分配递增的整数值；**不保证编号连续**。
- `NOT NULL`：字段不能为空。
- `UNIQUE`：限制值不能重复（MySQL 允许唯一索引中的多个 `NULL`，需结合业务考虑）。

### 2. CRUD（增删改查）

```sql
-- C：新增
INSERT INTO user (username, age, email)
VALUES ('Roger', 20, 'roger@example.com');

-- R：查询
SELECT id, username, age FROM user WHERE age >= 18;

-- U：更新
UPDATE user SET age = 21 WHERE username = 'Roger';

-- D：删除
DELETE FROM user WHERE username = 'Roger';
```

**警惕：** `UPDATE`、`DELETE` 不加 `WHERE`，可能操作整张表的所有行。

### 3. 条件查询、排序与分页

```sql
-- 多条件
SELECT * FROM user
WHERE age >= 18 AND username = 'Roger';

-- 按年龄降序
SELECT * FROM user ORDER BY age DESC;

-- 第一页，每页 10 条
SELECT * FROM user ORDER BY id LIMIT 0, 10;

-- 第二页
SELECT * FROM user ORDER BY id LIMIT 10, 10;
```

- `ASC`：升序；`DESC`：降序。
- `LIMIT offset, count`：跳过 `offset` 行，再取 `count` 行。
- 分页最好搭配**稳定的排序条件**，否则结果顺序可能不确定；大偏移量分页可能变慢。

### 4. NULL

`NULL` 表示未知或缺失，**不等于 `0`、空字符串 `''` 或字符串 `'NULL'`**。

```sql
SELECT * FROM user WHERE age IS NULL;
SELECT * FROM user WHERE age IS NOT NULL;
```

不要写 `age = NULL` 来判断空值。

### 5. 聚合与分组

假设有订单表 `orders(id, user_id, price)`：

```sql
-- 订单数
SELECT COUNT(*) FROM orders;

-- 订单均价
SELECT AVG(price) FROM orders;

-- 各用户累计消费
SELECT user_id, SUM(price) AS total_spent
FROM orders
GROUP BY user_id;
```

常见聚合函数：`COUNT`、`SUM`、`AVG`、`MAX`、`MIN`。

### 6. 多表关联 JOIN

`user.id` 与 `orders.user_id` 在业务上建立关联：

```sql
SELECT u.username, o.price
FROM user AS u
INNER JOIN orders AS o ON u.id = o.user_id;
```

- `INNER JOIN`：只保留能够匹配的记录。
- `LEFT JOIN`：保留左表全部记录，右表不匹配时结果为 `NULL`。

```sql
-- 包括没有订单的用户
SELECT u.id, u.username, o.id AS order_id
FROM user AS u
LEFT JOIN orders AS o ON u.id = o.user_id;
```

## 四、常见字段类型及 Java 对应

| MySQL 类型 | 常见 Java 类型 | 典型用途 |
| --- | --- | --- |
| `INT` | `Integer` | 数量、年龄 |
| `BIGINT` | `Long` | 主键 ID |
| `VARCHAR(n)` | `String` | 用户名、标题 |
| `TEXT` | `String` | 较长文本 |
| `DECIMAL(10,2)` | `BigDecimal` | 金额，避免二进制浮点误差 |
| `DATE` | `LocalDate` | 日期 |
| `DATETIME` | `LocalDateTime` | 日期时间 |
| `TIMESTAMP` | 需结合驱动时区配置处理 | 时间戳型数据 |
| `BOOLEAN` | `Boolean` | MySQL 中一般是 `TINYINT(1)` 的别名 |

`DECIMAL(10,2)` 表示总精度 10 位，其中小数 2 位。金额计算建议在 Java 使用 `BigDecimal`。

## 五、超越语法：MySQL 真正要理解的知识

### 1. 数据库设计

需要判断：

- 一个实体应不应该单独建表？
- 用户与订单是“一对多”，订单与商品为什么通常需要订单明细表？
- 哪些字段必须非空？哪些字段应唯一？
- 如何选择主键、关联字段与数据类型？

例如：

```text
user（用户）       1 ── N  orders（订单）
orders（订单）    1 ── N  order_item（订单明细）
product（商品）   1 ── N  order_item（订单明细）
```

### 2. 索引：为什么查询快或慢

```sql
CREATE INDEX idx_username ON user(username);
```

- 没有合适索引的查询可能发生**全表扫描**。
- InnoDB 常见索引底层采用 **B+ 树**，可显著缩小查找范围，但不是所有查询都一定走索引。
- 索引也有代价：占空间，插入/更新/删除需要维护索引。
- 后续重点：聚簇索引、二级索引、联合索引、最左前缀、回表、覆盖索引、`EXPLAIN`。

### 3. 事务：一组操作的一致性

银行转账：A 扣 100，B 加 100，不能只完成一半。

```sql
START TRANSACTION;
UPDATE account SET balance = balance - 100 WHERE id = 1;
UPDATE account SET balance = balance + 100 WHERE id = 2;
COMMIT;

-- 如果业务检查失败或出错，应主动执行：
-- ROLLBACK;
```

事务强调 **ACID**：

- **Atomicity（原子性）**：一组操作作为整体提交或回滚。
- **Consistency（一致性）**：事务前后满足定义的约束和业务规则。
- **Isolation（隔离性）**：并发事务之间的影响受隔离级别约束。
- **Durability（持久性）**：成功提交的数据在系统故障后仍具有持久性保障。

**关键纠正：** 数据库不会自动保证“任意业务 SQL 写错了就全部回滚”。需要正确的事务边界、错误处理、业务校验与回滚机制，并使用支持事务的表引擎（如 InnoDB）。

在 Spring 中常会使用 `@Transactional` 管理事务，但仍需理解其生效条件和回滚规则。

### 4. 并发：多人同时修改数据

假设库存只剩一件，两人同时下单。仅仅在 Java 中“先查询库存，再分别扣减”可能出现超卖风险。

一种基础思路是**条件更新**：

```sql
UPDATE product
SET stock = stock - 1
WHERE id = 1001 AND stock > 0;
```

应用还应检查受影响行数是否为 1，进一步将订单创建等操作纳入正确的事务流程。进阶再学习锁、隔离级别、MVCC 与死锁。

## 六、结合 Java 后端如何理解

实际业务示例：用户打开“我的订单”。

```text
GET /orders
    ↓
Controller 接收请求
    ↓
Service 识别当前用户、执行业务规则
    ↓
Mapper 查询当前用户的订单
    ↓
SELECT * FROM orders WHERE user_id = ? ORDER BY id DESC;
    ↓
将结果映射成 Java 对象，返回给前端
```

- **JDBC**：Java 操作关系型数据库的基础 API。
- **MyBatis**：通过映射配置/注解等方式，将 SQL 查询结果与 Java 对象关联。
- **Spring Boot**：组织 Web 接口、业务层、数据访问层和配置。
- **MyBatis-Plus**：能减少常见 CRUD 的样板代码，但复杂查询、优化和数据正确性仍离不开数据库知识。

## 七、学习策略：哪些要背，哪些查文档

| 优先级 | 内容 | 学习要求 |
| --- | --- | --- |
| 必须熟练 | CRUD、`WHERE`、`JOIN`、`GROUP BY`、`ORDER BY`、`LIMIT` | 能独立手写常见 SQL |
| 必须理解 | 主键、唯一约束、表间关系、索引用途、事务与并发风险 | 能解释用途，能进行简单设计 |
| 建议动手 | `CREATE TABLE`、`CREATE INDEX`、事务、`EXPLAIN` | 至少自己操作几次 |
| 按需查阅 | 少用的函数、特殊日期函数、不常见 SQL 语法 | 不必死记硬背 |
| 项目进阶 | B+ 树、联合索引、MVCC、锁、undo/redo log、binlog、SQL 优化 | 结合实际项目逐步深入 |

**推荐学习路径：**

```text
基础 SQL → 表设计与 JOIN → 索引基础 → 事务基础
           ↓
     Spring Boot + MyBatis 项目实践
           ↓
     深入索引、MVCC、锁、日志、优化
```

不需要等 MySQL 底层全部学完再开始 Spring。

## 八、建议练习：做一个简易订单数据库

设计四张表：

1. `user`：用户 ID、用户名、注册时间。
2. `product`：商品 ID、名称、单价、库存。
3. `orders`：订单 ID、用户 ID、下单时间、状态。
4. `order_item`：明细 ID、订单 ID、商品 ID、购买数量、成交单价。

完成以下练习：

- 插入 5 个用户、10 个商品和多条订单。
- 查询某用户的所有订单（按时间倒序）。
- 分页查询订单，每页 10 条。
- 查询每个用户的订单数量和总消费。
- 查询没有下过订单的用户（`LEFT JOIN`）。
- 为常用查询创建适当索引，并用 `EXPLAIN` 观察查询计划。
- 尝试在事务中创建订单并扣减库存，模拟失败后回滚。
- 使用带 `stock > 0` 条件的更新，理解如何避免库存被直接扣成负数。

## 九、常见误区与自检

**误区 1：SQL 不用背，MySQL 就没有学习难度。**

语法可以查询，但设计错误、慢查询、并发超卖、事务边界错误，不能只靠查单条 SQL 解决。

**误区 2：学会 MyBatis 就不需要 SQL。**

MyBatis 负责简化数据库访问，不会代替对数据库执行计划、索引、事务的理解。

**误区 3：加索引一定更快。**

索引需结合查询条件、数据量、选择性、维护成本判断。

**误区 4：使用事务就能自动解决一切并发问题。**

事务还涉及隔离级别、锁、SQL 条件、检查受影响行数，以及跨系统一致性等问题。

### 自检问题

1. MySQL 与 SQL 有什么区别？
2. `UPDATE` 不带 `WHERE` 有什么风险？
3. `NULL` 和 `0` 有什么区别？
4. `INNER JOIN` 与 `LEFT JOIN` 的区别是什么？
5. 索引有什么好处和成本？
6. 为什么转账需要事务？事务失败时如何处理？
7. 两人同时购买最后一件商品，为什么可能出现超卖？
8. 为什么 SQL 语法可以按需查，而表设计和事务思想必须理解？

---

**一句话总结：MySQL 不难入门，难在用它设计出正确、高效、能承受并发访问的数据系统。**
