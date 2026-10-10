# JDBC 与 MyBatis 学习笔记

## 1. JDBC 是什么

JDBC 全称：

```text
Java Database Connectivity
```

即 **Java 数据库连接**。

它解决的核心问题是：

> Java 程序如何连接数据库、执行 SQL、读取查询结果。

整体调用关系：

```text
Java 程序
   ↓
JDBC
   ↓
数据库驱动 Driver
   ↓
MySQL
```

JDBC 本身定义了一套统一 API，不同数据库厂商负责提供对应的驱动实现。

---

## 2. JDBC 的核心流程

使用 JDBC 操作数据库，通常分为四步：

```text
1. 连接数据库
2. 准备 SQL
3. 执行 SQL
4. 处理结果
```

典型查询代码：

```java
String url = "jdbc:mysql://localhost:3306/java_app";
String username = "root";
String password = "123456";

Connection conn = DriverManager.getConnection(
        url,
        username,
        password
);

String sql = "SELECT * FROM user WHERE id = ?";

PreparedStatement stmt = conn.prepareStatement(sql);

stmt.setLong(1, 1);

ResultSet rs = stmt.executeQuery();

if (rs.next()) {
    long id = rs.getLong("id");
    String name = rs.getString("username");
    int age = rs.getInt("age");
}

rs.close();
stmt.close();
conn.close();
```

---

## 3. JDBC 三个核心对象

### 3.1 Connection

`Connection` 代表：

> Java 程序与数据库之间的一条连接。

例如：

```java
Connection conn =
        DriverManager.getConnection(url, username, password);
```

可以理解为：

```text
Java ←──── Connection ────→ MySQL
```

事务也和 `Connection` 有直接关系：

```java
conn.setAutoCommit(false);
conn.commit();
conn.rollback();
```

---

### 3.2 PreparedStatement

`PreparedStatement` 用于执行带参数的 SQL。

例如：

```java
String sql =
        "SELECT * FROM user WHERE username = ?";

PreparedStatement ps =
        conn.prepareStatement(sql);

ps.setString(1, "luojie");
```

其中：

```sql
?
```

表示参数占位符。

```java
ps.setString(1, "luojie");
```

表示给第一个 `?` 绑定字符串参数 `"luojie"`。

推荐使用 `PreparedStatement`，而不是直接拼接 SQL 字符串，因为字符串拼接更容易引入 SQL 注入风险。

---

### 3.3 ResultSet

查询 SQL 执行后：

```java
ResultSet rs = ps.executeQuery();
```

数据库可能返回：

```text
id    username    age

1     luojie      20
2     tom         21
3     jack        22
```

`ResultSet` 就是 JDBC 对查询结果的封装。

遍历：

```java
while (rs.next()) {
    long id = rs.getLong("id");
    String username = rs.getString("username");
    int age = rs.getInt("age");
}
```

`rs.next()` 的作用：

> 将结果集游标移动到下一行。

因此：

```java
while (rs.next())
```

就是逐行读取查询结果。

---

## 4. JDBC 为什么比较麻烦

假设数据库中的表：

```sql
CREATE TABLE user (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50),
    age INT
);
```

Java 实体类：

```java
public class User {
    private Long id;
    private String username;
    private Integer age;
}
```

使用 JDBC 查询后，需要自己完成对象转换：

```java
User user = new User();

user.setId(rs.getLong("id"));
user.setUsername(rs.getString("username"));
user.setAge(rs.getInt("age"));
```

每次数据库操作还需要重复处理：

```text
Connection
PreparedStatement
ResultSet
异常
资源关闭
数据库字段 → Java 对象
```

项目变大后会出现大量重复代码。

因此出现了 MyBatis。

---

# 5. MyBatis 是什么

MyBatis 是一个：

> Java 持久层 / 数据访问层框架。

它最核心的作用是：

> 帮助开发者更方便地执行 SQL，并将数据库结果映射成 Java 对象。

整体调用链：

```text
Java
 ↓
MyBatis
 ↓
JDBC
 ↓
MySQL Driver
 ↓
MySQL
```

非常重要：

> MyBatis 并没有取代 JDBC，而是建立在 JDBC 之上。

底层最终仍然要通过 JDBC 与数据库通信。

---

# 6. JDBC 与 MyBatis 对比

## JDBC 写法

```java
PreparedStatement ps =
        conn.prepareStatement(
                "SELECT * FROM user WHERE id = ?"
        );

ps.setLong(1, id);

ResultSet rs = ps.executeQuery();

User user = new User();

if (rs.next()) {
    user.setId(rs.getLong("id"));
    user.setUsername(rs.getString("username"));
    user.setAge(rs.getInt("age"));
}
```

## MyBatis 写法

```java
User user = userMapper.selectById(1L);
```

MyBatis 会帮助处理大量重复工作：

```text
Connection
PreparedStatement
ResultSet
参数绑定
字段映射
资源关闭
```

---

# 7. MyBatis 的核心：Mapper

示例：

```java
@Mapper
public interface UserMapper {

    @Select("""
        SELECT *
        FROM user
        WHERE id = #{id}
    """)
    User selectById(Long id);

}
```

调用：

```java
User user = userMapper.selectById(1L);
```

背后的过程：

```text
调用 selectById()
        ↓
MyBatis 找到对应 SQL
        ↓
创建 / 使用 JDBC
        ↓
执行 SQL
        ↓
获得 ResultSet
        ↓
数据库数据映射成 User
        ↓
返回 User 对象
```

虽然 `UserMapper` 是接口，没有自己写实现类，但是 MyBatis 会为它生成代理对象并执行对应逻辑。

---

# 8. `#{}` 参数绑定

例如：

```java
@Select("""
SELECT *
FROM user
WHERE id = #{id}
""")
User selectById(Long id);
```

这里：

```java
#{id}
```

表示 SQL 参数。

底层思想类似：

```java
PreparedStatement ps =
        conn.prepareStatement(
                "SELECT * FROM user WHERE id = ?"
        );

ps.setLong(1, id);
```

因此 `#{}` 通常会通过 JDBC 的 `PreparedStatement` 进行参数绑定。

---

# 9. MyBatis 增删改查

## 查询

```java
@Select("""
SELECT *
FROM user
WHERE id = #{id}
""")
User selectById(Long id);
```

## 插入

```java
@Insert("""
INSERT INTO user(username, age)
VALUES(#{username}, #{age})
""")
int insert(User user);
```

## 修改

```java
@Update("""
UPDATE user
SET username = #{username},
    age = #{age}
WHERE id = #{id}
""")
int update(User user);
```

## 删除

```java
@Delete("""
DELETE FROM user
WHERE id = #{id}
""")
int deleteById(Long id);
```

返回的 `int` 通常表示：

> SQL 影响的行数。

---

# 10. MyBatis 对象映射

数据库数据：

```text
id        1
username  luojie
age       20
```

Java：

```java
public class User {
    private Long id;
    private String username;
    private Integer age;
}
```

MyBatis 可以自动完成：

```text
id        → user.id
username  → user.username
age       → user.age
```

即：

```text
数据库的一行
       ↓
Java 对象
```

MyBatis 常被称为：

```text
SQL Mapper 框架
```

或者：

```text
半自动 ORM 框架
```

因为 SQL 通常仍然由开发者自己编写和控制。

---

# 11. 数据库字段与 Java 字段不一致

例如数据库字段：

```text
user_name
```

Java 字段：

```java
userName
```

可以通过驼峰配置自动映射：

```text
user_name
    ↓
userName
```

也可以使用 SQL 别名：

```java
@Select("""
SELECT
    id,
    user_name AS userName
FROM user
""")
List<User> selectAll();
```

---

# 12. MyBatis XML Mapper

除了注解：

```java
@Select
@Insert
@Update
@Delete
```

MyBatis 还支持把 SQL 写到 XML 文件中。

Java：

```java
public interface UserMapper {
    User selectById(Long id);
}
```

XML：

```xml
<select
    id="selectById"
    resultType="User">

    SELECT *
    FROM user
    WHERE id = #{id}

</select>
```

其中：

```text
id="selectById"
```

对应：

```java
selectById()
```

调用：

```java
userMapper.selectById(1L);
```

就会执行 XML 中对应 SQL。

复杂 SQL 中，XML Mapper 仍然非常常见。

---

# 13. 后端项目中的整体调用链

典型 Java Web 项目：

```text
Controller
    ↓
Service
    ↓
Mapper
    ↓
MyBatis
    ↓
JDBC
    ↓
MySQL
```

例如登录：

```text
用户点击登录
        ↓
POST /login
        ↓
LoginController
        ↓
UserService
        ↓
UserMapper
        ↓
MyBatis
        ↓
JDBC
        ↓
SELECT * FROM user ...
        ↓
MySQL
```

其中：

```text
Controller：处理 HTTP 请求
Service：处理业务逻辑
Mapper：负责数据库访问
```

---

# 14. JDBC 与 MyBatis 的本质关系

可以这样记：

## JDBC

```text
Java 操作数据库的基础标准 API
```

## MyBatis

```text
对 JDBC 进行进一步封装的数据库访问框架
```

自己使用 JDBC：

```text
开连接
 ↓
准备 SQL
 ↓
绑定参数
 ↓
执行 SQL
 ↓
处理 ResultSet
 ↓
转换 Java 对象
 ↓
关闭资源
```

使用 MyBatis：

```java
User user = userMapper.selectById(id);
```

MyBatis 将大量重复操作封装起来。

---

# 15. 为什么还要学习 JDBC

虽然实际项目通常会使用：

```text
MyBatis
MyBatis-Plus
JPA
```

而不会大量手写 JDBC，但是 JDBC 仍然必须理解。

因为如果不知道 JDBC，就很难真正理解：

```text
数据库连接池
事务
MyBatis 执行流程
Spring 事务
PreparedStatement
SQL 参数绑定
Mapper 底层原理
```

看到：

```java
User selectById(Long id);
```

应该能够想到背后的实际流程：

```text
Mapper 方法
   ↓
MyBatis
   ↓
PreparedStatement
   ↓
执行 SQL
   ↓
ResultSet
   ↓
Java 对象映射
```

---

# 16. 当前阶段的推荐学习顺序

推荐：

```text
MySQL 基础
    ↓
JDBC 基础
    ↓
MyBatis
    ↓
Spring + MyBatis
    ↓
Spring Boot + MyBatis
```

JDBC 不需要投入太久。

JDBC 重点掌握：

```text
DriverManager
Connection
PreparedStatement
ResultSet
executeQuery()
executeUpdate()
事务 commit / rollback
try-with-resources
```

然后尽快进入 MyBatis。

---

# 17. MyBatis 重点掌握内容

```text
Mapper
@Select
@Insert
@Update
@Delete
#{}
XML Mapper
参数传递
结果映射
动态 SQL
```

实际项目中最常见组合：

```text
Spring Boot
+
MyBatis
+
MySQL
```

---

# 18. 最重要的架构图

```text
前端
 ↓ HTTP
Controller
 ↓
Service
 ↓
Mapper
 ↓
MyBatis
 ↓
JDBC
 ↓
MySQL Driver
 ↓
MySQL
```

理解这张图，就基本理解了 Java 后端访问数据库的主要路径。

---

# 19. 手写练习建议

## 必须手写

```text
一个完整 JDBC 查询
PreparedStatement 参数查询
MyBatis Mapper 增删改查
```

## 建议手写一次

```text
JDBC insert / update / delete
JDBC commit / rollback
MyBatis XML Mapper
```

## 当前阶段看懂即可

```text
JDBC Driver 底层实现
MyBatis 源码
ResultSet 底层协议
Mapper 动态代理源码
```

---

# 20. 一句话总结

```text
JDBC 是 Java 访问数据库的基础 API。

MyBatis 建立在 JDBC 之上，
帮助我们封装连接、参数绑定、结果处理和对象映射等重复操作。

MyBatis 并没有替代 JDBC，
而是在 JDBC 之上提高开发效率。
```

学习路线：

```text
把 JDBC 完整跑通一次
        ↓
理解 Connection / PreparedStatement / ResultSet
        ↓
进入 MyBatis
        ↓
结合 Spring Boot + MyBatis + MySQL 做项目
```
