# Java 后端基础学习笔记

> 本笔记整理自本次聊天，按“先整体作用 → 再局部语法 → 最后手写建议”的方式组织。

---

## 1. JDBC 与 MyBatis

### 1.1 JDBC 是什么

JDBC（Java Database Connectivity）是 Java 官方提供的数据库访问 API。

整体链路：

```text
Java 程序
   ↓
JDBC API
   ↓
数据库驱动
   ↓
MySQL
```

JDBC 典型流程：

```text
获取连接
→ 创建 PreparedStatement
→ 设置 SQL 参数
→ 执行 SQL
→ 处理 ResultSet
→ 关闭资源
```

常见对象：

- `Connection`：表示 Java 与数据库之间的一条连接
- `PreparedStatement`：执行带参数的 SQL
- `ResultSet`：接收查询结果

示例：

```java
Connection conn = DriverManager.getConnection(url, username, password);

String sql = "SELECT * FROM user WHERE id = ?";
PreparedStatement ps = conn.prepareStatement(sql);

ps.setLong(1, 1L);

ResultSet rs = ps.executeQuery();

if (rs.next()) {
    long id = rs.getLong("id");
    String usernameValue = rs.getString("username");
}
```

### 1.2 MyBatis 是什么

MyBatis 是建立在 JDBC 之上的持久层框架。

整体关系：

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

MyBatis 主要帮你处理：

- 获取和管理连接
- 参数绑定
- 执行 SQL
- `ResultSet → Java 对象`
- 资源释放
- 大量重复 JDBC 模板代码

例如：

```java
@Mapper
public interface UserMapper {

    @Select("SELECT * FROM user WHERE id = #{id}")
    User findById(Long id);
}
```

调用：

```java
User user = userMapper.findById(1L);
```

### 1.3 JDBC 和 MyBatis 的关系

不是二选一，而是：

```text
MyBatis
   ↓
JDBC
   ↓
数据库驱动
   ↓
MySQL
```

MyBatis 更高层，JDBC 更底层。

---

## 2. MyBatis 注解与 SQL

### 2.1 `@Insert`

```java
@Insert("""
    INSERT INTO user(username, password)
    VALUES(#{username}, #{password})
""")
```

这段代码包含三层语法。

#### 第一层：`@Insert`

MyBatis 注解，表示该方法执行 INSERT SQL。

```java
@Insert("INSERT INTO user(username) VALUES(#{username})")
void insert(User user);
```

#### 第二层：`""" ... """`

Java 文本块（Text Block），本质仍然是 `String`，只是方便写多行文本。

```java
String sql = """
    SELECT *
    FROM user
    WHERE id = 1
    """;
```

#### 第三层：`#{username}`

MyBatis 参数占位符。

例如：

```java
void insert(User user);
```

如果 `User` 有：

```java
private String username;
private String password;
```

那么：

```text
#{username} → user.getUsername()
#{password} → user.getPassword()
```

MyBatis 底层大致会转换成：

```sql
INSERT INTO user(username, password)
VALUES(?, ?)
```

然后使用 `PreparedStatement` 设置参数。

---

## 3. `SELECT` 是什么

`SELECT` 用来查询数据库数据。

### 3.1 查询整张表

```sql
SELECT * FROM user;
```

含义：

> 查询 `user` 表中的所有列、所有行。

### 3.2 查询指定列

```sql
SELECT id, username FROM user;
```

只查询 `id` 和 `username`。

### 3.3 条件查询

```sql
SELECT * FROM user WHERE id = 1;
```

表示：

> 查询 `id = 1` 的用户。

### 3.4 CRUD 基础

```text
SELECT  查
INSERT  增
UPDATE  改
DELETE  删
```

---

## 4. 登录代码中的 Map 与 JWT

典型代码：

```java
Map<String, Object> claims = new HashMap<>();

claims.put(
    JwtClaimsConstant.EMP_ID,
    employee.getId()
);

String token = JwtUtil.createJWT(
    jwtProperties.getAdminSecretKey(),
    jwtProperties.getAdminTtl(),
    claims
);
```

### 4.1 整体作用

这段代码的作用：

> 把登录员工的信息放进 JWT 的 claims，然后生成 token。

整体流程：

```text
登录成功
   ↓
拿到 Employee
   ↓
创建 claims
   ↓
放入 employeeId
   ↓
生成 JWT
   ↓
把 token 返回前端
```

### 4.2 为什么创建 Map

```java
Map<String, Object> claims = new HashMap<>();
```

这里的 `Map` 是一个临时数据容器，用来保存 JWT 要携带的数据。

例如：

```java
claims.put("empId", 1L);
claims.put("username", "admin");
claims.put("role", "ADMIN");
```

可以理解成：

```text
empId    → 1
username → admin
role     → ADMIN
```

这些数据之后会放进 JWT 的 payload。

### 4.3 每次都需要创建新的 Map 吗

通常每次生成 JWT 都会创建一份新的 claims：

```java
Map<String, Object> claims = new HashMap<>();
```

原因：

- 每次登录的数据不同
- 避免不同请求之间共享数据
- 避免并发覆盖
- 创建一个小 `HashMap` 成本很低

不要把登录 claims 做成全局共享变量。

---

## 5. Builder 与链式调用

例如：

```java
EmployeeLoginVO employeeLoginVO =
        EmployeeLoginVO.builder()
                .id(employee.getId())
                .userName(employee.getUsername())
                .name(employee.getName())
                .token(token)
                .build();
```

### 5.1 整体作用

Builder 用来方便地构造属性较多的对象。

不用 Builder 时可能写成：

```java
EmployeeLoginVO vo = new EmployeeLoginVO();

vo.setId(employee.getId());
vo.setUserName(employee.getUsername());
vo.setName(employee.getName());
vo.setToken(token);
```

Builder 更紧凑：

```java
EmployeeLoginVO.builder()
    .id(...)
    .userName(...)
    .name(...)
    .token(...)
    .build();
```

### 5.2 为什么能一直 `.xxx()`

因为这些方法通常会返回 Builder 自己：

```java
public Builder id(Long id) {
    this.id = id;
    return this;
}
```

所以可以连续：

```java
.id(...)
.userName(...)
.name(...)
.token(...)
```

这叫链式调用。

---

## 6. YAML / YML

YAML 常见扩展名：

```text
.yml
.yaml
```

它经常用来写配置。

Spring Boot 常见：

```text
application.yml
```

### 6.1 基本语法

```yaml
name: luojie
age: 20
```

核心语法：

```text
key: value
```

### 6.2 用缩进表示层级

```yaml
spring:
  datasource:
    username: root
    password: 123456
```

等价理解：

```text
spring.datasource.username = root
spring.datasource.password = 123456
```

### 6.3 `server.port`

```yaml
server:
  port: 8080
```

整体作用：

> 告诉 Spring Boot：Web 服务器监听 8080 端口。

其中：

```yaml
server:
```

是 Spring Boot 规定的“服务器相关配置”前缀。

```yaml
port: 8080
```

表示监听端口为 8080。

项目启动后一般访问：

```text
http://localhost:8080
```

如果改成：

```yaml
server:
  port: 9090
```

则通常访问：

```text
http://localhost:9090
```

---

## 7. 设计模式

设计模式可以理解成：

> 针对常见代码设计问题总结出来的成熟解决套路。

设计模式不是 Java 语法，也不是框架。

核心目的通常是：

```text
降低耦合
+
提高扩展性
+
提高可维护性
```

### 7.1 创建型

#### Singleton：单例模式

作用：

> 保证一个类通常只有一个实例。

典型结构：

```java
public class Singleton {

    private static final Singleton INSTANCE = new Singleton();

    private Singleton() {}

    public static Singleton getInstance() {
        return INSTANCE;
    }
}
```

#### Factory：工厂模式

作用：

> 把对象创建交给专门的工厂。

```java
Payment payment = PaymentFactory.create("ali");
```

#### Builder：建造者模式

作用：

> 方便创建属性较多的对象。

```java
User.builder()
    .id(1L)
    .username("admin")
    .build();
```

### 7.2 结构型

#### Proxy：代理模式

作用：

> 在调用真实对象前后增加额外逻辑。

适合：

- 日志
- 权限检查
- 事务
- 性能统计

Spring AOP 大量使用代理思想。

#### Adapter：适配器模式

作用：

> 两个接口不兼容时，在中间加一层转换。

#### Decorator：装饰器模式

作用：

> 不修改原类的情况下给对象增加功能。

Java IO 典型：

```java
InputStream input =
        new BufferedInputStream(
            new FileInputStream("a.txt")
        );
```

### 7.3 行为型

#### Strategy：策略模式

作用：

> 把不同方案分别封装成独立实现，可以自由替换。

适合减少大型 `if-else`。

#### Observer：观察者模式

作用：

> 一个对象变化时通知多个对象。

#### Template Method：模板方法

作用：

> 固定整体流程，让部分步骤由子类决定。

Spring 常见名字：

```text
JdbcTemplate
RedisTemplate
RestTemplate
```

---

## 8. Web Service、Web Server 与后端

### 8.1 Web Service

可以简单理解成：

> 通过网络对外提供功能的服务。

例如：

```http
GET /users/1
```

### 8.2 Web Server

Web Server 是负责接收网络请求的服务器程序。

常见：

```text
Tomcat
Nginx
Apache
```

### 8.3 后端是不是服务器

从“服务器程序”的角度看，是。

Spring Boot 启动之后：

```text
Spring Boot
   ↓
内置 Tomcat
   ↓
监听 8080
   ↓
等待 HTTP 请求
```

因此 Spring Boot 后端本身就是服务端程序。

---

## 9. 服务器到底是干什么的

服务器最核心的工作：

```text
监听请求
→ 接收请求
→ 处理请求
→ 返回结果
```

典型 Web 后端链路：

```text
前端 / 浏览器
      ↓
   HTTP 请求
      ↓
Tomcat / Nginx
      ↓
Controller
      ↓
Service
      ↓
Mapper / MyBatis
      ↓
MySQL
      ↑
Spring Boot 返回 JSON
      ↑
前端
```

“服务器”有两层含义：

### 服务器机器

例如：

```text
一台 Linux 云主机
```

### 服务器程序

例如：

```text
Nginx
Tomcat
MySQL
Redis
```

常见端口：

```text
Nginx     → 80 / 443
Tomcat    → 8080
MySQL     → 3306
Redis     → 6379
```

可以记：

```text
IP 地址 = 找到哪台机器
端口号   = 找到机器上的哪个程序
```

---

## 10. 前端能不能直接请求后端

可以。

开发环境：

```text
前端
  ↓
Spring Boot :8080
```

前端可以直接请求：

```text
http://服务器IP:8080/api/user
```

不一定需要 Nginx。

生产环境常见：

```text
前端
  ↓
Nginx :80 / :443
  ↓
Spring Boot :8080
```

Nginx 常负责：

- HTTPS
- 静态资源
- 反向代理
- 负载均衡
- 统一入口

而 Spring Boot 负责真正的业务逻辑。

---

## 11. 加 Nginx 后是不是两个服务器

如果按“服务器程序”来理解，是。

```text
前端
  ↓
Nginx
  ↓
Spring Boot
```

其中：

```text
Nginx
```

负责接入和转发。

```text
Spring Boot
```

负责业务处理。

它们可以运行在同一台机器：

```text
一台 Linux 机器

├── Nginx       80 / 443
├── Spring Boot 8080
└── MySQL       3306
```

也可以分布在不同机器。

---

## 12. MD5 信息摘要

Java 已经有现成 API，不需要自己实现 MD5 算法。

标准库：

```java
MessageDigest
```

示例：

```java
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;

String text = "hello";

MessageDigest md = MessageDigest.getInstance("MD5");

byte[] result =
        md.digest(text.getBytes(StandardCharsets.UTF_8));
```

### 12.1 关键语法

```java
MessageDigest.getInstance("MD5")
```

获取一个 MD5 摘要算法对象。

```java
text.getBytes(StandardCharsets.UTF_8)
```

把字符串转成字节数组。

```java
md.digest(...)
```

执行摘要计算。

最终得到：

```java
byte[]
```

通常再转成十六进制字符串。

### 12.2 MD5 不适合存密码

密码存储不要使用普通 MD5。

更常见：

```text
BCrypt
Argon2
PBKDF2
```

---

# 13. 当前阶段建议掌握程度

## 必须手写

```text
JDBC 基本流程
PreparedStatement
ResultSet
SELECT / INSERT / UPDATE / DELETE

MyBatis：
@Select
@Insert
#{参数}

Map：
new HashMap<>()
put()

YAML：
key: value
缩进层级
server.port

Builder 基本用法

MessageDigest 基本调用
```

## 建议手写一次

```text
完整 JDBC CRUD
MyBatis CRUD
Builder 小 Demo
Singleton
Factory
Strategy
Proxy
YAML 列表
MD5 byte[] → 十六进制字符串
```

## 当前看懂即可

```text
MyBatis 内部 Executor
JDBC Driver 底层协议
JWT 内部签名算法实现
Spring AOP 动态代理源码
23 种 GoF 模式全部细节
MD5 算法内部轮函数
```

---

# 14. 最重要的整体知识链路

把本次聊天内容串起来，可以形成这张图：

```text
前端
  ↓ HTTP
Nginx（可选）
  ↓
Spring Boot
  ↓
Tomcat
  ↓
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

登录场景：

```text
前端提交账号密码
   ↓
Spring Boot 接收请求
   ↓
Service 校验用户
   ↓
MyBatis / JDBC 查询 MySQL
   ↓
登录成功
   ↓
创建 claims Map
   ↓
生成 JWT
   ↓
Builder 创建返回对象
   ↓
返回 token 给前端
```

---

# 15. 自测问题

1. JDBC 和 MyBatis 的关系是什么？
2. `PreparedStatement` 为什么使用 `?`？
3. `@Select` 和 `@Insert` 分别干什么？
4. `#{id}` 是 Java 语法还是 MyBatis 语法？
5. JWT 中为什么经常创建一个 `Map<String, Object>`？
6. 为什么不要多个登录请求共用一个 claims Map？
7. `server.port: 8080` 实际配置了什么？
8. `server` 是普通 Java 变量吗？
9. Spring Boot 后端算不算服务器程序？
10. 前端能不能不经过 Nginx 直接访问 Spring Boot？
11. Nginx 和 Spring Boot 能不能部署在同一台机器？
12. Builder 模式解决了什么问题？
13. Strategy 模式适合解决哪类大型 `if-else`？
14. Java 中做 MD5 可以使用哪个标准类？
15. 为什么不应该用普通 MD5 存储用户密码？
