# Java 后端学习笔记：从 HTTP 请求到 Spring Boot

> 整理范围：本次聊天涉及的 Java 注解、接口文档、HTTP 请求链路、Tomcat、Servlet、容器、Spring MVC / Spring Boot、分层开发、MyBatis / JDBC、Nginx、密码哈希，以及 Java 版本与《苍穹外卖》的学习取舍。
>
> **学习主线：先理解没有框架时程序如何工作，再理解框架替我们封装了什么。**

---

## 0. 一张图建立整体认知

```text
浏览器（HTML / CSS / JavaScript）
    │  fetch() 发送 HTTP 请求
    │  DNS（访问域名时）→ 建立连接 → HTTPS 时建立 TLS
    ▼
[Nginx：可选的反向代理 / HTTPS 入口 / 静态资源服务器]
    │  转发动态接口请求
    ▼
Tomcat（HTTP 服务器 + Servlet 容器）
    │  接收连接、解析 HTTP、执行 Filter 链
    ▼
DispatcherServlet（Spring MVC 的核心 Servlet）
    │  HandlerMapping 找方法、HandlerAdapter 执行
    │  参数解析、类型转换、拦截器等
    ▼
Controller（Web 接口入口）
    ▼
Service（业务规则）
    ▼
Mapper / MyBatis（数据库访问映射）
    ▼
JDBC API → MySQL JDBC 驱动 → MySQL 服务器
    │
    └── 结果按调用链返回到 Controller
           ▼
    Spring MVC 将返回值写入 HTTP 响应体（如 JSON）
           ▼
    Tomcat → [Nginx] → 浏览器解析 JSON、更新页面
```

**一句话记忆：**网络协议负责传输数据；Tomcat 负责提供 Java Web 运行环境；Spring MVC 负责把 HTTP 请求交给 Java 方法；Spring 管理业务对象；MyBatis/JDBC 负责数据库访问；Spring Boot 把这些组件方便地整合、配置并启动。

---

## 1. 先弄懂“程序、框架、容器”

### 1.1 这些东西本质上是程序吗？

是。Spring、Spring MVC、Spring Boot 等都是由大量 Java 类、接口和方法组成的软件框架，通常以 JAR 依赖引入项目；Tomcat 则是可以独立部署、也可以内嵌运行的 Java Web 服务器软件。

**框架不是脱离 Java 的魔法。**它只是提供通用代码、约定和扩展点，让开发者不用重复编写底层实现。

### 1.2 “容器”到底是什么意思？

在这里，容器指**管理一组组件/对象，并向它们提供运行支持与公共服务的机制**，不是一定有一个物理“盒子”。

| 名称 | 管理对象 | 提供的服务 |
|---|---|---|
| Servlet 容器（如 Tomcat） | Servlet、Filter 等 Web 组件 | 生命周期、请求调度、Web 运行环境 |
| Spring IoC 容器（如 `ApplicationContext`） | Spring Bean（Controller、Service 等） | 对象创建、依赖注入、生命周期管理 |
| Docker 容器 | 应用进程及相关运行环境 | 操作系统层面的进程隔离等 |

这三种“容器”**名字相同但不是一个技术概念**。

普通 Java 写法通常是：

```java
UserService service = new UserService();
service.handle();
```

而在 Spring 中，很多对象由 IoC 容器创建、管理，并在别的对象需要时注入。

### 1.3 IoC 与 DI

- **IoC（控制反转）：**对象的创建、组装等控制工作从业务代码转移给框架。
- **DI（依赖注入）：**IoC 的一种常见实现方式；框架把一个对象所需的依赖交给它。

```java
@Service
public class UserService {
    private final UserMapper mapper;

    public UserService(UserMapper mapper) {
        this.mapper = mapper;
    }
}
```

这个构造器不需要你在 Controller 中手动调用：Spring 可以注入符合要求的 Mapper Bean。

---

## 2. 浏览器向后端发送请求，底层发生了什么？

### 2.1 前端负责什么

- HTML：页面结构。
- CSS：样式。
- JavaScript：交互、发送请求、处理响应、更新页面。

```html
<button onclick="loadUser()">查询用户</button>
<script>
async function loadUser() {
    const response = await fetch('/api/users/123');
    if (!response.ok) throw new Error('请求失败');
    const user = await response.json();
    console.log(user.name);
}
</script>
```

`fetch()` 是**浏览器提供的 API**；开发者没有直接手写 TCP 协议代码。

### 2.2 从 URL 到服务器

以 `https://example.com/api/users/123` 为例：

1. 浏览器解析 URL，确定协议、主机、端口、路径。
2. 需要时通过 DNS 把域名解析成 IP 地址。
3. 建立网络连接（常见 HTTP/1.1、HTTP/2 基于 TCP；HTTP/3 基于 QUIC/UDP）。
4. HTTPS 会进行 TLS 协商以保护传输。
5. 发送 HTTP 请求；服务端处理后返回 HTTP 响应。

**IP 决定与哪台主机通信，端口决定与该主机上的哪个网络服务通信。**

### 2.3 HTTP 请求示例

```http
GET /api/users/123 HTTP/1.1
Host: localhost:8080
Accept: application/json
```

- `GET`：请求方法（通常用于读取资源）。
- `/api/users/123`：路径。
- `HTTP/1.1`：协议版本。
- `Host`：目标主机及端口。
- `Accept`：客户端希望接收的响应媒体类型。

典型响应：

```http
HTTP/1.1 200 OK
Content-Type: application/json

{"id":123,"name":"Roger"}
```

HTTP 是**通信协议**，不是某个框架专有的技术。

### 2.4 前后端不是直接“共享 Java 对象”

后端 JVM 内存中的对象不能直接送给浏览器当作 Java 对象使用。通常需要将对象**序列化**为 JSON 等网络可传输的数据；浏览器再解析为 JavaScript 数据。

常见状态码：`200` 成功；`400` 请求参数有问题；`401` 未认证；`403` 无权限；`404` 未找到资源；`500` 服务器内部错误。

---

## 3. 如果没有框架：Java 怎么处理 HTTP？

学过 `Socket`、`ServerSocket` 后，可以把 Web 服务器理解成一种长期运行的网络程序：

```java
ServerSocket server = new ServerSocket(8080);
while (true) {
    Socket client = server.accept();  // 等待 TCP 客户端连接
    // 读取客户端发送的 HTTP 数据
    // 解析方法、路径、请求头、请求体
    // 执行业务逻辑
    // 按 HTTP 协议格式构造响应并写回
    client.close();
}
```

上述是**原理伪代码**，不能当作完整的 HTTP 服务器。真实实现还要正确处理 HTTP 报文边界、并发、超时、连接复用、安全等。

- `ServerSocket(8080)`：通过操作系统在本机监听 TCP 端口。
- `accept()`：等待并获取已建立连接对应的 Socket。
- `getInputStream()`：读取客户端的数据。
- `getOutputStream()`：向客户端写数据。

**问题：**每写一个 Web 项目，都重写网络管理、HTTP 解析、路径分发、异常处理和线程调度，成本很高。

**解决办法：**交给成熟的 Web 服务器、Servlet 容器与 Web 框架。

---

## 4. Tomcat、Servlet、Servlet 容器

### 4.1 Tomcat 是什么？必须使用吗？

Tomcat 是 Java Web 服务器，也是 Servlet 容器：

- 接收 HTTP 请求、解析 HTTP 消息。
- 管理网络连接、线程和 Web 组件运行。
- 依据映射关系调用 Servlet。
- 把响应发送给客户端。

**Tomcat 不是 Java 后端必需的。**对于传统 Spring MVC 应用，通常需要兼容的 Servlet 容器，但可以选择适配的其他容器；非 Servlet 技术栈（如使用 Reactor Netty 的 WebFlux）也能处理 Web 请求。

常见 Spring Boot MVC 工程的 `spring-boot-starter-web` 会引入内嵌 Tomcat（在常见默认配置下），无需单独部署一个 Tomcat 安装包。

### 4.2 Servlet 到底是什么？

Servlet 首先是一套 **Java Web 规范及 API**。开发者实现它规定的行为，Servlet 容器根据规则管理并调用这些实现。

典型示例：

```java
@WebServlet("/hello")
public class HelloServlet extends HttpServlet {
    @Override
    protected void doGet(HttpServletRequest req,
                         HttpServletResponse resp) throws IOException {
        resp.setContentType("text/plain;charset=UTF-8");
        resp.getWriter().write("Hello World");
    }
}
```

说明：

- `@WebServlet`：声明 Servlet 映射，需要应用按相应配置注册/扫描。
- `extends HttpServlet`：继承处理 HTTP 的 Servlet 基类。
- `doGet()`：处理 GET 请求；`HttpServlet` 的 `service()` 会按请求方法分派。
- `req`：请求信息；`resp`：配置和写出响应。

Servlet 容器管理生命周期，大致是**加载/创建 → `init()` → 多次 `service()` → `destroy()`**。创建可以发生在启动期或首次请求；**通常不是每次请求创建一个 Servlet。**

### 4.3 为什么有 Servlet 还要 Spring MVC？

原生 Servlet 已免去手写 HTTP 服务器，但许多路径匹配、参数转换、JSON 响应、业务接口组织工作还不够便利。

Spring MVC 在 Servlet 之上进一步封装这些工作，让你可以写：

```java
@GetMapping("/hello")
public String hello() {
    return "Hello";
}
```

典型流程：

```text
Tomcat → Filter 链 → DispatcherServlet → Controller 方法 → HTTP 响应
```

`DispatcherServlet` 本身就是 Spring MVC 提供的一个 Servlet，属于**前端控制器（Front Controller）模式**的实现。

---

## 5. Spring、Spring MVC、Spring Boot 和 Nginx

| 名称 | 本质和主要职责 |
|---|---|
| **Spring Framework** | 一套框架，核心能力包括 IoC/DI、AOP、事务支持等 |
| **Spring MVC** | Spring 的 Servlet Web MVC 模块：请求映射、参数绑定、响应处理 |
| **Spring Boot** | 基于 Spring 的启动和自动配置体系：简化依赖、配置、组件整合与启动 |
| **Tomcat** | Java Web 服务器和 Servlet 容器：接收 HTTP、管理 Servlet 运行 |
| **Nginx** | Web 服务器 / 反向代理：静态资源、HTTPS 入口、负载均衡、请求转发 |

### 5.1 Nginx 与 Tomcat 的主要区别

```text
浏览器 → Nginx（80/443）
              ├─ /index.html → 直接返回静态文件
              └─ /api/... → Tomcat（如 8080）→ Spring MVC
```

- **Tomcat**擅长运行 Java Servlet Web 应用；自身也能处理 HTTP、HTTPS 和静态文件。
- **Nginx**常作为统一对外入口，可以转发给多个后端服务、做负载均衡等；也能独立处理 HTTP。
- 它们**不是必须成对出现**。学习阶段直接访问内嵌 Tomcat 的 `localhost:8080` 即可。
- Nginx 不局限于代理 Java，可以代理 Python、Go、Node.js 等应用。

### 5.2 区分“服务器”和“后端”

“服务器”可能指：

1. **硬件/主机**：一台电脑或虚拟机。
2. **服务器程序**：Tomcat、Nginx、MySQL Server。
3. **服务端应用**：你写的 Spring Boot 业务程序。

所以“Nginx 加上 Spring Boot，就有两个服务器吗？”——从**服务器软件/服务进程**角度可以这样理解，但它们完全可能部署在同一台机器上。

---

## 6. Spring MVC 注解与 Controller 语法精讲

```java
@RestController
@RequestMapping("/user")
public class UserController {

    @GetMapping("/{id}")
    public String getUser(@PathVariable("id") Long id) {
        return "用户ID：" + id;
    }
}
```

### 6.1 整体作用

访问 `GET http://localhost:8080/user/123`，Spring MVC 找到 `getUser()`，将路径中的 `"123"` 转为 `Long` 类型，执行方法，响应正文为：

```text
用户ID：123
```

**这段代码没有访问数据库**，只是演示 HTTP 到 Java 方法的映射。

### 6.2 逐个语法理解

| 代码 | 含义 |
|---|---|
| `public class UserController` | 定义 Java 类；`public` 为访问修饰符 |
| `@RestController` | Spring MVC 控制器；组合了 `@Controller` 与 `@ResponseBody` |
| `@RequestMapping("/user")` | 为类中处理方法指定共同的路径前缀 |
| `@GetMapping("/{id}")` | 将 GET 请求及相应路径映射到这个方法 |
| `/{id}` | Spring MVC 路径模板中的变量，不是 Java 字符串插值 |
| `@PathVariable("id")` | 从匹配的 URL 路径中取出 `id` 并绑定方法参数 |
| `Long id` | 包装类型；请求路径中的数字文本会进行类型转换 |
| `return "用户ID：" + id;` | Java 字符串拼接，并返回给 MVC 的响应处理机制 |

**映射路径的组合：**类上的 `/user` + 方法上的 `/{id}` ⇒ `/user/123`。

### 6.3 注解括号的语法

```java
@RequestMapping("/user")
@RequestMapping(value = "/user")  // 等价写法
```

在注解只指定 `value` 属性时可以省略 `value =`。

`@GetMapping("/{id}")` 可理解为下面这种组合注解的简写：

```java
@RequestMapping(value = "/{id}", method = RequestMethod.GET)
```

### 6.4 `@PathVariable`、`@RequestParam`、`@RequestBody`

```java
// GET /users/123 —— 路径变量
@GetMapping("/users/{id}")
public String a(@PathVariable("id") Long id) {
    return id.toString();
}

// GET /users?id=123 —— 查询参数
@GetMapping("/users")
public String b(@RequestParam("id") Long id) {
    return id.toString();
}

// POST /users —— 读取 JSON 请求体并转换为 Java 对象
@PostMapping("/users")
public String c(@RequestBody UserDTO dto) {
    return dto.name();
}

public record UserDTO(String name) {}
```

为了避免依赖 Java 方法参数名的编译保留设置，初学时建议明确写 `@PathVariable("id")`、`@RequestParam("id")`。

### 6.5 Java 注解到底有什么作用？没注解会怎样？

**注解是附着在类、方法、字段、参数等位置的元数据**，可供编译器、注解处理器、框架在不同阶段读取。

- `@Override`：编译器校验方法是否正确重写；删掉通常不影响本来正确的重写逻辑。
- `@RestController`：Spring MVC 识别控制器；没有相应替代配置时，删掉可能导致类不作为控制器注册。
- `@GetMapping`：Spring MVC 建立请求映射；删掉且无其他映射方式时，请求可能变成 404。
- `@Transactional`：Spring 事务功能的声明；是否生效还取决于事务配置、代理调用方式等。

**注解本身不会神奇地执行功能，是读取注解的编译器或框架代码让功能生效。**没有注解也能写 Java/Spring 程序，只是可能需要 XML、Java 配置或其他注册方式。

### 6.6 `@RestController` 是否一定返回 JSON？

不一定。

- `String` 返回值通常直接写成响应体中的文本。
- 对象返回值在配置了适当 JSON 消息转换器后，通常被序列化为 JSON。
- 这不是 Controller 直接“发网络包”，而是 Spring MVC 消息转换和 Tomcat 网络写出机制协作完成的。

---

## 7. Spring MVC 收到请求后，内部如何工作？

先区分两个阶段。

### 7.1 启动阶段：准备组件

```java
@SpringBootApplication
public class Application {
    public static void main(String[] args) {
        SpringApplication.run(Application.class, args);
    }
}
```

启动时，Spring Boot 大体会：创建 Spring 容器、扫描/注册 Bean、执行自动配置、创建并启动内嵌 Web 服务器、注册 `DispatcherServlet`、建立请求映射等。**它们的真实内部顺序并不等于这个概念列表的排列。**

### 7.2 请求阶段：按已建立的映射执行

```text
GET /api/users/123
  ↓
Tomcat：接收与解析 HTTP
  ↓
Filter 链：按配置执行过滤器（可能提前结束）
  ↓
DispatcherServlet
  ├─ HandlerMapping：根据路径、HTTP 方法等条件找 Handler
  ├─ HandlerAdapter：适配并执行 Handler
  ├─ 参数解析 / 类型转换："123" → Long 123
  ├─ Interceptor：在匹配 Handler 后按配置执行
  ↓
UserController.getUser(123L)
  ↓
Service → Mapper → MySQL
  ↓
Controller 返回 Java 对象
  ↓
HttpMessageConverter（如 Jackson JSON）
  ↓
Tomcat 写出 HTTP 响应
```

**几个容易混的组件：**

- `Filter`：Servlet 规范层面的过滤器，执行位置通常在 `DispatcherServlet` 之前。
- `Interceptor`：Spring MVC 处理器拦截器，围绕 Handler 执行。
- `DispatcherServlet`：中央请求分发 Servlet。
- `HandlerMapping`：解决“该执行谁”。
- `HandlerAdapter`：解决“怎样调用它”。
- `HttpMessageConverter`：解决“Java 返回值如何变成 HTTP 响应体”。

Spring MVC 默认的同步 Servlet 请求处理中，处理业务的代码通常运行在 Web 容器工作线程中。多个请求可能并发执行，因此单例 Bean 里不要随意保存每个请求独有的可变状态。

---

## 8. Controller → Service → Mapper → JDBC → MySQL

### 8.1 每层的职责

| 层 | 职责 | 不应误解为 |
|---|---|---|
| Controller | 接 HTTP 请求、参数绑定、组织响应 | 网络服务器本身 |
| Service | 组织业务规则、必要的事务边界 | 必须只做简单转发 |
| Mapper | 把 Java 方法映射为数据库操作 | 真正存数据的数据库 |
| MyBatis | SQL 映射、参数绑定、结果映射 | JDBC 的网络协议实现 |
| JDBC | Java 访问关系型数据库的 API 规范 | MySQL 本身 |
| JDBC Driver | 实现 JDBC，与数据库协议通信 | Spring MVC 的一部分 |
| MySQL Server | 存储数据并执行 SQL | Java 代码的一部分 |

### 8.2 一组连贯的示例（省略 import 与项目配置）

```java
// 实体/响应数据
public record User(Long id, String name) {}
```

```java
@RestController
@RequestMapping("/api/users")
public class UserController {
    private final UserService service;

    public UserController(UserService service) {
        this.service = service;
    }

    @GetMapping("/{id}")
    public ResponseEntity<User> getUser(@PathVariable("id") Long id) {
        User user = service.getById(id);
        return user == null
            ? ResponseEntity.notFound().build()
            : ResponseEntity.ok(user);
    }
}
```

```java
@Service
public class UserService {
    private final UserMapper mapper;

    public UserService(UserMapper mapper) {
        this.mapper = mapper;
    }

    public User getById(Long id) {
        return mapper.findById(id);
    }
}
```

```java
@Mapper
public interface UserMapper {
    @Select("SELECT id, name FROM users WHERE id = #{id}")
    User findById(@Param("id") Long id);
}
```

这组代码表达的是流程，需要配置相容的 Spring Boot、MyBatis、数据源和 `users` 表才能实际运行。

**为什么 Mapper 是接口还可以调用？**MyBatis 与 Spring 集成后会为 Mapper 接口生成代理对象，由代理根据 SQL 映射去执行数据库操作。

### 8.3 SQL 到数据库的进一步链路

```text
UserMapper.findById(123)
      ↓
MyBatis 处理映射、参数与结果
      ↓
JDBC（通常由连接池获取/复用 Connection）
      ↓
MySQL JDBC Driver
      ↓
通过网络连接与 MySQL Server 通信
      ↓
执行 SELECT
      ↓
结果映射为 User → Service → Controller
```

实际 Spring Boot 项目常使用数据库连接池（如 HikariCP）复用连接，而不是每次查询都重新进行数据库 TCP 握手。

### 8.4 参数绑定为什么重要？

错误示例：

```java
String sql = "SELECT * FROM users WHERE name='" + username + "'";
```

直接拼接不可信输入可能造成 SQL 注入。使用 JDBC `PreparedStatement` 或 MyBatis `#{}` 参数绑定，有助于防止输入被当成 SQL 结构的一部分：

```java
String sql = "SELECT * FROM users WHERE name = ?";
PreparedStatement ps = connection.prepareStatement(sql);
ps.setString(1, username);
```

但动态表名、列名、排序方向等**不能简单当作普通值参数绑定**，仍需要白名单等校验。

---

## 9. 接口文档是什么？“导入接口文档”是什么意思？

接口文档相当于**前后端协作的 API 说明书**，描述：

- 接口地址与 HTTP 方法。
- 请求路径参数、查询参数和请求体。
- 认证要求、状态码、返回数据结构。
- 请求/响应示例。

例如：

```text
POST /admin/employee/login
Content-Type: application/json
请求体：{"username":"admin","password":"123456"}
返回：{"code":1,"data":{"token":"..."}}
```

“导入接口文档”通常指将 OpenAPI JSON/YAML 等接口描述，导入 Apifox、Postman 等工具，以便浏览接口、填入参数、测试 HTTP 请求。具体支持的格式取决于工具。

**导入文档不等于创建后端接口。**Controller、Service 等业务代码仍需要实现。反过来，也可以借助 `springdoc-openapi` 等工具，从项目定义生成 OpenAPI 文档。

---

## 10. MD5、Hash 与加密：登录密码应该怎么存？

### 10.1 先纠正术语

- **Hash（哈希/摘要）**：把输入映射为固定长度摘要（这里讨论密码学哈希）。通常设计为难以逆向还原。
- **MD5**：一种具体的哈希算法，输出 128 bit（16 字节），通常显示为 32 位十六进制字符。
- **Encryption（加密）**：使用密钥进行可逆保护，如 AES 对称加密。

因此：**MD5 是 Hash 的一种，不是“MD5 和 Hash 两种加密”。**

特别注意：Java 对象的 `hashCode()` 也是“哈希”一词，但它不是安全的密码学哈希，更不能用来存储密码。

### 10.2 为什么不应该 `MD5(password)`？

MD5 计算很快，并且存在已知的密码学安全问题。攻击者得到密码数据库后，可以高速尝试候选密码，并比较其摘要。即使对 MD5 简单加盐，仍不能改变其计算速度过快的本质。

直接使用 `SHA-256(password)` 也**不适合密码存储**：虽然 SHA-256 作为摘要算法仍有广泛用途，但它同样是快速哈希。

### 10.3 密码存储的正确方向

推荐使用专门设计的密码哈希 / KDF：`Argon2id`、`bcrypt`、`scrypt`、适当配置的 `PBKDF2` 等。

| 场景 | 建议 |
|---|---|
| 用户密码存储 | Argon2id；或结合框架与兼容性使用 bcrypt 等 |
| 文件完整性摘要 | SHA-256 等安全摘要算法 |
| 需要解密恢复数据 | 依据需求选择 AES 等加密方案 |
| Java 对象散列表 | `hashCode()`，不是密码安全用途 |

在 Spring Security 中，bcrypt 示例：

```java
BCryptPasswordEncoder encoder = new BCryptPasswordEncoder();

// 注册：存数据库的是哈希值，不是原始密码
String storedHash = encoder.encode("123456");

// 登录：校验输入的密码是否匹配已存储哈希
boolean ok = encoder.matches("123456", storedHash);
```

bcrypt 通常使用随机盐，同一个密码多次调用 `encode()` 得到的结果也可能不同，因此**不要通过再次 `encode()` 后直接比较两个字符串**来验证密码。

**密码哈希保护数据库里的凭据；HTTPS 保护数据传输。二者不能互相替代。**

---

## 11. Java 版本与《苍穹外卖》应该如何学？

### 11.1 Java 企业版图（对应本次聊天讨论）

- Java 8、11：仍存在于不少存量企业系统。
- Java 17、21：现代 Spring 应用的重要长期支持版本。
- Java 25：更新的 LTS 版本，是否采用取决于项目生态和兼容性。

**对当前 Java 后端学习，选 Java 21 是稳妥方案**：既可学现代语言/API 特性，也容易与常见 Spring Boot 3.x 技术栈配合。别把“最新版”理解为企业唯一版本，也不必从 Java 8 逐个升级着学。

### 11.2 不要完全照抄《苍穹外卖》

《苍穹外卖》的价值是让你理解：请求路由、三层架构、CRUD、MyBatis、MySQL、Redis、缓存、认证、事务、WebSocket 等在真实业务中的组合方式。

可保留的核心学习内容：

- Spring MVC 请求处理流程。
- Controller / Service / Mapper 分层。
- MyBatis、SQL、MySQL。
- Redis 缓存、事务、异常处理、权限与接口设计。

需要按实际源码检查、适当改造的部分：

- **MD5 存密码 → bcrypt / Argon2id 等密码哈希方案。**
- **过旧的 Swagger / 依赖版本 → 与 Spring Boot 版本兼容的维护方案**（如 springdoc-openapi）。
- 旧版框架、弱化的参数校验、安全配置、缺少测试等，需要按现行实践补齐。

建议路线：先能独立解释、重写一条业务主链路，再逐步改造安全、异常、事务和测试。**不必在入门第一天为追新版本而重构全部代码。**

### 11.3 聊天学习的模型选择（非技术原理）

本次聊天最后明确的偏好是：**主要问技术概念和代码原理，不要求模型直接写完整项目。**

因此约定：

- 默认 **GPT-6 中思考强度**：技术概念、Spring 语法、普通代码讲解。
- 简单定义/查询：低思考也可。
- 复杂并发、事务失效、JVM 深层机制、系统架构权衡：切换高思考。

思考强度和讲解深度是两回事；中思考也可以要求详细解释和自检。

---

## 12. 容易混淆的概念速查

| 容易混淆的说法 | 更准确的理解 |
|---|---|
| “Spring MVC 就是服务器” | Spring MVC 是 Web 框架；典型项目用 Tomcat 提供 HTTP/Servlet 运行环境 |
| “Tomcat 必不可少” | 不是；普通 MVC 需要兼容的 Servlet 运行环境，但不局限于 Tomcat |
| “Servlet 就是 Tomcat” | Servlet 是规范/API；Tomcat 实现并运行它 |
| “注解自己就会执行代码” | 编译器或框架读取注解并执行对应逻辑 |
| “没有注解 Java 就无法运行” | 不对；很多功能可通过其他配置或手动代码实现 |
| “Spring IoC 容器就是 Servlet 容器” | 两者管理的对象和职责不同 |
| “Controller 直接负责网络收发” | 网络连接由底层服务器处理；Controller 处理映射到的业务入口 |
| “Java 方法返回对象，浏览器直接收到对象引用” | 通常序列化为 JSON 等响应体数据 |
| “Nginx 必须配 Tomcat” | 二者可组合，也可独立使用 |
| “导入接口文档就是实现接口” | 文档说明接口契约；接口代码仍需开发 |
| “Hash 就是加密，可解密还原” | 密码学哈希通常单向；加密通常可使用密钥还原 |
| “换 SHA-256 就能安全存密码” | 普通快速哈希不适合直接存密码，应使用密码哈希/KDF |

---

## 13. 复习题：能回答就说明主线基本打通

1. 浏览器访问 `http://localhost:8080/user/123`，从 URL 到 Java 方法调用，中间发生了哪些事？
2. IP 与端口分别解决什么问题？DNS、TCP、HTTP 各在哪一层起作用？
3. 如果只有 `ServerSocket`，要自己实现哪些 HTTP 功能？
4. Tomcat 和 Servlet 分别是什么关系？Servlet 容器管理哪些内容？
5. 为什么 Spring MVC 还需要 `DispatcherServlet`？
6. `HandlerMapping` 和 `HandlerAdapter` 各负责什么？
7. `@RestController`、`@GetMapping`、`@PathVariable` 读取和使用者是谁？
8. `@PathVariable`、`@RequestParam`、`@RequestBody` 如何区分？
9. Controller、Service、Mapper 的职责边界是什么？
10. MyBatis 的 Mapper 接口为什么不写实现类也能调用？
11. HTTP 响应 JSON 是怎么从 Java 对象变出来的？
12. Spring IoC 容器、Servlet 容器、Docker 容器为什么不能当作同一回事？
13. Nginx 解决什么问题？去掉 Nginx 后 Spring Boot 能不能工作？
14. 为什么 MD5、直接 SHA-256 都不适合存用户密码？bcrypt 如何校验密码？
15. 如果按照《苍穹外卖》学习，你应保留哪些架构思想、替换哪些旧做法？

### 推荐动手练习（约 30～60 分钟）

1. 创建一个最小的 Spring Boot Web 项目，实现 `GET /users/{id}`，返回 JSON。
2. 设置断点：`Controller → Service → Mapper`，观察 `id` 如何传递。
3. 试三个请求：合法 ID、不存在的 ID、无法转换为 `Long` 的 ID，观察状态码与返回结果。
4. 通过浏览器开发者工具 Network 面板查看请求行、请求头、响应头和 JSON。
5. 用 Apifox/Postman 发同样请求，确认 **HTTP 与前端框架是相互独立的**。

---

## 14. 最终自检与一句话总结

**自检要点：**

- 没有把 Tomcat 说成 Java 项目的强制依赖；明确了 Servlet 是规范，Tomcat 是其实现/运行容器之一。
- 没有把 Spring MVC、Spring Boot、Spring IoC 容器混为一谈。
- 区分了 Web 连接处理、MVC 方法映射、业务逻辑、数据库访问和 JSON 序列化。
- 区分了 Hash、密码哈希与可逆加密；没有建议使用 MD5 存密码。
- 示例中的网络、框架启动与业务代码是为理解而简化，具体行为受依赖、配置、协议和版本影响。

> **核心心智模型：Java Web 后端不是神秘的东西，它从“一个监听端口、收发数据的普通程序”开始，逐步叠加 HTTP 规范、Servlet 容器、Spring MVC 请求分发、Spring IoC 对象管理、Spring Boot 自动配置，以及数据库访问框架。你写的是业务逻辑，框架承担通用工作。**
