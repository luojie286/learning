# 30 · JDBC 与数据库访问

> 一句话概括：JDBC 是 Java 访问关系型数据库的标准 API——连接、参数化 SQL、游标结果集、事务、资源释放，全部围绕 `Connection → PreparedStatement → ResultSet` 三件套。

## 一、定位与链路

```text
Java 业务代码 → JDBC API → MySQL JDBC 驱动 → TCP/MySQL 协议 → MySQL 服务器
```

| 对象 | 作用 |
|---|---|
| `DriverManager` | 获取数据库连接 |
| `Connection` | 一条数据库连接；创建执行对象、管理事务 |
| `PreparedStatement` | 执行带参数的 SQL |
| `ResultSet` | 逐行读取查询结果（游标） |
| `SQLException` | JDBC/数据库访问异常 |

现代驱动自动注册，不必手动 `Class.forName(...)`。Maven 加 `mysql-connector-j` 依赖（版本需与 JDK/MySQL 核实兼容）。

## 二、核心流程与参数化

```java
String url = "jdbc:mysql://localhost:3306/demo";
Connection conn = DriverManager.getConnection(url, "root", password);

String sql = "SELECT * FROM user WHERE username = ? AND age = ?";
PreparedStatement ps = conn.prepareStatement(sql);  // 只创建，不执行
ps.setString(1, "Tom");      // 参数编号从 1 开始！
ps.setInt(2, 20);
ResultSet rs = ps.executeQuery();
```

方法选择：`SELECT → executeQuery()` 返回 `ResultSet`；`INSERT/UPDATE/DELETE → executeUpdate()` 返回**影响行数**（不是自增 ID，要 ID 需 `RETURN_GENERATED_KEYS` + `getGeneratedKeys()`）。

## 三、ResultSet 游标

`executeQuery()` 后游标在第一行**之前**，必须先 `next()` 才能读。`next()` 每次只移动一行：移到有效行返回 true，没有下一行返回 false。

```java
if (rs.next()) { /* 至多处理第一条 */ }
while (rs.next()) { /* 逐行处理全部 */ }
```

## 四、SQL 注入与参数化原理

拼接用户输入会让数据变成 SQL 结构：

```java
String sql = "SELECT * FROM user WHERE username = '" + username + "'";
// 输入 ' OR '1'='1  →  WHERE username = '' OR '1'='1'  逻辑被改写
```

参数化的核心：**让用户输入始终保持为数据值，而不是 SQL 可执行结构**。实现有两类——服务端预处理（MySQL 协议 `COM_STMT_PREPARE/EXECUTE`）或驱动端模拟预处理（安全编码转义后发送）。所以"防注入"不等于"没有字符串拼接"，关键是输入始终被正确的数据边界约束。

**参数化解决不了的**：动态表名/列名/排序方向（`ORDER BY ?` 不能当列名）——必须用**白名单**映射；部分拼接仍危险；下游存储过程再次拼接也危险。防注入也不代替认证与权限控制。

## 五、事务与资源管理

```java
try {
    conn.setAutoCommit(false);   // 关闭自动提交
    // ... 多条 SQL ...
    conn.commit();
} catch (SQLException e) {
    conn.rollback();             // 出错回滚
    throw e;
}
```

资源必须关闭，用 try-with-resources 自动关（`Connection/PreparedStatement/ResultSet` 都实现 `AutoCloseable`）。注意：连接池环境下 `close()` 通常是**归还连接**而非真断开；事务代码归还连接前要恢复连接状态，且表引擎要支持事务（InnoDB）。

```java
try (Connection conn = DriverManager.getConnection(url, user, pwd);
     PreparedStatement ps = conn.prepareStatement(sql)) {
    // ...
}
```

JDBC 与 MyBatis、Spring Boot 的关系：JDBC 是底层 API；MyBatis 在其上封装映射与模板代码；Spring Boot 负责整合数据源、连接池（HikariCP）与事务。学习顺序：手写一次完整 JDBC 查询 → CRUD → 事务 → 再进 MyBatis（见 `31`）。

## 📌 易错点

- `prepareStatement()` 不执行 SQL，执行在 `executeQuery/Update()`。
- 参数编号从 **1** 开始，不是 0。
- `if (rs.next())` 只处理第一条，遍历要用 `while`。
- `executeUpdate()` 返回影响行数，不是主键。
- `UPDATE/DELETE` 忘写 `WHERE` 会操作整表。
- 密码哈希才是密码该用的（见 `34`），登录查询别比对明文。

## 🧠 速记口诀

```text
连接管理三兄弟：DriverManager 拿连接、Connection 管事务、ResultSet 是游标
查 query 增删改 update，返回影响行数
参数从 1 开始，next 一次走一行
注入防的是"数据变结构"，动态列名走白名单
打开资源 try-with-resources，池里 close 是归还
```

## ✅ 自测问题

1. `Connection`、`PreparedStatement`、`ResultSet` 各负责什么？
2. `ps.setString(2, "Tom")` 里的 `2` 是什么含义？为什么不是 0 开始？
3. 为什么 `executeQuery()` 后不能立刻 `getString()`？
4. 参数化为什么能阻止 `' OR '1'='1`？`ORDER BY` 为什么不能参数化？
5. `setAutoCommit(false)` / `commit()` / `rollback()` 各自做什么？

> 来源：原始《JDBC 学习笔记：基本语法、结果集与 SQL 注入》(JDBC学习笔记_基本语法结果集与SQL注入.md)、《JDBC 与 MyBatis 学习笔记》(JDBC与MyBatis学习笔记.md §1-4)、《Java 后端基础学习笔记》(Java后端基础_本次聊天学习笔记.md §1)，重叠部分已去重合并。
