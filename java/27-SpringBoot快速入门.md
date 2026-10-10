# 27 · Spring Boot 快速入门

> 一句话概括：Spring Boot 用 Starter 依赖、自动配置、内嵌服务器和统一配置文件，把"搭建并启动一个 Spring 应用"的成本降到极低。

## 一、定位：它不是 Spring 的替代品

| 技术 | 解决的问题 |
|---|---|
| Spring Framework | IoC、DI、AOP、事务等基础设施 |
| Spring MVC | 处理 HTTP 请求（Spring 的 Web 模块） |
| Spring Boot | 自动配置 + Starter 依赖 + 内嵌服务器，简化搭建、配置、启动 |

**Spring Boot ≠ Spring MVC ≠ Tomcat**：它负责把各组件整合、配好、启动。`spring-boot-starter-web` 会经依赖传递引入内嵌 Tomcat，无需单独部署。

## 二、创建项目

用 IDEA 向导或 [Spring Initializr](https://start.spring.io/)：Maven + Java（选与 Spring Boot 版本兼容的 JDK，入门用 17/21）→ 加 Web Starter → 等依赖下载 → 运行启动类 → 访问 `http://localhost:8080/hello`。

```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-web</artifactId>
</dependency>
```

**Starter = 为特定功能组织好的依赖入口**，经传递依赖引入组件；具体自动配置取决于类路径、配置属性和条件判断。

典型结构：`pom.xml` + `src/main/java/.../{DemoApplication, controller/, service/, dto/, mapper/}.java` + `resources/application.yml`。

## 三、启动类与运行过程

```java
@SpringBootApplication
public class DemoApplication {
    public static void main(String[] args) {
        SpringApplication.run(DemoApplication.class, args);
    }
}
```

```text
main() → 创建并刷新 ApplicationContext（容器）
       → 组件扫描、Bean 注册/实例化/依赖注入
       → 执行适用的自动配置
       → 启动内嵌 Web 服务器 → 等待 HTTP 请求
```

`@SpringBootApplication` 组合了配置、自动配置、组件扫描。**默认扫描启动类所在包及其子包**——启动类要放项目根包，否则加了 `@Service` 也不一定能注册成 Bean。

## 四、最小接口与 JSON

```java
@RestController
public class HelloController {
    @GetMapping("/hello")
    public String hello() { return "hello spring boot"; }
}
```

JSON 接收（Java 17+ 用 record 写 DTO）：

```java
public record LoginRequest(String username, String password) {}

@PostMapping("/login")
public Map<String, String> login(@RequestBody LoginRequest request) {
    return Map.of("username", request.username(), "message", "request received");
}
```

Spring MVC 经 Jackson 把 JSON ↔ Java 对象互转。真实登录接口**不要把密码放 URL 查询串**（会进历史记录和日志），要 HTTPS + 规范认证机制（见 `33`）。

## 五、配置文件 application.yml

```yaml
server:
  port: 8081            # 改端口
spring:
  datasource:           # 接数据库时用（还需 JDBC/MyBatis 依赖与驱动）
    url: jdbc:mysql://localhost:3306/java_app
    username: ${DB_USERNAME:root}   # 环境变量优先，缺省 root
    password: ${DB_PASSWORD}        # 真实密码走环境变量，别提交进 Git
```

**只配 datasource 不会自动 CRUD**——还需要驱动、MyBatis/JDBC 和相应代码。

## 六、学习路线与版本选择

```text
Maven/Spring Boot 基础 → HTTP 接口与 JSON → 分层 + DI
→ MySQL 基础 SQL → MyBatis/Mapper → CRUD + 校验 + 统一异常
→ 事务、Redis、JWT 进阶
```

- **Java 版本选 21**：现代特性 + 配 Spring Boot 3.x 稳妥；不必从 Java 8 逐版升级，也别以为企业都用最新版。
- 学《苍穹外卖》类项目：保留其请求链/分层/MyBatis/事务思想，**替换旧做法**——MD5 存密码改 bcrypt/Argon2id，旧 Swagger 改 springdoc-openapi。
- 现阶段不深挖：Spring 源码、复杂 AOP、微服务/Spring Cloud。能独立写出可测试的 API 比背概念重要。

## 📌 易错点

- "Spring Boot 就是 Tomcat/就是 Spring MVC"——三者分工不同（见第一节）。
- 启动类放错包 → 组件扫描不到，Bean 注册失败。
- "加了 starter-web 就一定能跑"——还要启动类和正确的目录结构。
- "配了 datasource 就能查库"——配置不等于实现。
- 数据库密码写进 yml 提交 Git——应该用环境变量。

## 🧠 速记口诀

```text
Starter 一键引依赖，自动配置看类路径
启动类放根包，扫描范围就是它和子包
yml 改端口、连数据库；密码走环境变量
版本选 21，跟 Spring Boot 3.x 走
```

## ✅ 自测问题

1. Spring Boot、Spring MVC、Tomcat 分别做什么？为什么说不必单独装 Tomcat？
2. 为什么启动类建议放在项目根包？
3. `SpringApplication.run(...)` 之后应用大致经历哪些阶段？
4. `@RequestParam`、`@PathVariable`、`@RequestBody` 各收什么参数？
5. 仅配置 `spring.datasource` 能自动完成 CRUD 吗？为什么？

> 来源：原始《2026-09-19 学习笔记：Spring Boot 入门与 ICPC 赛程记录》(20260919_SpringBoot入门与ICPC赛程_学习笔记.md，其中 ICPC 赛程为时效信息不入知识体系)、《Java 后端学习笔记：从 HTTP 请求到 Spring Boot》(Java后端学习笔记_从HTTP请求到SpringBoot.md §11)。
