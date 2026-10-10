# Java 后端学习笔记：Spring、Web 请求处理与数据库设计

> 整理日期：2026-10-08  
> 主题：Spring Framework / Spring Boot / Spring MVC / Tomcat / Servlet / IoC / DI / 反射 / 接口与数据库设计 / 编码 / HandlerInterceptor  
> 阅读建议：先掌握请求处理主线，再对照各节的代码理解注解、对象管理和请求校验。

## 0. 一张图串联全部知识

```text
浏览器 / App
  │ HTTPS + HTTP 请求（URL、Header、Body）
  ▼
[Nginx，可选：反向代理 / TLS / 静态资源]
  ▼
Tomcat（Web 服务器 + Servlet 容器）
  │
  ├─ Servlet Filter（Servlet 过滤器链）
  ▼
DispatcherServlet（Spring MVC 的核心 Servlet）
  │
  ├─ HandlerMapping 查找处理器及匹配的拦截器
  ├─ HandlerInterceptor.preHandle(...)
  ▼
Controller（路由、参数绑定、返回 HTTP 响应）
  ▼
Service（业务规则、事务）
  ▼
Mapper / Repository（数据库访问）
  ▼
MySQL
  │
  └─ 返回数据 → Service → Controller → JSON 序列化 → HTTP 响应
```

**区分三个维度：**

- **Tomcat / Servlet**：Web 服务器和 Java Web 请求处理基础机制。
- **Spring MVC**：基于 Servlet 技术栈的 Web 框架，负责路由、参数绑定、Controller 调用及结果处理。
- **Spring Framework / Spring Boot**：对象管理、事务等应用基础设施，以及自动配置和便捷启动。

> 说明：图为典型同步 Servlet MVC 请求的简化流程；异常处理、异步请求、过滤器顺序和事务代理等在实际运行时存在额外细节。

---

## 1. Spring Framework、Spring Boot 与 Spring 应用

### 1.1 三者是什么

| 概念 | 本质 | 典型职责 |
| --- | --- | --- |
| Spring Framework | Java 应用开发框架 | IoC、DI、AOP、事务、Web MVC 等 |
| Spring Boot | 基于 Spring 的快速开发与配置方案 | 自动配置、Starter 依赖、可执行应用和内嵌服务器支持 |
| Spring Application | 使用 Spring 开发的具体程序 | 例如用户管理系统、图书借阅后端、定时任务 |

**重要：**Spring 应用不一定是 Web 应用，也不一定使用 Spring Boot；Spring Boot Web 应用通常会使用 Spring MVC 和内嵌 Tomcat，但也可以采用其他服务器或 Web 技术栈。

### 1.2 Spring Framework 的常见能力

1. **IoC（控制反转）**：把对象的创建、组装与生命周期管理交给 Spring 容器。
2. **DI（依赖注入）**：Spring 为组件提供其所需的其他对象。
3. **AOP（面向切面编程）**：通过代理等机制为方法调用增加事务、日志等横切逻辑。
4. **声明式事务**：使用 `@Transactional` 控制满足条件的方法调用中的事务行为。
5. **模块化集成**：可以组合 Spring MVC、JDBC、数据访问组件等。

### 1.3 Spring Boot 启动类

```java
@SpringBootApplication
public class DemoApplication {
    public static void main(String[] args) {
        SpringApplication.run(DemoApplication.class, args);
    }
}
```

- `@SpringBootApplication`：组合了 Spring Boot 配置、自动配置和组件扫描相关注解。
- `SpringApplication.run(...)`：创建并启动 Spring 应用上下文；对具备 Web 依赖的应用，还会启动相应的服务器。
- 默认组件扫描通常覆盖启动类所在包及其子包；类不在扫描范围内时，单独添加 `@Service` 不保证注册成 Bean。

配置示例：

```yaml
server:
  port: 8080
spring:
  application:
    name: demo
```

其中 `server.port` 控制应用内嵌 Web 服务器的监听端口，`spring.application.name` 是应用名称。

---

## 2. Spring 容器、Bean、IoC 与 DI

### 2.1 `@Service` 到底做什么

```java
@Service
public class UserService {
    public String findName(Long id) {
        return "user-" + id;
    }
}
```

- `class UserService`：定义一个普通 Java 类。
- `@Service`：声明此类为服务层组件，**符合扫描与注册条件时**由 Spring 管理为 Bean。
- `@Service` **不会**自动执行其业务方法，也不会生成业务实现。
- Bean 是由 Spring 容器管理的对象；`ApplicationContext` 是常见的容器入口。

### 2.2 构造器注入如何工作

```java
@RestController
@RequestMapping("/users")
public class UserController {
    private final UserService userService;

    public UserController(UserService userService) {
        this.userService = userService;
    }

    @GetMapping("/{id}/name")
    public String findName(@PathVariable Long id) {
        return userService.findName(id);
    }
}
```

执行逻辑：Spring 创建 `UserService` Bean → 调用 `UserController(UserService)` 构造器 → 将依赖传给 Controller → 请求到来时调用该对象。

**普通 Java 的语法并未改变：**

```java
private final UserService userService;
```

这一行**只声明字段，没有创建对象或赋值**。由于字段是 `final`，必须在声明处、实例初始化块或每条正常完成的构造器路径中初始化它。Spring 的构造器注入，本质上是让容器提供参数，再用普通 Java 的赋值语句进行初始化。

```java
// 普通 Java 也可以使用构造器注入，不必在当前类里 new UserService
UserService service = new UserService();
UserController controller = new UserController(service);
```

`private UserService userService;` 不写 `final` 时是合法的，实例字段默认值为 `null`，但**声明本身仍不等于对象创建**。

### 2.3 常见组件注解

| 注解 | 用途 |
| --- | --- |
| `@Component` | 通用组件 |
| `@Service` | 业务层组件 |
| `@Repository` | 数据访问组件 |
| `@Controller` | MVC 控制器 |
| `@RestController` | `@Controller` + `@ResponseBody`，适用于 REST 接口 |
| `@Configuration` | 配置类 |
| `@Bean` | 用方法显式定义 Bean |
| `@Autowired` | 自动装配；只有一个构造器时通常不必标注 |

**常见误解：**IoC 不等于“Spring 会自动创建程序里所有的类”；Spring 只管理注册到容器的 Bean。Bean 默认常见作用域是 singleton，但并非只有这一种作用域。

---

## 3. Tomcat、Servlet、Spring MVC 如何协作

### 3.1 本质区别

- **Tomcat**：实现 Servlet 规范的 Web 容器，同时提供 HTTP 服务能力；负责监听端口、连接处理、Servlet 生命周期等。
- **Servlet**：Java Web 规范定义的接口；`HttpServlet` 是便于处理 HTTP 的抽象基类。
- **Spring MVC**：在 Servlet 基础上组织 HTTP 请求处理的框架。
- **DispatcherServlet**：Spring MVC 的**前端控制器**，本身是一个 Servlet，由 Servlet 容器管理和调用。

因此，**不能简单说“整个 Spring MVC 运行在某个 Servlet 里面”**。更准确地说：Spring MVC 的核心入口 `DispatcherServlet` 是 Servlet，Controller 等则是 Spring 管理的业务/处理组件。

### 3.2 请求具体怎么到达 Controller

1. 客户端向服务器的 IP/域名及端口发送 HTTP 请求。
2. Tomcat 接收请求，经 Servlet Filter 链交给对应的 `DispatcherServlet`。
3. `DispatcherServlet` 使用 `HandlerMapping` 匹配处理方法和拦截器。
4. 匹配成功的拦截器执行 `preHandle()`。
5. `HandlerAdapter` 调用 Controller，处理参数绑定、类型转换等。
6. Controller 调用 Service、Mapper；处理完后生成响应内容（REST 场景通常序列化为 JSON）。
7. 返回 HTTP 响应，执行适用的拦截器收尾逻辑。

### 3.3 Spring MVC 与 Spring Boot 的分工

- Spring MVC 决定**怎么路由、绑定参数、处理 Controller 和响应**。
- Spring Boot 主要决定**怎么便捷配置、组合并启动这些组件**。

---

## 4. Controller 与 HTTP 接口常见用法

### 4.1 最小示例

```java
@RestController
@RequestMapping("/users")
public class UserController {
    @GetMapping("/{id}")
    public String find(@PathVariable Long id) {
        return "用户 ID：" + id;
    }
}
```

请求 `GET /users/123` 时，路径中的 `123` 会绑定成 `Long id`。

### 4.2 请求映射与参数注解

| 注解 | 用途 | 例子 |
| --- | --- | --- |
| `@GetMapping` | 查询 | `GET /users/1` |
| `@PostMapping` | 创建/提交 | `POST /users` |
| `@PutMapping` | 整体替换 | `PUT /users/1` |
| `@PatchMapping` | 部分更新 | `PATCH /users/1` |
| `@DeleteMapping` | 删除 | `DELETE /users/1` |
| `@RequestParam` | 读取查询/表单参数 | `/users?name=Roger` |
| `@PathVariable` | 读取路径变量 | `/users/1` |
| `@RequestBody` | 读取请求体并转换成 Java 对象 | JSON → DTO |

JSON 请求示例：

```http
POST /users
Content-Type: application/json

{"username":"Roger","age":20}
```

```java
public record CreateUserRequest(String username, Integer age) {}

@PostMapping("/users")
public String create(@RequestBody CreateUserRequest request) {
    return request.username();
}
```

`@RequestBody` 通常通过 Spring MVC 的 `HttpMessageConverter`（例如 Jackson JSON 转换器）完成 JSON 和 Java 对象之间的转换。

### 4.3 常见的配套功能

- `@Valid` + Bean Validation：检查非空、长度、格式等，需要对应的 Validation 依赖。
- `@RestControllerAdvice` + `@ExceptionHandler`：统一异常响应。
- `@Value` / `@ConfigurationProperties`：读取配置；多项配置更适合后者。
- `@SpringBootTest`：启动 Spring 测试上下文做集成测试。
- `@Transactional`：声明式事务；**默认主要针对 RuntimeException 和 Error 回滚**，受检异常默认不回滚，且受到代理调用等条件限制。

---

## 5. JDBC、MyBatis 与数据库访问

- **JDBC**：Java 数据库访问的基础 API，通过驱动连接数据库、发送 SQL、获取结果。
- **MyBatis**：在 JDBC 之上提供 SQL 映射、参数绑定和结果映射等能力。
- **Mapper**：数据访问接口；在 MyBatis 中可由框架生成代理实现。

```java
@Mapper
public interface UserMapper {
    @Select("SELECT id, username FROM user WHERE id = #{id}")
    User findById(Long id);
}
```

`#{id}` 对应参数绑定，而不是将用户输入直接拼进 SQL 字符串。动态 SQL 需要特别注意 `${}`：它是字符串替换，不能直接拼接不可信输入。

典型代码职责：

```text
Controller：协议层，接收请求、组织响应
Service：业务规则、协调操作与事务
Mapper：执行持久化操作
MySQL：数据持久化与约束
```

---

## 6. 如何从需求设计数据库和 API 文档

### 6.1 推荐流程

```text
原始需求
→ 提取用户行为、权限、规则
→ 识别实体及关系（业务建模）
→ 设计 API 初稿和数据表初稿
→ 共同评审、调整
→ 实现 Controller / Service / Mapper
→ 测试正常、异常和并发场景
```

**通常由后端开发工程师主导业务表设计**；复杂项目会由资深开发、架构师、DBA 参与设计或审核。DBA 也会负责运维、性能和变更管理等工作。

### 6.2 图书借阅案例

需求：注册登录、查询图书、借书、还书、查看借阅记录；一本书有多册；库存不足不能借；同一用户不能同时重复借同一种书。

核心实体：

```text
user               用户
book               图书（图书种类与库存）
borrow_record      借阅记录（关联 user 和 book）
```

字段示例：

```sql
CREATE TABLE user (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE book (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(200) NOT NULL,
    author VARCHAR(100) NOT NULL,
    total_stock INT NOT NULL DEFAULT 0,
    available_stock INT NOT NULL DEFAULT 0,
    CHECK (total_stock >= 0),
    CHECK (available_stock >= 0 AND available_stock <= total_stock)
);

CREATE TABLE borrow_record (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    book_id BIGINT NOT NULL,
    borrowed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    returned_at DATETIME NULL,
    status TINYINT NOT NULL DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (book_id) REFERENCES book(id),
    CHECK (status IN (0, 1))
);
```

约定 `status=0` 为借阅中，`status=1` 为已归还；这里是教学方案。若要跟踪每一册实体书、损坏情况和条形码，需要再引入 `book_copy` 表。CHECK 约束须使用支持并执行 CHECK 的数据库版本（如 MySQL 8.0.16+）。

### 6.3 数据库设计关注点

- **主键**：每行记录的唯一标识。
- **外键/关联**：表达用户、图书与借阅记录之间的关系。
- **约束**：NOT NULL、UNIQUE、CHECK、FOREIGN KEY 等。
- **字段类型**：ID 用 BIGINT、文本用 VARCHAR、计数用 INT、金额用 DECIMAL、时间用 DATETIME 等。
- **索引**：依据真实查询条件创建；索引有写入和维护成本。
- **事务与并发**：不能只依赖“先 SELECT 再 UPDATE”的非原子判断。

库存扣减推荐条件更新：

```sql
UPDATE book
SET available_stock = available_stock - 1
WHERE id = #{bookId}
  AND available_stock > 0;
```

受影响行为 `1` 表示成功扣减一册；为 `0` 表示不存在或库存不足。扣库存和新增借阅记录必须放在同一个事务中。**重复借阅**还需要数据库约束或正确的锁/并发方案，单独 `SELECT` 检查不可靠。

### 6.4 接口清单

| 功能 | Method | URL |
| --- | --- | --- |
| 注册 | POST | `/api/users/register` |
| 登录 | POST | `/api/auth/login` |
| 图书列表 | GET | `/api/books` |
| 图书详情 | GET | `/api/books/{id}` |
| 借阅图书 | POST | `/api/borrow-records` |
| 归还图书 | POST | `/api/borrow-records/{id}/return` |
| 我的借阅 | GET | `/api/borrow-records/me` |

单个接口文档至少包含：

1. 功能说明、HTTP 方法与 URL。
2. 认证权限要求。
3. 请求头、路径/查询参数、请求体的结构和校验规则。
4. 成功状态码及 JSON 响应示例。
5. 失败状态码、错误码和业务边界条件。

例如借阅接口：

```http
POST /api/borrow-records
Authorization: Bearer <JWT>
Content-Type: application/json

{"bookId":10}
```

成功：`201 Created`

```json
{"id":1001,"bookId":10,"status":"BORROWED"}
```

库存不足：`409 Conflict`

```json
{"code":"BOOK_OUT_OF_STOCK","message":"图书库存不足"}
```

**安全细节**：当前借阅用户的 ID 应从已验证的登录身份提取，而不是直接相信前端传来的 `userId`。

可使用 **OpenAPI / Swagger UI / Apifox** 管理、展示和测试 API 文档。

---

## 7. Java 反射：动态获取类信息与调用方法

### 7.1 反射解决什么问题

普通调用：

```java
UserService service = new UserService();
service.hello();
```

反射允许在运行时读取类型、方法、字段等元数据，并动态实例化、调用：

```java
Class<?> clazz = Class.forName("com.example.UserService");
Object instance = clazz.getDeclaredConstructor().newInstance();
Method method = clazz.getMethod("hello");
method.invoke(instance);
```

这段代码假设目标类存在可访问的无参构造器和 public 的 `hello()` 方法。

### 7.2 常用 API

| API | 作用 |
| --- | --- |
| `User.class` | 直接取得 Class 对象 |
| `user.getClass()` | 获取对象实际运行时类型 |
| `Class.forName("...")` | 按完整类名加载/获得 Class 对象 |
| `getConstructor(...)` | 获取 public 构造器 |
| `getDeclaredConstructor(...)` | 获取当前类声明的构造器 |
| `getMethod(...)` | 获取 public 方法，包含可继承的方法 |
| `getDeclaredMethod(...)` | 获取本类声明的方法，不限 public |
| `getDeclaredField(...)` | 获取本类声明的字段 |
| `Constructor.newInstance(...)` | 创建对象 |
| `Method.invoke(...)` | 调用方法 |
| `Field.get(...)` / `Field.set(...)` | 读取/修改字段 |
| `isAnnotationPresent(...)` | 检查运行时可见注解 |

访问 private 成员时可能看到 `setAccessible(true)`，但它不能绕过所有 Java 模块访问限制，且不应滥用。

### 7.3 注解与 Spring 的关系

```java
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
public @interface MyService {}

@MyService
public class UserService {}

boolean exists = UserService.class.isAnnotationPresent(MyService.class);
```

Spring 可以读取类元数据和注解，结合 Bean 定义与实例化逻辑管理组件。**但 Spring 并不只靠反射工作**：还会使用元数据扫描、BeanDefinition、后处理器、动态代理等机制。

---

## 8. 常见编码、序列化与密码哈希

### 8.1 概念先区分

| 类别 | 代表 | 解决的问题 |
| --- | --- | --- |
| 字符集与字符编码 | Unicode、UTF-8、GBK、ASCII | 字符与字节如何对应 |
| 二进制到文本编码 | Base64 | 用可打印文本表示字节数据 |
| URL 百分号编码 | `%E4%BD%A0...` | 安全地表示 URL 中的特殊字节 |
| 序列化格式 | JSON、Protobuf | 结构化数据如何传输/保存 |
| 摘要与密码哈希 | SHA-256、bcrypt、Argon2id | 摘要、密码验证，不是可逆编码 |

### 8.2 字符编码

- **ASCII**：主要为英文字符和控制字符定义码值。
- **Unicode**：为字符定义码点（如 `A` 为 `U+0041`、`中` 为 `U+4E2D`），不是唯一的字节存储格式。
- **UTF-8**：Unicode 的变长编码；常见 ASCII 字符 1 字节，`中` 通常 3 字节，许多 emoji 4 字节。
- **GBK**：历史上常见的中文编码，与 UTF-8 不兼容。
- **UTF-16**：Java `char`/部分字符串 API 语义上采用 UTF-16 代码单元；现代 JVM 内部 String 存储可能使用 Compact Strings，**不能简单理解为每个 Java 字符固定占两字节**。

```java
byte[] data = "你好".getBytes(StandardCharsets.UTF_8);
String text = new String(data, StandardCharsets.UTF_8);
```

建议涉及编码转换时**显式指定 Charset**；现代 JDK 的默认 charset 通常为 UTF-8（JDK 18 起 JEP 400），但兼容旧程序及外部协议仍不应盲目依赖默认值。MySQL 保存完整 Unicode，通常应使用 `utf8mb4` 字符集。

### 8.3 Base64 和 URL 编码

```java
String b64 = Base64.getEncoder().encodeToString(
    "hello".getBytes(StandardCharsets.UTF_8));
// aGVsbG8=
```

Base64 **可解码，不是加密**。JWT 常见的三段结构采用 Base64URL 编码，并依靠签名或其他密码学机制保护真实性，而非依靠 Base64 保密。

URL 查询参数经百分号编码后可安全传输；Java `URLEncoder` 面向 HTML 表单编码规则（例如空格编码为 `+`），并非可无差别用于完整 URL 所有部分。

### 8.4 JSON、Protobuf 和密码

- **JSON**：常见 Web API 文本数据格式，Jackson 等组件负责对象与 JSON 之间的序列化/反序列化。
- **Protobuf**：具备 schema 的二进制序列化格式，常见于 gRPC 等场景。
- **MD5 / SHA-256**：快速摘要算法，不适合直接存储用户密码。
- **bcrypt / Argon2id**：专门用于密码哈希，结合随机盐及可调整成本，更适合保存用户密码。
- **加密 vs 哈希**：加密通常可在掌握密钥时解密；安全密码哈希是单向验证，不存明文。

---

## 9. HandlerInterceptor（Spring MVC 拦截器）

### 9.1 用途和位置

`HandlerInterceptor` 是 Spring MVC 的拦截器接口，用于在 Controller 处理前后执行公共逻辑，例如登录状态校验、访问日志、耗时统计等。

```text
客户端 → Tomcat → Filter 链 → DispatcherServlet
                                 ↓
                        HandlerInterceptor
                                 ↓
                             Controller
```

**Filter 属于 Servlet 体系；Interceptor 属于 Spring MVC 体系。**

### 9.2 三个回调的意义

```java
public interface HandlerInterceptor {
    boolean preHandle(HttpServletRequest request,
                      HttpServletResponse response,
                      Object handler) throws Exception;

    void postHandle(HttpServletRequest request,
                    HttpServletResponse response,
                    Object handler,
                    ModelAndView modelAndView) throws Exception;

    void afterCompletion(HttpServletRequest request,
                         HttpServletResponse response,
                         Object handler,
                         Exception ex) throws Exception;
}
```

| 方法 | 触发位置 | 常见用途 |
| --- | --- | --- |
| `preHandle` | Controller 的处理器执行之前 | 鉴权、设置请求上下文；返回 false 阻止继续调用 |
| `postHandle` | 处理器正常执行后、视图渲染前 | 调整视图模型；REST JSON 场景往往用途有限 |
| `afterCompletion` | 请求处理完成后的清理阶段 | 日志、清理 ThreadLocal 等 |

**注意：**返回 `false` 不会自动生成规范错误响应，应自行设置状态/响应或通过合适的异常处理机制实现；而且 `afterCompletion` 通常只会对已成功通过 `preHandle` 的相应拦截器执行。某个拦截器自己的 `preHandle` 返回 `false` 时，不能指望它自己的 `afterCompletion` 替它清理已经设置的 ThreadLocal。

### 9.3 注册拦截器

```java
@Configuration
public class WebConfig implements WebMvcConfigurer {
    private final LoginInterceptor loginInterceptor;

    public WebConfig(LoginInterceptor loginInterceptor) {
        this.loginInterceptor = loginInterceptor;
    }

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(loginInterceptor)
                .addPathPatterns("/**")
                .excludePathPatterns("/api/auth/login", "/api/users/register");
    }
}
```

注册路径需要与实际接口一致；只写 `@Component` 而不配置相关拦截路径，并不代表所有 MVC 请求都自动使用它。

### 9.4 JWT + Interceptor + ThreadLocal

典型思路：

```text
用户登录并通过身份验证
  ↓
后端签发 JWT
  ↓
客户端以后请求使用 Authorization: Bearer <token>
  ↓
拦截器验证 JWT 签名、有效期、预期算法、发行方/受众等所需条件
  ↓
获得可信的 userId
  ↓
存入当前请求上下文（示例可用 ThreadLocal）
  ↓
Controller / Service 读取 userId
  ↓
请求结束清理上下文
```

**示意代码（依赖 `JwtVerifier` 和 `CurrentUser` 的实现，不是可直接运行的完整认证系统）：**

```java
@Component
public class LoginInterceptor implements HandlerInterceptor {
    private final JwtVerifier jwtVerifier;

    public LoginInterceptor(JwtVerifier jwtVerifier) {
        this.jwtVerifier = jwtVerifier;
    }

    @Override
    public boolean preHandle(HttpServletRequest req,
                             HttpServletResponse resp,
                             Object handler) {
        String header = req.getHeader("Authorization");
        if (header == null || !header.startsWith("Bearer ")) {
            resp.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return false;
        }

        try {
            Long userId = jwtVerifier.verifyAndGetUserId(header.substring(7));
            CurrentUser.set(userId);
            return true;
        } catch (JwtVerificationException e) {
            CurrentUser.clear(); // 自己返回 false 时不能依赖 afterCompletion 清理
            resp.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return false;
        }
    }

    @Override
    public void afterCompletion(HttpServletRequest req,
                                HttpServletResponse resp,
                                Object handler,
                                Exception ex) {
        CurrentUser.clear();
    }
}
```

`ThreadLocal` 将值绑定到**当前线程**而非“当前用户”或“整个请求的全生命周期”；线程池复用时不清理会发生身份串用/内存泄漏风险，异步请求或跨线程执行时也不会自动传递上下文。对于复杂认证需求，优先研究 **Spring Security** 和其安全上下文机制，避免自行实现整套安全体系。

### 9.5 Filter、Interceptor、AOP 对比

| 机制 | 作用范围 | 典型用途 |
| --- | --- | --- |
| Servlet Filter | Servlet 请求/响应过滤链 | 编码、CORS、日志、安全过滤链等 |
| Spring MVC Interceptor | MVC 匹配到的处理器执行过程 | Controller 前置检查、请求日志 |
| Spring AOP | 符合代理切点的 Java 方法调用 | 声明式事务、方法级日志等 |

补充：Filter 也能使用 Spring Bean（例如通过 `DelegatingFilterProxy` 或 Spring Boot 集成）；**不能说 Filter 天然无法访问 Spring Bean**。Interceptor 对静态资源是否起作用取决于资源如何映射及是否经过 MVC 处理链，不宜绝对化。

---

## 10. 常见易错点与自检

| 常见说法 | 更准确的理解 |
| --- | --- |
| “`@Service` 自动执行业务” | 它主要声明组件身份，方法需要被业务调用才执行 |
| “`private final UserService x;` 不用初始化” | 必须初始化；构造器注入由 Spring 提供对象再赋值 |
| “Spring MVC 运行在 Servlet 里面” | DispatcherServlet 是 Servlet，Spring MVC 是围绕它的一套框架组件 |
| “`preHandle(false)` 就自动返回 401” | 需要代码设置 401 或使用合适的异常处理机制 |
| “任何情况下 afterCompletion 都会执行” | 受 preHandle 是否通过、异步调度等流程限制 |
| “ThreadLocal 是用户级存储” | 是线程级存储，要及时清理，不适合默认跨线程传递 |
| “JWT 解析出 ID 就安全了” | 必须严格验证签名及相关声明，且配合业务授权判断 |
| “Base64 就是加密” | Base64 是可逆编码，不保护机密性 |
| “MD5 加密密码” | MD5 是摘要且不适合密码存储；用 bcrypt/Argon2id 等 |
| “先查库存再扣减就不会超卖” | 并发下不可靠，需要原子条件更新及事务/约束 |
| “注解多就能代替业务设计” | 注解简化基础设施，不代替需求分析与业务规则 |

### 面试/复盘自测

1. `Bean`、`ApplicationContext`、IoC、DI 分别是什么？
2. Spring 是如何把 `UserService` 注入 `UserController` 的？
3. `private final UserService userService;` 为什么不能永远不赋值？
4. `DispatcherServlet` 与 Tomcat、Controller 的关系是什么？
5. `@RequestBody` 如何把 JSON 转为 Java 对象？
6. `@Transactional` 为什么有时不生效？默认回滚规则是什么？
7. 反射中的 `Class`、`Constructor`、`Method`、`Field` 有什么用途？
8. 根据“借书”需求，怎么确定表、索引、事务与 HTTP API？
9. UTF-8、Unicode、Base64、JSON、密码哈希各解决什么问题？
10. `preHandle`、`postHandle`、`afterCompletion` 的触发时机有什么区别？
11. 为什么 JWT 的 userId 要从验证后的令牌获取，而不能信任客户端传入的 userId？
12. 使用 ThreadLocal 保存当前用户有什么风险？

## 11. 推荐学习与实践顺序

1. **先打通请求链**：Tomcat → DispatcherServlet → Controller → Service → Mapper → MySQL。
2. **写最小 CRUD**：用户/任务管理 API，用 `GET/POST/PATCH/DELETE` 与 JSON DTO。
3. **补充容器知识**：`@Service`、组件扫描、Bean、构造器注入。
4. **补充数据设计**：主外键、索引、事务、并发更新和接口文档。
5. **引入身份认证**：先理解 JWT/Interceptor/ThreadLocal，再学习 Spring Security 的成熟实现。
6. **补充底层机制**：反射、AOP/动态代理、`@Transactional` 原理、过滤器/拦截器差异。

**总目标：**不仅会照着教程写注解，还能根据需求独立判断“需要哪些数据、暴露哪些接口、具有什么业务规则、每一层代码负责什么”。
