# JDBC 学习笔记：基本语法、结果集与 SQL 注入

> 整理范围：本次聊天讨论的 Java SQL 字符串、JDBC 基本用法、`PreparedStatement`、`ResultSet.next()`、增删改查、事务、资源管理以及 SQL 注入防御。

## 1. JDBC 是什么

JDBC（Java Database Connectivity）是 Java 访问关系型数据库的标准 API。Java 程序通过 JDBC 接口和数据库厂商提供的驱动（如 MySQL Connector/J）与数据库通信。

典型链路：

```text
Java 业务代码 → JDBC API → MySQL JDBC 驱动 → TCP/MySQL 协议 → MySQL 服务器
```

常见对象：

| 对象 / 接口 | 作用 |
|---|---|
| `DriverManager` | 获取数据库连接 |
| `Connection` | 表示一个数据库连接，创建 SQL 执行对象、管理事务 |
| `PreparedStatement` | 执行带参数的 SQL |
| `ResultSet` | 逐行读取查询结果 |
| `SQLException` | 表示 JDBC / 数据库访问异常 |

## 2. 从 SQL 字符串拼接开始

```java
String sql = "SELECT * FROM admin WHERE username = '"
    + username + "' AND password = '"
    + password + "'";
```

- `String sql`：声明字符串变量。
- Java 中双引号 `"..."` 定义字符串常量；SQL 中单引号 `'...'` 通常界定字符串值。
- Java 的 `+` 用于字符串拼接。
- `SELECT * FROM admin`：从 `admin` 表查询所有列；`WHERE` 指定条件；`AND` 表示两个条件同时满足。
- 这里只是**生成 SQL 文本**，还没有执行数据库查询。

假设 `username = "admin"`、`password = "123456"`，拼接结果是：

```sql
SELECT * FROM admin WHERE username = 'admin' AND password = '123456'
```

**安全问题：**直接拼接用户输入，可能允许输入内容改变 SQL 的原有结构，引起 SQL 注入。实际登录不应保存或直接比较明文密码，应使用加盐密码哈希（如 BCrypt）并在应用层验证。

## 3. 数据库准备与驱动

示例数据表：

```sql
CREATE DATABASE demo;
USE demo;

CREATE TABLE user (
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL,
    age INT NOT NULL
);
```

使用 Maven 时添加与 Java / MySQL 版本兼容的 MySQL Connector/J 依赖，例如：

```xml
<dependency>
    <groupId>com.mysql</groupId>
    <artifactId>mysql-connector-j</artifactId>
    <version>9.6.0</version>
</dependency>
```

版本号仅是聊天中的示例；实际使用前需核实可用且兼容的版本。现代 JDBC 驱动通常可自动注册，不必手动调用 `Class.forName(...)`。

## 4. 建立连接

```java
String url = "jdbc:mysql://localhost:3306/demo";
String dbUser = "root";
String dbPassword = "123456";

Connection conn = DriverManager.getConnection(url, dbUser, dbPassword);
```

- `jdbc:mysql`：选择 MySQL JDBC 驱动对应的 URL 格式。
- `localhost`：本机数据库服务器地址。
- `3306`：MySQL 常见端口。
- `demo`：数据库名称。
- `DriverManager.getConnection(...)`：静态方法，返回 `Connection` 对象。
- `Connection` 是接口；`conn` 保存实现该接口的连接对象引用。

数据库密码在此仅为演示值；真实项目不应将敏感凭据写死到代码仓库。

## 5. PreparedStatement：参数化查询

```java
String sql = "SELECT * FROM user WHERE username = ? AND age = ?";
PreparedStatement ps = conn.prepareStatement(sql);
ps.setString(1, "Tom");
ps.setInt(2, 20);
ResultSet rs = ps.executeQuery();
```

逐句理解：

1. `?`：**值占位符**，表示待绑定的参数，而不是任意 SQL 语法。
2. `conn.prepareStatement(sql)`：使用连接创建 `PreparedStatement` 对象。
3. `ps.setString(1, "Tom")`：第 **1** 个占位符绑定字符串 `Tom`。
4. `ps.setInt(2, 20)`：第 **2** 个占位符绑定整数 `20`。
5. `ps.executeQuery()`：执行查询，返回 `ResultSet`。

**容易混淆：**JDBC 参数编号从 **1** 开始，不是从 0 开始；`prepareStatement()` 并不等于已经执行查询。

常见参数绑定方法：`setString`、`setInt`、`setLong`、`setDouble`、`setBoolean` 等。

## 6. 增删改查 CRUD

### 6.1 查询 SELECT

```java
String sql = "SELECT id, username, age FROM user WHERE id = ?";
PreparedStatement ps = conn.prepareStatement(sql);
ps.setInt(1, 1);
ResultSet rs = ps.executeQuery();
while (rs.next()) {
    int id = rs.getInt("id");
    String name = rs.getString("username");
    int age = rs.getInt("age");
    System.out.println(id + " " + name + " " + age);
}
```

`getInt("id")`、`getString("username")` 读取**当前行**的字段。也可以通过从 1 开始的列序号读取，但按列名更清晰。

### 6.2 插入 INSERT

```java
String sql = "INSERT INTO user(username, age) VALUES(?, ?)";
PreparedStatement ps = conn.prepareStatement(sql);
ps.setString(1, "Tom");
ps.setInt(2, 20);
int rows = ps.executeUpdate();
```

`executeUpdate()` 常用于 `INSERT / UPDATE / DELETE`，返回影响行数；不是新记录的自增 ID。若需要自增主键，可在创建语句时指定 `Statement.RETURN_GENERATED_KEYS`，执行后调用 `getGeneratedKeys()`。

### 6.3 更新 UPDATE

```java
String sql = "UPDATE user SET age = ? WHERE id = ?";
PreparedStatement ps = conn.prepareStatement(sql);
ps.setInt(1, 25);
ps.setInt(2, 1);
int rows = ps.executeUpdate();
```

**注意：**省略 `WHERE` 可能更新整表。

### 6.4 删除 DELETE

```java
String sql = "DELETE FROM user WHERE id = ?";
PreparedStatement ps = conn.prepareStatement(sql);
ps.setInt(1, 1);
int rows = ps.executeUpdate();
```

**注意：**省略 `WHERE` 可能删除整表数据。

### 方法选择

| SQL | 常用方法 | 返回值 |
|---|---|---|
| `SELECT` | `executeQuery()` | `ResultSet` |
| `INSERT` | `executeUpdate()` | `int`（影响行数） |
| `UPDATE` | `executeUpdate()` | `int`（影响行数） |
| `DELETE` | `executeUpdate()` | `int`（影响行数） |

## 7. ResultSet.next() 为什么必要

`ResultSet` 是具有**游标**的结果集。刚调用 `executeQuery()` 后，游标通常在第一行之前，因此不能立即 `getString(...)`；必须先移动到有效行。

```java
ResultSet rs = ps.executeQuery();
if (rs.next()) {
    System.out.println(rs.getString("username"));
}
```

`next()` **每调用一次只向后移动一行**：成功移动到一条记录返回 `true`；没有下一行时返回 `false`。

假设结果依次为 Tom、Jack、Alice：

| 第几次调用 `next()` | 当前行 | 返回值 |
|---|---|---|
| 1 | Tom | `true` |
| 2 | Jack | `true` |
| 3 | Alice | `true` |
| 4 | 最后一行之后 | `false` |

因此：

```java
if (rs.next()) { /* 至多处理第一条 */ }
while (rs.next()) { /* 逐行处理所有结果 */ }
```

`while` 才会**反复调用** `next()`，不是一次 `next()` 遍历所有行。

`ResultSet` 的逐行读取接口并不保证驱动一定逐行从服务器取数据；是否先缓存结果、是否分批获取，取决于 JDBC 驱动和配置。

## 8. SQL 注入及参数化的原理

不安全示例：

```java
String sql = "SELECT * FROM user WHERE username = '" + username + "'";
```

输入如果为：

```text
' OR '1'='1
```

拼接结果：

```sql
SELECT * FROM user WHERE username = '' OR '1'='1'
```

用户数据中的 `OR` 被解析成 SQL 逻辑，可能改变查询含义。

安全参数化示例：

```java
String sql = "SELECT * FROM user WHERE username = ?";
PreparedStatement ps = conn.prepareStatement(sql);
ps.setString(1, username);
```

核心是：**让用户输入保持为数据值，而不变成 SQL 的可执行结构。**

两类实现思路：

- **服务端预处理**：SQL 模板在服务端准备，参数在执行时绑定；MySQL 协议可使用 `COM_STMT_PREPARE`、`COM_STMT_EXECUTE`。
- **驱动端模拟预处理**：驱动按数据库规则对参数安全编码与转义，然后发送 SQL。安全性依赖驱动正确处理，而不是自己拿 `+` 拼接未经处理的输入。

所以，**防 SQL 注入不等于最终一定没有 SQL 文本拼接**，关键是用户输入是否始终受正确的数据边界约束。

### 参数化也不能解决所有情况

1. **动态表名/列名/排序方向**：`ORDER BY ?` 不能将 `?` 当作任意列名；不要直接拼接用户提供的列名，应使用**白名单**映射。
2. **部分参数化、部分直接拼接**：只绑定 `username`，却拼接 `role`，后者仍有风险。
3. **数据库存储过程或下游代码再次拼接动态 SQL**：即使 Java 层安全绑定，后续重新解释数据仍可能带来注入风险。
4. **其他安全问题**：SQL 注入防护并不能代替身份认证、对象访问权限控制、最小数据库权限等措施。

安全排序示例：

```java
String sortColumn;
if ("age".equals(input)) {
    sortColumn = "age";
} else if ("username".equals(input)) {
    sortColumn = "username";
} else {
    throw new IllegalArgumentException("不支持的排序字段");
}
String sql = "SELECT * FROM user ORDER BY " + sortColumn;
```

这里允许拼接的内容由程序白名单决定，不直接接受任意 SQL 片段。

## 9. 事务：commit() 与 rollback()

事务适合转账、订单与库存更新等需要多条 SQL 保持一致的操作。

```java
try {
    conn.setAutoCommit(false);

    try (PreparedStatement debit = conn.prepareStatement(
             "UPDATE account SET balance = balance - ? WHERE id = ?");
         PreparedStatement credit = conn.prepareStatement(
             "UPDATE account SET balance = balance + ? WHERE id = ?")) {
        debit.setInt(1, 100);
        debit.setInt(2, 1);
        debit.executeUpdate();

        credit.setInt(1, 100);
        credit.setInt(2, 2);
        credit.executeUpdate();
    }

    conn.commit();
} catch (SQLException e) {
    conn.rollback();
    throw e;
}
```

- `setAutoCommit(false)`：关闭自动提交，由程序控制事务。
- `commit()`：提交事务。
- `rollback()`：回滚未提交的事务修改。

**简化示例的局限**：没有检查余额、影响行数与并发安全；生产环境还必须在连接归还连接池前正确恢复连接状态，并确保数据库表和存储引擎支持事务。

## 10. 资源管理：try-with-resources

数据库连接、SQL 执行对象和查询结果均应及时关闭。

```java
String sql = "SELECT username FROM user WHERE id = ?";

try (Connection conn = DriverManager.getConnection(url, dbUser, dbPassword);
     PreparedStatement ps = conn.prepareStatement(sql)) {

    ps.setInt(1, 1);
    try (ResultSet rs = ps.executeQuery()) {
        while (rs.next()) {
            System.out.println(rs.getString("username"));
        }
    }
} catch (SQLException e) {
    e.printStackTrace();
}
```

`try (...)` 是 Java 的 **try-with-resources** 语法，可自动关闭实现 `AutoCloseable` 的资源。关闭 `Connection` 并不是关闭 MySQL 服务；使用连接池时，`close()` 通常意味着归还连接给池。

## 11. JDBC、MyBatis 与 Spring Boot 的关系

- **JDBC**：底层数据库访问 API，负责连接、参数绑定、SQL 执行、结果读取。
- **MyBatis**：在 JDBC 之上封装 SQL 参数映射、执行和结果对象映射，减少手写模板代码。
- **Spring Boot**：负责整合数据源、连接池、MyBatis / JDBC 等组件。Spring 的声明式事务也在更高层封装事务管理。

推荐练习顺序：先手写一次 `Connection → PreparedStatement → ResultSet` 查询，再写完整 CRUD，最后练习事务和 MyBatis。

## 12. 易错点速查

| 易错理解 | 正确理解 |
|---|---|
| `prepareStatement()` 会执行 SELECT | 它创建准备语句对象，通常在 `executeQuery()` 时执行查询 |
| JDBC 参数编号从 0 开始 | `setXxx(1, ...)` 对应第一个 `?` |
| `next()` 一次会访问所有行 | 每次只尝试移动到下一行 |
| `if (rs.next())` 会遍历结果集 | `if` 只做一次判断；`while` 反复调用 |
| `executeUpdate()` 返回插入记录的主键 | 它通常返回影响行数 |
| `PreparedStatement` 能让任何拼接都安全 | 只能保护正确绑定的值；动态 SQL 结构仍需控制 |
| 参数化 SQL 就等于系统安全 | 仍需防越权、保护凭据、控制数据库权限等 |

## 13. 自测题

1. `Connection`、`PreparedStatement`、`ResultSet` 分别负责什么？
2. `ps.setString(2, "Tom")` 中的 `2` 是什么含义？
3. 为什么 `executeQuery()` 后不能保证立即调用 `getString()`？
4. `if(rs.next())` 和 `while(rs.next())` 有什么本质区别？
5. `executeQuery()` 与 `executeUpdate()` 的典型使用场景分别是什么？
6. SQL 参数化为什么能阻止 `' OR '1'='1` 改变查询逻辑？
7. 为什么 `ORDER BY ?` 不能直接替代任意列名？如何安全实现动态排序？
8. `setAutoCommit(false)`、`commit()` 和 `rollback()` 分别做什么？
9. `try-with-resources` 解决什么问题？
10. JDBC 和 MyBatis 的关系是什么？

---

## 自检结论

- **结果：通过，附条件说明。**
- **准确性**：区分了 SQL 字符串拼接、参数化、`ResultSet` 游标与事务方法。
- **边界说明**：JDBC 驱动可能使用服务端或客户端预处理；结果集读取方式取决于驱动配置；参数化不能代替授权检查。
- **示例限制**：数据库凭据、事务转账和示例依赖版本均用于教学，不应直接作为生产配置。
