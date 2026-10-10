# 32 · MySQL 基础

> 一句话概括：MySQL 是关系型数据库管理系统，SQL 是操作它的语言——语法可以现查，但表设计、索引、事务和并发一致性的思想必须真正理解。

## 一、定位与层次

- **MySQL**：RDBMS，持久化存储和管理结构化数据；**SQL**：与数据库交互的语言。
- 层次：`实例 → 数据库 → 表 → 行与列`。一个应用多张表（user / product / orders / order_item）。
- 与 Java 的类比：`class User` ↔ `user` 表；`User` 对象 ↔ 一行；成员变量 ↔ 列；`List<User>` ↔ 多行结果（近似类比，非一一对应）。

后端访问路径：`Controller → Service → Mapper → MySQL`；Spring Boot/JDBC/MyBatis 只是不同连接方式，**不代表可以不懂 SQL**。

## 二、基础 SQL（必须能手写）

```sql
CREATE DATABASE java_app;
USE java_app;

CREATE TABLE user (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    age INT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO user (username, age) VALUES ('Roger', 20);          -- 增
SELECT id, username FROM user WHERE age >= 18 ORDER BY id LIMIT 0, 10;  -- 查+排序+分页
UPDATE user SET age = 21 WHERE username = 'Roger';              -- 改
DELETE FROM user WHERE username = 'Roger';                      -- 删
```

- `UPDATE/DELETE` 忘写 `WHERE` 会动整表。
- `NULL` 表示未知，**不等于 0、'' 或 'NULL'**；判断空值用 `IS NULL` / `IS NOT NULL`，不能写 `= NULL`。
- 聚合：`COUNT/SUM/AVG/MAX/MIN` + `GROUP BY`。
- 关联：`INNER JOIN` 只留匹配行；`LEFT JOIN` 保留左表全部，右表不匹配补 NULL。
- 分页 `LIMIT offset, count` 要配稳定排序，大偏移会变慢。

常见类型与 Java 对应：`INT→Integer`、`BIGINT→Long`、`VARCHAR→String`、`DECIMAL→BigDecimal`（金额，避免浮点误差）、`DATETIME→LocalDateTime`。

## 三、超越语法的四块核心

**1. 数据库设计**：实体该不该建表？一对多怎么表达（用户 1─N 订单，订单 1─N 明细）？哪些字段 NOT NULL / UNIQUE？主键、外键、类型怎么选？

**2. 索引**：没有索引全表扫描；InnoDB 索引底层是 B+ 树。索引有代价——占空间、写入要维护。**加索引不一定更快**，要结合查询条件、数据量、选择性。

**3. 事务（ACID）**：原子性（整体提交或回滚）、一致性（满足约束与业务规则）、隔离性（并发互相影响的程度）、持久性（提交后故障不丢）。

```sql
START TRANSACTION;
UPDATE account SET balance = balance - 100 WHERE id = 1;
UPDATE account SET balance = balance + 100 WHERE id = 2;
COMMIT;   -- 出错则 ROLLBACK
```

关键纠正：数据库**不会**自动保证"业务 SQL 写错就全部回滚"——要正确的事务边界、错误处理、业务校验，且引擎要支持事务（InnoDB）。Spring 里用 `@Transactional`（生效条件与回滚规则见 `25`）。

**4. 并发**：库存只剩一件、两人同时下单，"先 SELECT 再 UPDATE"会超卖。基础解法——**原子条件更新**：

```sql
UPDATE product SET stock = stock - 1 WHERE id = 1001 AND stock > 0;
```

应用检查受影响行数是否为 1，并把订单创建纳入同一事务。进阶再学锁、隔离级别、MVCC、死锁。

## 四、一次查询的底层旅程（了解即可）

```text
Java → MyBatis → JDBC → Driver → TCP → MySQL Server
  → SQL 解析（词法/语法/语义/权限）→ 优化器（"我要什么" vs "怎么拿"）
  → InnoDB（真正读写数据）→ Buffer Pool（内存命中直接读，未命中先读盘入池）
  → B+Tree 索引定位 → 结果原路返回映射成 Java 对象
```

UPDATE 额外走：B+Tree 找到记录 → 改 Buffer Pool 数据页 → 写 **Undo Log**（回滚/MVCC，存修改前版本）→ 写 **Redo Log**（持久性/崩溃恢复）→ COMMIT → 脏页后续刷盘。客户端工具推荐 **DBeaver**（免费通用）+ 命令行，但 SQL 要能手写。

## 五、学习策略

| 优先级 | 内容 |
|---|---|
| 必须熟练 | CRUD、WHERE、JOIN、GROUP BY、ORDER BY、LIMIT |
| 必须理解 | 主键/唯一约束、表关系、索引用途、事务与并发风险 |
| 建议动手 | CREATE TABLE/INDEX、事务、EXPLAIN |
| 按需查阅 | 少用函数、特殊语法 |
| 项目进阶 | B+ 树、联合索引、MVCC、锁、binlog、SQL 优化 |

练习：建 user/product/orders/order_item 四张表，完成插入、按用户查订单倒序、分页、聚合统计、`LEFT JOIN` 查无订单用户、建索引看 EXPLAIN、事务内下单扣库存并回滚。

## 📌 易错点

- "学会 MyBatis 就不用 SQL"——框架简化访问，不替你懂执行计划和事务。
- `AUTO_INCREMENT` 不保证编号连续。
- 唯一索引里多个 `NULL` 是允许的（MySQL）。
- 用 `= NULL` 判断空值——永远查不到。
- "用事务就自动解决并发"——还要隔离级别、锁、条件更新、检查影响行数。

## 🧠 速记口诀

```text
NULL 是未知不是零，IS NULL 来判断
UPDATE DELETE 忘 WHERE，整表数据两行泪
INNER 留匹配，LEFT 留左表
索引 B+ 树提速，写入维护有成本
超卖靠原子条件更新 + 事务，不靠先查后改
```

## ✅ 自测问题

1. MySQL 和 SQL 有什么区别？
2. `UPDATE` 不带 `WHERE` 有什么风险？`NULL` 和 `0` 有何区别？
3. `INNER JOIN` 和 `LEFT JOIN` 的区别？
4. 为什么转账需要事务？事务失败怎么处理？
5. 两人同时买最后一件商品为什么会超卖？基础解法是什么？

> 来源：原始《MySQL 基础与核心能力｜学习笔记》(MySQL_基础与核心能力_学习笔记.md)、《Java 后端学习笔记：Spring、Web 请求处理与数据库设计》(Java后端学习笔记_Spring与Web请求处理_2026-10-08.md §6，数据库设计部分)。
