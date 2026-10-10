# 2026-09-19 学习笔记：Spring Boot 入门与 ICPC 赛程记录

> 整理范围：本次聊天中的 Spring Boot 入门讲解、实践建议，以及 2026 年 9—10 月 ICPC/CCPC 赛事信息。  
> 笔记整理日期：2026-10-10。比赛日程属于时效信息，以赛事官方最终公告为准。

## 一、Spring Boot 是什么

**Spring Boot 是基于 Spring 生态的快速开发框架**，核心价值是让开发者用更少的手工配置创建、运行 Spring 应用。

- **Spring**：提供 IoC 容器、依赖注入（DI）、AOP 等基础能力。
- **Spring MVC**：基于 Servlet 体系处理 Web 请求，包括路由、参数绑定与响应转换。
- **Spring Boot**：通过 Starter、自动配置、嵌入式服务器和统一配置机制，简化 Spring 应用的搭建和运行。
- **Tomcat**：常用的 Servlet 容器，负责 HTTP 连接与 Servlet 执行环境；使用 Web Starter 时通常作为嵌入式服务器引入。

一个典型的请求链：

```text
客户端（浏览器 / Postman）
    ↓ HTTP 请求
嵌入式 Tomcat
    ↓
Spring MVC（DispatcherServlet、HandlerMapping 等）
    ↓
Controller（参数接收 / 响应）
    ↓
Service（业务逻辑）
    ↓
Mapper / DAO（数据访问）
    ↓
MySQL
```

> 关键认识：**Spring Boot 不等于 Spring MVC，也不等于 Tomcat**。它负责自动配置、集成和启动各组件。

## 二、如何创建 Spring Boot 项目

推荐用 IntelliJ IDEA 的 Spring Boot 向导或 [Spring Initializr](https://start.spring.io/)：

1. 创建 **Maven + Java** 项目，选择与所用 Spring Boot 版本兼容的 JDK（入门常用 Java 17/21）。
2. 添加 Web 相关 Starter（名称视 Spring Boot 版本而定；传统项目常见 `spring-boot-starter-web`）。
3. 等待 Maven 下载依赖。
4. 运行带有 `@SpringBootApplication` 的启动类。
5. 访问本地接口，例如 `http://localhost:8080/hello`。

典型目录结构：

```text
demo/
├── pom.xml                     # Maven 依赖与构建配置
└── src/main/
    ├── java/com/example/demo/
    │   ├── DemoApplication.java
    │   ├── controller/
    │   ├── service/
    │   ├── dto/
    │   └── mapper/             # 接入数据库后使用
    └── resources/
        └── application.yml    # 配置文件
```

常见 Web Starter 依赖示例：

```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-web</artifactId>
</dependency>
```

**Starter 的含义**：一个为特定功能组织好的依赖入口。它通常会通过依赖传递引入相应组件；具体自动配置还取决于类路径、配置属性和条件判断。

## 三、启动类及运行过程

```java
package com.example.demo;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

@SpringBootApplication
public class DemoApplication {
    public static void main(String[] args) {
        SpringApplication.run(DemoApplication.class, args);
    }
}
```

启动时可粗略理解为：

```text
main()
  → SpringApplication.run(...)
  → 创建并刷新 ApplicationContext（Spring 容器）
  → 组件扫描与 Bean 注册 / 实例化 / 依赖注入
  → 执行适用的自动配置
  → 启动嵌入式 Web 服务器
  → 等待 HTTP 请求
```

`@SpringBootApplication` 综合了自动配置、组件扫描等能力。**默认从启动类所在包及其子包扫描组件**，所以建议将启动类放在项目根包。

## 四、Controller：编写 HTTP 接口

最小 GET 接口：

```java
package com.example.demo.controller;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class HelloController {
    @GetMapping("/hello")
    public String hello() {
        return "hello spring boot";
    }
}
```

启动后使用浏览器请求：

```http
GET http://localhost:8080/hello
```

响应体：

```text
hello spring boot
```

- `@RestController`：声明 REST 风格的 Controller；相当于 `@Controller` 与 `@ResponseBody` 的组合，使处理方法返回值通常直接写入 HTTP 响应体。
- `@GetMapping("/hello")`：将 GET `/hello` 请求映射到这个方法。
- `@RequestMapping("/users")`：可放在类上统一声明路径前缀。

### 三种常见参数接收方式

| 方式 | 典型请求 | Java 注解 | 用途 |
|---|---|---|---|
| 查询参数 | `GET /users?id=10` | `@RequestParam Long id` | URL 查询参数、表单参数 |
| 路径参数 | `GET /users/10` | `@PathVariable Long id` | REST 资源 ID |
| JSON 请求体 | `POST /login`，Body 为 JSON | `@RequestBody LoginRequest body` | 接收结构化数据 |

> **注意**：真实登录接口不要把密码放到 URL 查询串中。URL 可能进入历史记录、日志等；正式应用应使用 HTTPS、妥善保护凭据，并实现规范的身份验证机制。

## 五、接收 JSON：`@RequestBody`

客户端发送：

```http
POST /login
Content-Type: application/json

{
  "username": "roger",
  "password": "example-password"
}
```

Java 17+ 可先用 `record` 简化数据传输对象（DTO）：

```java
package com.example.demo.dto;

public record LoginRequest(String username, String password) {}
```

Controller 示例（**仅用于练习 JSON 映射，不实现真实登录验证**）：

```java
@PostMapping("/login")
public Map<String, String> login(@RequestBody LoginRequest request) {
    return Map.of("username", request.username(), "message", "request received");
}
```

此时 Spring MVC 通过 HTTP 消息转换机制（通常借助 Jackson）把 JSON 转成 Java 对象；返回的 `Map` 又会被序列化为 JSON。

## 六、Service 与依赖注入（DI）

**分层职责**：

- **Controller**：接收参数、调用业务服务、返回响应。
- **Service**：实现业务规则，例如校验用户是否存在、计算价格、操作多个业务对象。
- **Mapper/DAO**：与数据库交互；接入 MyBatis 后通常由 Mapper 负责 SQL 映射。

一个可运行的 Controller → Service 示例：

`service/UserService.java`：

```java
package com.example.demo.service;

import org.springframework.stereotype.Service;

@Service
public class UserService {
    public String getUser(Long id) {
        return "用户 id = " + id;
    }
}
```

`controller/UserController.java`：

```java
package com.example.demo.controller;

import com.example.demo.service.UserService;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/users")
public class UserController {
    private final UserService userService;

    public UserController(UserService userService) {
        this.userService = userService;
    }

    @GetMapping("/{id}")
    public String getUser(@PathVariable Long id) {
        return userService.getUser(id);
    }
}
```

访问：

```http
GET http://localhost:8080/users/10
```

响应：

```text
用户 id = 10
```

### IoC 和 DI 的区别

- **IoC（控制反转）**：对象的创建、组织和生命周期管理主要交由容器负责，而非业务代码到处 `new`。
- **DI（依赖注入）**：容器把所需的对象传入另一个对象；构造器注入是推荐的常见方式。

```text
Spring 创建 UserService Bean
           ↓
Spring 创建 UserController Bean
           ↓
通过 UserController(UserService userService) 注入依赖
           ↓
请求 /users/10 时调用 userService.getUser(10L)
```

`final` 字段可以在构造器里完成一次赋值，这也是构造器注入与不可变依赖配合良好的原因。

## 七、配置文件 `application.yml`

修改服务端口：

```yaml
server:
  port: 8081
```

此后访问地址改为 `http://localhost:8081`。

接入数据库时会用到类似配置（示意，需同时引入 JDBC/MyBatis 等依赖与数据库驱动）：

```yaml
spring:
  datasource:
    url: jdbc:mysql://localhost:3306/java_app
    username: ${DB_USERNAME:root}
    password: ${DB_PASSWORD}
```

- `${DB_USERNAME:root}`：优先读取环境变量，没有时示例中默认 `root`。
- `${DB_PASSWORD}`：从环境变量读取密码；不要把真实数据库密码提交到 Git 仓库。
- **仅配置数据源并不会自动完成 CRUD**，还需要数据库、驱动、MyBatis/JDBC 及相应代码。

## 八、需要先认识的常见注解

| 注解 | 主要作用 |
|---|---|
| `@SpringBootApplication` | 标记 Spring Boot 启动类 |
| `@RestController` | 声明返回响应数据的 Web Controller |
| `@RequestMapping` | 声明请求路径等映射规则 |
| `@GetMapping` | 处理 GET 请求 |
| `@PostMapping` | 处理 POST 请求 |
| `@PutMapping` | 处理 PUT 请求 |
| `@DeleteMapping` | 处理 DELETE 请求 |
| `@RequestBody` | 将请求体转换为 Java 参数 |
| `@RequestParam` | 绑定请求参数 |
| `@PathVariable` | 绑定 URL 路径变量 |
| `@Service` | 标记业务服务组件，使其被组件扫描发现 |
| `@Component` | 通用 Spring 组件标记 |

学习方法：先知道**用途和请求链位置**，再在练习中熟悉语法，不必一开始钻研每个注解源码。

## 九、当前阶段的练习任务与验收标准

建议先自己完成一个**不接数据库的用户查询 API**，再接入 MySQL。

### 练习 A：Web 接口

- [ ] 用 IDEA / Spring Initializr 创建 Maven 项目，并成功启动。
- [ ] 实现 `GET /hello`，浏览器能看到字符串。
- [ ] 实现 `GET /users/{id}`，利用 `@PathVariable` 获取用户 ID。
- [ ] 实现 `GET /users?id=10`，利用 `@RequestParam` 获取查询参数。
- [ ] 实现 `POST /login`，利用 `@RequestBody` 接收 JSON，并返回不含密码的 JSON。
- [ ] 通过 `application.yml` 把端口从 8080 改成 8081。

### 练习 B：分层与 IoC/DI

- [ ] 把 `getUser` 逻辑从 Controller 移到 `@Service`。
- [ ] 使用构造器注入 `UserService`，不手动 `new UserService()`。
- [ ] 能口述完整调用链：HTTP → Tomcat → Spring MVC → Controller → Service → HTTP 响应。
- [ ] 能说清 Spring、Spring MVC、Spring Boot、Tomcat 的分工。

### 下一阶段顺序

```text
Maven 和 Spring Boot 基础
    ↓
HTTP 接口与 JSON
    ↓
Controller / Service 分层及 DI
    ↓
MySQL 基础 SQL
    ↓
MyBatis / Mapper
    ↓
CRUD API + 参数校验 + 统一异常处理
    ↓
事务、Redis、JWT 等进阶主题
```

**现阶段先不深挖**：Spring 源码、复杂 AOP、微服务、Spring Cloud。对于准备 Java 后端实践的人，能独立写出可测试的 API 比只记住概念更重要。

## 十、ICPC/CCPC 赛程记录（本次聊天的附加话题）

> 以下是本次聊天在 **2026-09-19** 提到的赛事日期记录，**不是长期稳定知识点**。报名资格、举办时间、现场赛安排以 [ICPC 北京总部](https://icpc.pku.edu.cn/) 和各赛站官方通知为准；截至本笔记整理日（2026-10-10），9 月赛事均已是过去的赛事。

| 日期 | 聊天中提及的赛事 | 截至 2026-10-10 |
|---|---|---|
| 9 月 6 日 | ICPC Asia EC 网络预选赛第一场 | 已过去 |
| 9 月 12 日 | ICPC Asia EC 网络预选赛第二场 | 已过去 |
| 9 月 19 日 | CCPC 网络预选赛 | 已过去 |
| 9 月 19 日 | ICPC Asia Taiwan Online Programming Contest | 已过去 |
| 10 月 17—18 日 | ICPC 西安站 | 聊天记录中的安排，待核对 |
| 10 月 24—25 日 | ICPC 成都站 | 聊天记录中的安排，待核对 |
| 10 月 31 日—11 月 1 日 | ICPC 武汉站 | 聊天记录中的安排，待核对 |
| 11 月 7—8 日 | ICPC 南京站 | 聊天记录中的安排，待核对 |
| 11 月 14—15 日 | ICPC 沈阳站 | 聊天记录中的安排，待核对 |
| 12 月 5—6 日 | ICPC 上海站 | 聊天记录中的安排，待核对 |
| 12 月 19—20 日 | ICPC 南昌站 | 聊天记录中的安排，待核对 |

聊天中给出的参考链接：

- [ICPC 北京总部](https://icpc.pku.edu.cn/)
- [XCPC 赛程整理（第三方）](https://lus-jlu.github.io/xcpc)
- [ICPC Asia Taiwan Online Programming Contest](https://topc.icpc.tw/)

## 十一、自测问题

1. Spring Boot、Spring MVC、Tomcat 分别做什么？
2. 为什么添加一个 Web Starter，就不需要手动引入许多依赖？
3. `SpringApplication.run(...)` 启动后，应用的大致运行流程是什么？
4. `@RestController` 和 `@Service` 的职责有什么不同？
5. `@RequestParam`、`@PathVariable` 和 `@RequestBody` 分别处理什么参数？
6. 容器怎么把 `UserService` 注入 `UserController`？
7. 为什么不建议把业务逻辑全部塞到 Controller？
8. 仅配置 `spring.datasource`，就能自动进行数据库 CRUD 吗？为什么？

**一句话总结：** 目前先把 **“创建 Spring Boot 项目 → 接收 HTTP 请求 → Controller 调用 Service → 返回 JSON”** 写熟，再学习 MySQL + MyBatis，把假数据替换为真实数据库数据。
