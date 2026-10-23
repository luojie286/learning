# Spring Boot 使用与底层执行原理：聊天核对版学习笔记

> 核对日期：2026-10-23  
> 范围：本次对话中关于 Spring Boot、Spring MVC、Tomcat、IoC/DI、自动配置、Bean 生命周期、Filter/Interceptor/AOP、事务、并发与 MyBatis 的说明和示例。  
> 示例技术栈：**Java 21 + Spring Boot 3.5.16 + Maven + Spring MVC + 内嵌 Tomcat**。  
> 说明：示例专门用于理解执行过程，使用内存数据，不是含鉴权、完整校验、事务、数据库的生产模板。源码逻辑和依赖版本已核对官方文档；**未在本环境完成 Maven 编译与实际 HTTP 集成测试**。

## 目录

1. [本次聊天准确性核对与纠错](#1-本次聊天准确性核对与纠错)
2. [Spring Boot、Spring Framework、Spring MVC、Tomcat 的关系](#2-spring-bootspring-frameworkspring-mvctomcat-的关系)
3. [完整可复制的 Spring Boot 示例](#3-完整可复制的-spring-boot-示例)
4. [Spring Boot 启动时的底层执行流程](#4-spring-boot-启动时的底层执行流程)
5. [IoC、DI、BeanDefinition 与 Bean 生命周期](#5-iocdibeandefinition-与-bean-生命周期)
6. [Starter、组件扫描和自动配置](#6-starter组件扫描和自动配置)
7. [一次 HTTP 请求的完整执行路径](#7-一次-http-请求的完整执行路径)
8. [常用 Spring 注解速查](#8-常用-spring-注解速查)
9. [Filter、Interceptor、AOP 和事务](#9-filterinterceptoraop-和事务)
10. [单例 Controller 与并发安全](#10-单例-controller-与并发安全)
11. [数据库访问与 MyBatis 代理](#11-数据库访问与-mybatis-代理)
12. [配置、调试与源码追踪](#12-配置调试与源码追踪)
13. [练习路线、典型面试题和自测](#13-练习路线典型面试题和自测)
14. [官方参考资料](#14-官方参考资料)

---

## 1. 本次聊天准确性核对与纠错

**总体结论：核心概念和执行主线正确，可作为入门基础；一些表述是教学简化，部分需要限定适用前提。下表区分“正确”“需要补充”“需要修正”。**

| 聊天中的说法或示例 | 判定 | 核对与更严谨的表述 |
|---|---|---|
| Spring Boot 构建在 Spring Framework 之上，不替代 Spring | ✅ 正确 | Spring Boot 主要提供自动配置、Starter、启动与运维集成；IoC、AOP、MVC 等仍来自 Spring 生态中的相应模块。 |
| `@SpringBootApplication` 整合三个核心注解 | ✅ 正确 | 核心为 `@SpringBootConfiguration`、`@EnableAutoConfiguration`、`@ComponentScan`。 |
| Spring Web Starter 引入 MVC、Tomcat、JSON 能力 | ✅ 正确（限定版本） | **Boot 3.5** 通常使用 `spring-boot-starter-web`；**Boot 4.x** 推荐 `spring-boot-starter-webmvc`，旧名称在 4.x 文档中为 deprecated。 |
| 前后两次 `pom.xml` 分别使用 3.5.16 和 3.5.7 | ⚠️ 版本不统一 | 这两个版本都真实存在，不是凭空捏造；为避免复制时混用，本笔记统一为 **3.5.16**。官方 2026-06-25 公告表明，它是 3.5.x **最后的 OSS 版本**；新生产项目应优先评估有持续支持的版本（如 4.x），而非直接照搬。 |
| 只要引入 JAR，相关 Bean 都会自动创建 | ❌ 若如此理解则不对 | 依赖只提供类；自动配置还受 `@Conditional*`、Web 应用类型、用户自定义 Bean 等条件限制。此前讲解已经指出了这一点。 |
| 扫描 `@Service`、`@RestController` 后自动创建实例 | ✅ 主线正确 | **扫描/注册 BeanDefinition** 与 **Bean 实例化** 不同。实例通常在容器刷新、创建单例时完成，也可能因懒加载等设置而推迟。 |
| Spring 启动按照七个步骤严格顺序执行 | ⚠️ 教学简化 | 启动图是逻辑阶段。自动配置解析、Bean 定义注册、Tomcat 创建、Bean 初始化等步骤会在 `ApplicationContext.refresh()` 的多个子阶段交错发生，不能当作精确源码调用顺序。 |
| Bean 生命周期写成“BeanPostProcessor 前置 → `@PostConstruct` → 后置” | ⚠️ 需要修正 | `@PostConstruct` 一般由 `CommonAnnotationBeanPostProcessor` 在 **初始化前处理阶段** 调用，而不是所有前置处理器执行完后才统一进入独立阶段。不同处理器有自己的排序。正确的简化图见第 5 节。 |
| `@RestController` 相当于 `@Controller` + `@ResponseBody` | ✅ 正确 | 返回值并非一定为 JSON，而是经返回值处理器和合适的 `HttpMessageConverter`，依据类型、配置和内容协商写出响应。示例中返回 JSON。 |
| Tomcat 接收请求并找到对应 Controller 方法 | ⚠️ 需区分职责 | Tomcat 在 Servlet 层完成请求处理与分发；**`DispatcherServlet` + `HandlerMapping` + `HandlerAdapter`** 才负责 MVC Handler 匹配、参数解析和 Controller 方法调用。 |
| `HandlerInterceptor.postHandle()` 在响应最终写出之前执行 | ⚠️ 容易误会 | 对 `@ResponseBody`/`ResponseEntity` 方法，**响应可能在 HandlerAdapter 内就已写入/提交**，因此 `postHandle` 不适合在此时修改响应体或追加 Header；可考虑 `ResponseBodyAdvice` 等。 |
| `@Transactional` 加在方法上自动实现事务 | ⚠️ 缺前提 | 必须有适用的事务管理基础设施、正确的代理调用链及实际受事务管理的资源。本笔记的纯 `Map` 示例**没有数据库事务**，不能用它验证事务回滚。 |
| `@Transactional` 在同类方法自调用时可能失效 | ✅ 正确 | 默认 **proxy 模式**仅拦截经过代理的调用；目标对象内部通过 `this.method()` 调用通常绕过代理。 |
| `@Transactional` 默认运行时异常回滚、受检异常不回滚 | ✅ 默认规则正确 | 默认 `RuntimeException`、`Error` 触发回滚；checked exception 通常不触发。可以用 `rollbackFor` 或框架支持的全局策略改变。 |
| 默认 Controller/Service 为 singleton | ✅ 正确（限定范围） | 是**每个容器、每个 Bean 定义**的 singleton，不表示整个 JVM 或所有容器中同类型对象只有一个。 |
| `ConcurrentHashMap` 保证线程安全 | ⚠️ 需补充 | 它保护单次 Map 操作；不能自动保证跨多个操作的业务原子性，且本示例数据会随进程重启丢失。 |
| `@Value("${server.port}")` 读取默认 8080 | ⚠️ 隐含风险 | **默认值不一定作为属性显式存在于 `Environment` 中**；如果配置文件没有设置 `server.port`，这个占位符可能解析失败。可写 `@Value("${server.port:8080}")`。这读取的是配置值，不一定是随机端口模式下的实际监听端口。 |
| `@ConfigurationProperties` 注解一加就能自动注入 | ⚠️ 需要补充 | 需要通过 `@ConfigurationPropertiesScan`、`@EnableConfigurationProperties` 或已注册的 `@Bean` 等机制完成注册。 |
| `@Mapper` 的接口可以调用数据库 | ✅ 原理正确（须配置） | 需要 MyBatis/MyBatis-Plus 依赖、数据源和 Mapper 扫描/注册；框架为接口创建 Mapper 代理。不是普通 Java 接口天然能够执行 SQL。 |
| 内存版示例“完整可运行” | ✅ 结构基本完整 | 适合学习 HTTP 链路，但**未做输入校验、持久化、授权、接口测试**；客户端传入非法内容可能创建无效菜品。不可直接称作生产方案。 |

**特别注意：**前面的回答中出现过带有 `id="..."` 的代码围栏，这是展示/渲染层的问题；下方笔记统一采用标准 Markdown 三反引号代码块，方便在 Obsidian、Typora、VS Code 阅读。

---

## 2. Spring Boot、Spring Framework、Spring MVC、Tomcat 的关系

| 组件 | 主要职责 | 不要误解成 |
|---|---|---|
| Spring Framework | IoC 容器、依赖注入、AOP、事务基础设施、事件等 | “负责直接监听 HTTP 端口” |
| Spring Boot | 启动引导、自动配置、Starter、嵌入式服务器集成、配置管理 | “取代 Spring Framework” |
| Spring MVC | HTTP 请求映射、Controller 调用、参数绑定、响应处理 | “就是 Tomcat” |
| Tomcat | Servlet 容器、HTTP 连接和请求处理 | “自动理解所有 `@GetMapping`” |
| Jackson | Java 对象与 JSON 的序列化/反序列化 | “Spring Boot 核心代码自己解析 JSON” |
| Maven | 构建、依赖解析、打包 | “在运行时判断自动配置条件” |

**一句话串联：**Maven 准备依赖 → Spring Boot 引导并组装应用 → Spring IoC 创建/管理 Bean → Tomcat 接收 HTTP 请求 → Spring MVC 调用 Controller → Service 执行业务 → 返回 JSON。

> 这里的链路以 **Servlet 栈（Spring MVC + Tomcat）** 为准；WebFlux 的处理模型不同。

---

## 3. 完整可复制的 Spring Boot 示例

### 3.1 项目结构

```text
food-demo/
├── pom.xml
└── src/
    └── main/
        ├── java/
        │   └── com/example/food/
        │       ├── FoodApplication.java
        │       ├── controller/
        │       │   └── DishController.java
        │       ├── service/
        │       │   └── DishService.java
        │       └── model/
        │           ├── Dish.java
        │           └── CreateDishRequest.java
        └── resources/
            └── application.yml
```

### 3.2 `pom.xml`：引入 Spring Web Starter

```xml
<project xmlns="http://maven.apache.org/POM/4.0.0"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://maven.apache.org/POM/4.0.0 https://maven.apache.org/xsd/maven-4.0.0.xsd">

    <modelVersion>4.0.0</modelVersion>

    <parent>
        <groupId>org.springframework.boot</groupId>
        <artifactId>spring-boot-starter-parent</artifactId>
        <version>3.5.16</version>
        <relativePath/>
    </parent>

    <groupId>com.example</groupId>
    <artifactId>food-demo</artifactId>
    <version>0.0.1-SNAPSHOT</version>

    <properties>
        <java.version>21</java.version>
    </properties>

    <dependencies>
        <dependency>
            <groupId>org.springframework.boot</groupId>
            <artifactId>spring-boot-starter-web</artifactId>
        </dependency>
    </dependencies>

    <build>
        <plugins>
            <plugin>
                <groupId>org.springframework.boot</groupId>
                <artifactId>spring-boot-maven-plugin</artifactId>
            </plugin>
        </plugins>
    </build>
</project>
```

- `spring-boot-starter-parent` 管理一系列依赖与构建插件的默认配置。
- `spring-boot-starter-web` 为 Boot **3.5** 引入 Spring MVC、默认 Tomcat、JSON 等 Web 应用所需依赖。
- Maven 的**传递依赖**与 Spring Boot 的**自动配置**是两个阶段：前者确定 JAR，后者在运行时根据条件配置 Bean。
- Spring Boot 3.5.16 是官方宣布的 3.5.x 最后一版 OSS 发布，本版本用于和聊天中的 3.x 示例对照学习，不代表推荐新项目生产选型。

### 3.3 `FoodApplication.java`：应用入口

```java
package com.example.food;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

@SpringBootApplication
public class FoodApplication {

    public static void main(String[] args) {
        SpringApplication.run(FoodApplication.class, args);
    }
}
```

`SpringApplication.run()` 会准备环境、创建和刷新 Spring 容器、整合 Web 服务器、发布启动生命周期事件等；不是简单地执行一个业务函数。

### 3.4 `Dish.java`：返回的数据模型

```java
package com.example.food.model;

public record Dish(
        Long id,
        String name,
        Integer price
) {}
```

`record` 为 Java 提供的不可变数据载体语法；**这里金额单位为“分”**，例如 `2200` 表示 22 元。项目真正涉及货币计算时，须根据需求选择适当的金额类型和转换规则。

### 3.5 `CreateDishRequest.java`：请求 DTO

```java
package com.example.food.model;

public record CreateDishRequest(
        String name,
        Integer price
) {}
```

DTO（Data Transfer Object）用来承载传输数据。本例未添加 `@NotBlank`、`@NotNull`、`@Positive` 等校验约束，因此只适合教学。**`@RequestBody` 不等于自动验证所有字段。**

### 3.6 `DishService.java`：业务逻辑

```java
package com.example.food.service;

import com.example.food.model.Dish;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.web.server.ResponseStatusException;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

@Service
public class DishService {

    private final Map<Long, Dish> dishes =
            new ConcurrentHashMap<>();

    private final AtomicLong sequence =
            new AtomicLong(2);

    public DishService() {
        dishes.put(1L, new Dish(1L, "宫保鸡丁", 2200));
        dishes.put(2L, new Dish(2L, "牛肉饭", 2800));
    }

    public Dish getById(Long id) {
        Dish dish = dishes.get(id);
        if (dish == null) {
            throw new ResponseStatusException(
                    HttpStatus.NOT_FOUND,
                    "菜品不存在"
            );
        }
        return dish;
    }

    public Dish create(String name, Integer price) {
        Long id = sequence.incrementAndGet();
        Dish dish = new Dish(id, name, price);
        dishes.put(id, dish);
        return dish;
    }
}
```

讲解：

- `@Service` 使类成为候选 Spring 组件；位于默认扫描路径中时能由容器注册管理。
- `ConcurrentHashMap` 让独立的读写 Map 操作具备并发安全性；`AtomicLong` 保证 ID 增长的原子性。
- **局限**：数据仅在内存；重启丢失；参数可能为空或不合法；没有数据库事务或跨操作原子性保证。

### 3.7 `DishController.java`：HTTP 接口

```java
package com.example.food.controller;

import com.example.food.model.CreateDishRequest;
import com.example.food.model.Dish;
import com.example.food.service.DishService;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/dishes")
public class DishController {

    private final DishService dishService;

    public DishController(DishService dishService) {
        this.dishService = dishService;
    }

    @GetMapping("/{id}")
    public Dish getById(@PathVariable("id") Long id) {
        return dishService.getById(id);
    }

    @PostMapping
    public ResponseEntity<Dish> create(
            @RequestBody CreateDishRequest request) {

        Dish dish = dishService.create(
                request.name(),
                request.price()
        );

        return ResponseEntity
                .status(HttpStatus.CREATED)
                .body(dish);
    }
}
```

讲解：

- `@RestController` = 带有 `@ResponseBody` 语义的 `@Controller`。
- `@RequestMapping("/api/dishes")` 声明类级别基础路径。
- `@GetMapping("/{id}")` 匹配 `GET /api/dishes/1`。
- `@PathVariable("id")` 从路径模板中提取 ID 并转换成 `Long`。
- `@PostMapping` 与 `@RequestBody` 处理 JSON 请求体；`ResponseEntity` 可显式设置 HTTP 状态码。
- 构造器没有标 `@Autowired`：对于只有一个构造器的 Bean，这通常不需要。

### 3.8 `application.yml`：外部配置

```yaml
server:
  port: 8080

spring:
  application:
    name: food-demo
```

生产环境一般还会配置不同的 Profile、数据库、连接池、日志和安全参数；不要把密钥提交到 Git 仓库。

### 3.9 启动和测试

需要本机 JDK 21 与 Maven；如果使用 Spring Initializr 生成的 Maven Wrapper，也可以用 `mvnw.cmd`（Windows）或 `./mvnw`（macOS/Linux）。

```bash
mvn spring-boot:run
```

查询：

```http
GET /api/dishes/1 HTTP/1.1
Host: localhost:8080
```

预期响应体：

```json
{
  "id": 1,
  "name": "宫保鸡丁",
  "price": 2200
}
```

创建：

```http
POST /api/dishes HTTP/1.1
Host: localhost:8080
Content-Type: application/json

{
  "name": "咖喱饭",
  "price": 2500
}
```

预期：HTTP **201 Created**，响应体：

```json
{
  "id": 3,
  "name": "咖喱饭",
  "price": 2500
}
```

Windows PowerShell 可以使用显式的 `curl.exe`，避免旧版 PowerShell 下 `curl` 与 `Invoke-WebRequest` 别名混淆：

```powershell
curl.exe -i http://localhost:8080/api/dishes/1
curl.exe -i -X POST http://localhost:8080/api/dishes -H "Content-Type: application/json" -d '{"name":"咖喱饭","price":2500}'
```

> PowerShell 对引号的处理会因版本、shell 与 native argument passing 设置而有所差异。如果 POST 命令解析异常，优先使用 Apifox/Postman，或把 JSON 保存到 `body.json` 后调用 `curl.exe --data-binary @body.json`。

### 3.10 可选增强：请求参数校验（不属于上面最小可运行示例）

在 `pom.xml` 额外添加：

```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-validation</artifactId>
</dependency>
```

将 DTO 改成：

```java
package com.example.food.model;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Positive;

public record CreateDishRequest(
        @NotBlank String name,
        @NotNull @Positive Integer price
) {}
```

Controller 的 POST 方法参数改为：

```java
public ResponseEntity<Dish> create(
        @jakarta.validation.Valid @RequestBody CreateDishRequest request) {
    // 其余方法体不变
}
```

含义：由 Jakarta Bean Validation 在 Spring MVC 参数绑定之后执行校验。上面只是局部修改示例，不应和最小样例的完整代码混作同一个未说明依赖的版本。

---

## 4. Spring Boot 启动时的底层执行流程

### 4.1 宏观执行链

```text
JVM 启动，执行 main()
        ↓
SpringApplication.run(FoodApplication.class, args)
        ↓
准备 Environment / 属性来源 / Profile
        ↓
根据应用类型创建 ApplicationContext
        ↓
注册主配置类及相关引导信息
        ↓
ApplicationContext.refresh()
        ├─ 处理配置类、组件扫描、自动配置和 BeanDefinition
        ├─ 注册 BeanPostProcessor 等容器扩展点
        ├─ Web 容器的创建、初始化及 Servlet 注册
        └─ 实例化并初始化非懒加载单例 Bean 等
        ↓
完成 WebServer 启动与容器刷新相关步骤
        ↓
调用 ApplicationRunner / CommandLineRunner（如果定义）
        ↓
发布 ApplicationReadyEvent 等启动完成信号
        ↓
应用持续接受请求
```

**警告：**这是宏观逻辑链，不能当作 `refresh()` 内部各方法的一一对应且严格串行的源码调用图。

### 4.2 `SpringApplication` 是什么？

它是 Spring Boot 的应用引导入口；不是 IoC 容器本身。

```java
SpringApplication application =
        new SpringApplication(FoodApplication.class);
application.run(args);
```

作用包括根据启动参数准备应用环境、选择上下文类型、驱动容器刷新、触发启动生命周期等。

### 4.3 `Environment` 与配置优先级

例如配置文件中：

```yaml
server:
  port: 8080
```

命令行传入：

```bash
java -jar food-demo-0.0.1-SNAPSHOT.jar --server.port=9090
```

默认属性来源优先级下，命令行参数通常覆盖配置文件里的 `server.port`，故最终配置端口为 9090。Spring Boot 对属性来源的管理不等于简单逐行读取 YAML 后直接修改 Tomcat 对象。

### 4.4 `ApplicationContext` 是什么？

Spring IoC 容器接口，负责 Bean 定义、Bean 生命周期、依赖解析、事件等功能。常见 Web 应用实现包括 `AnnotationConfigServletWebServerApplicationContext`；其底层利用 `ServletWebServerFactory` 创建内嵌服务器。

### 4.5 `@SpringBootApplication` 展开

```java
@SpringBootConfiguration
@EnableAutoConfiguration
@ComponentScan
```

- `@SpringBootConfiguration`：声明 Spring Boot 配置类。
- `@ComponentScan`：默认从启动类所在包及子包扫描受支持的 Spring 组件。
- `@EnableAutoConfiguration`：允许 Spring Boot 加载候选自动配置类，并按条件选择生效配置。

建议将启动类放在业务代码包的根部。例如主类在 `com.example.food`，Controller 和 Service 放在其子包中。

### 4.6 Web 服务器如何出现？

```text
Web Starter 提供相关类路径依赖
        ↓
Spring Boot 判断当前为 Servlet Web 应用
        ↓
自动配置 ServletWebServerFactory 等组件
        ↓
ServletWebServerApplicationContext 利用工厂创建 Tomcat
        ↓
注册 DispatcherServlet、Filter 等 Servlet 组件
        ↓
容器初始化并启动 WebServer
        ↓
监听配置的端口，接收 HTTP 请求
```

Spring Boot **没有凭空造出 Tomcat**：Tomcat 的实现由依赖提供；Boot 负责配置、创建和管理。Jetty 等服务器也可作为替代，但需调整依赖/配置。

---

## 5. IoC、DI、BeanDefinition 与 Bean 生命周期

### 5.1 IoC 与 DI 不同

- **IoC（控制反转）**：把对象的创建、装配与生命周期管理控制权交给容器。
- **DI（依赖注入）**：容器把一个组件所需的依赖提供给它，是实现 IoC 的常见方式。

手动方式：

```java
DishService service = new DishService();
DishController controller = new DishController(service);
```

Spring 方式：

```java
@Service
class DishService { /* ... */ }

@RestController
class DishController {
    private final DishService service;

    DishController(DishService service) {
        this.service = service;
    }
}
```

第二段为**局部机制示例**：完整可运行版本见第 3 节。

### 5.2 BeanDefinition ≠ Bean

```text
组件扫描 / @Bean / @Import / 自动配置等
                  ↓
              BeanDefinition
     （类型、作用域、创建信息等元数据）
                  ↓
            容器按需实例化
                  ↓
            Bean 实例及代理
```

**注册定义**不等于**马上创建对象**；某些对象可能延迟创建，初始化顺序也受到依赖关系影响。

### 5.3 Bean 生命周期（修正版）

```text
BeanDefinition 已注册
        ↓
实例化 Bean（构造器/工厂方法）
        ↓
属性注入、相关 Aware 回调等
        ↓
BeanPostProcessor.postProcessBeforeInitialization(...)
        │
        └─ 常见 @PostConstruct 在这一初始化前处理机制中触发
        ↓
InitializingBean.afterPropertiesSet() / 自定义 init-method 等
        ↓
BeanPostProcessor.postProcessAfterInitialization(...)
        │
        └─ 某些 AOP 代理在相应后处理机制中被创建
        ↓
Bean 可用（实际注入/暴露时可能涉及其他机制）
        ↓
容器正常关闭：执行符合条件的销毁回调
```

注意：实际生命周期还涉及多个处理器的排序、提前暴露代理、Scope 等细节；不能把这张图理解成对所有 Bean 都严格完全相同。

### 5.4 `@Component` 与 `@Bean`

| | `@Component` | `@Bean` |
|---|---|---|
| 放在哪里 | 类上 | 方法上 |
| 典型作用 | 让组件扫描发现类 | 用方法返回对象注册 Bean |
| 适合什么 | 自己的 Controller、Service、工具类 | 需要定制创建的对象、第三方类 |

```java
package com.example.food.config;

import java.time.Clock;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class AppConfig {

    @Bean
    public Clock clock() {
        return Clock.systemUTC();
    }
}
```

`@Bean` 可以出现在被 Spring 处理的配置类等位置，**并不意味着只有配了 `@Configuration` 才能注册 Bean**；完整 `@Configuration` 语义和其他模式还存在区别。

### 5.5 Singleton 的意义

一般默认是一个 Spring 容器、一个 Bean 定义对应一个共享实例。多个 HTTP 请求可以通过不同线程使用同一个 Controller 实例。**单例不等于线程安全。**

---

## 6. Starter、组件扫描和自动配置

### 6.1 三者分工

```text
Starter（构建期）
  → Maven 解析/下载相关 JAR

@ComponentScan（运行期配置解析时）
  → 找到业务项目里的组件并注册 BeanDefinition

@EnableAutoConfiguration（运行期配置解析时）
  → 发现候选自动配置类、检查条件并注册相应定义
```

### 6.2 自动配置怎么知道候选类？

Spring Boot 3.x 常通过 JAR 内资源文件登记候选自动配置：

```text
META-INF/spring/org.springframework.boot.autoconfigure.AutoConfiguration.imports
```

`@EnableAutoConfiguration` 与 `AutoConfigurationImportSelector` 等机制配合完成候选配置导入；条件判断后才决定哪些配置生效。

### 6.3 条件注解

| 注解 | 条件含义（简化） |
|---|---|
| `@ConditionalOnClass` | 类路径存在指定类 |
| `@ConditionalOnMissingBean` | 没有匹配的 Bean 时才创建默认方案 |
| `@ConditionalOnProperty` | 配置项满足要求 |
| `@ConditionalOnWebApplication` | 当前应用满足 Web 应用条件 |

例如（**演示片段，不含 `MyClient` 定义，不能单独编译**）：

```java
@AutoConfiguration
@ConditionalOnClass(MyClient.class)
public class MyClientAutoConfiguration {

    @Bean
    @ConditionalOnMissingBean(MyClient.class)
    MyClient myClient() {
        return new MyClient();
    }
}
```

Spring Boot 自动配置的重要思想：**条件成立时提供默认实现；当用户自己声明合适的 Bean 时，某些自动配置会主动退让。**

---

## 7. 一次 HTTP 请求的完整执行路径

以 `GET /api/dishes/1` 为例：

```text
客户端
  ↓  HTTP（底层建立连接，发送请求）
Tomcat 接收连接并处理 HTTP / Servlet 请求
  ↓
Servlet Filter 链（若匹配）
  ↓
DispatcherServlet
  ↓
HandlerMapping：确定 handler 与相关拦截器链
  ↓
HandlerInterceptor.preHandle（若适用且允许继续）
  ↓
HandlerAdapter
  ├─ 参数解析：@PathVariable、@RequestBody 等
  ├─ 类型转换 / 反序列化 / 可选校验
  └─ 调用 DishController.getById(1L)
         ↓
     DishService.getById(1L)
         ↓
     ConcurrentHashMap.get(1L)
         ↓
     返回 Dish Java 对象
  ↓
返回值处理：@ResponseBody / ResponseEntity
  ↓
HttpMessageConverter + Jackson（本例 JSON）
  ↓
写入 HttpServletResponse
  ↓
按适用规则执行 Interceptor 回调、Filter 出站部分等
  ↓
Tomcat/网络栈将 HTTP 响应发送给客户端
```

### 7.1 `DispatcherServlet` 做什么？

它是 Spring MVC **前端控制器**，主要负责组织请求分发：寻找 Handler、选择 HandlerAdapter、管理拦截器与异常处理、完成响应协调。典型分发入口为 `doDispatch()`。

### 7.2 `HandlerMapping` 做什么？

`RequestMappingHandlerMapping` 通常在初始化时建立注解请求映射关系：

```text
GET /api/dishes/{id}  →  DishController.getById(...)
```

每个请求不是重新扫描全部 Controller 源码。它依据已经注册的映射信息匹配路径、HTTP 方法及其他约束。

### 7.3 `HandlerAdapter` 做什么？

以 `RequestMappingHandlerAdapter` 为例，它能够调用注解定义的处理方法，并借助参数解析器（如 `HandlerMethodArgumentResolver`）、数据绑定、类型转换等组件准备实参。

| 注解 | 数据来源示例 |
|---|---|
| `@PathVariable` | `/users/12` → `id=12` |
| `@RequestParam` | `/users?id=12` → `id=12` |
| `@RequestBody` | JSON Body → Java 对象 |

### 7.4 Java 对象如何变成 JSON？

```text
Controller 返回 Dish
    ↓
@ResponseBody 的返回值处理逻辑
    ↓
按返回值、媒体类型与转换器选择结果
    ↓
MappingJackson2HttpMessageConverter 等
    ↓
Jackson 序列化 JSON
    ↓
响应体
```

不应说 `@RestController` **永远只返回 JSON**：也可能返回字符串、字节流等，具体取决于返回值、消息转换器、媒体类型协商和配置。

### 7.5 错误响应与异常处理

本例中不存在的 ID 会抛：

```java
throw new ResponseStatusException(
        HttpStatus.NOT_FOUND, "菜品不存在");
```

通常转成 HTTP **404**。异常响应体的具体字段和形式取决于 Spring Boot 的错误处理配置、是否启用 Problem Details 等；**不能默认断言一定会出现某一套 JSON 字段**。

全局异常处理常用：

```java
@RestControllerAdvice
public class GlobalExceptionHandler {
    // 使用 @ExceptionHandler 处理指定异常
}
```

这段仅展示扩展入口，不是完整异常处理实现。

### 7.6 `postHandle` 的关键细节

在传统返回视图的流程中，`postHandle()` 在 Handler 正常执行之后、视图渲染之前介入。但对 `@ResponseBody` / `ResponseEntity`，响应可能已经在 `HandlerAdapter` 中写出/提交，**不应把 `postHandle()` 当作修改最终 JSON 响应的通用位置**。

---

## 8. 常用 Spring 注解速查

### 8.1 应用和 Bean 管理

| 注解 | 作用 |
|---|---|
| `@SpringBootApplication` | Spring Boot 应用主配置，整合自动配置、组件扫描等 |
| `@Configuration` | 配置类 |
| `@Bean` | 通过工厂方法注册 Bean |
| `@Component` | 通用组件 |
| `@Service` | 业务层组件（语义化的组件注解） |
| `@Repository` | 持久层组件（可参与异常转换等支持） |
| `@Controller` | MVC 控制器 |
| `@RestController` | REST 风格 Controller（包含 `@ResponseBody` 语义） |
| `@Autowired` | 自动依赖装配；单构造器一般可省略 |
| `@Qualifier` | 约束候选 Bean 的选择 |
| `@Import` | 导入配置类、组件/选择器等（取决于导入目标） |

### 8.2 Web 接口

| 注解 | 作用 |
|---|---|
| `@RequestMapping` | 通用 HTTP 请求映射 |
| `@GetMapping` | GET 映射 |
| `@PostMapping` | POST 映射 |
| `@PutMapping` | PUT 映射 |
| `@DeleteMapping` | DELETE 映射 |
| `@PathVariable` | URL 路径模板变量 |
| `@RequestParam` | 查询参数、表单参数等请求参数 |
| `@RequestBody` | HTTP 请求体到 Java 对象 |
| `@ResponseBody` | 返回值到 HTTP 响应体 |
| `@RestControllerAdvice` | 对 REST 控制器的全局增强/异常处理等 |
| `@ExceptionHandler` | 指定异常的处理方法 |

### 8.3 配置、生命周期、事务

| 注解 | 作用/注意点 |
|---|---|
| `@Value` | 注入配置值；缺少属性时要考虑占位符默认值 |
| `@ConfigurationProperties` | 类型安全绑定一组外部配置；需要注册配置属性 Bean |
| `@ConfigurationPropertiesScan` | 扫描配置属性类 |
| `@PostConstruct` | 初始化回调（通过 Bean 后处理机制支持） |
| `@PreDestroy` | 销毁回调（受 Bean 作用域/容器销毁管理限制） |
| `@Transactional` | 声明事务语义；需要事务管理基础设施 |
| `@Valid` | 触发 Bean Validation（需具备相应校验支持） |

### 8.4 `@Value` 的正确示例

```java
@Value("${server.port:8080}")
private int configuredPort;
```

**注意**：这里表示配置期望的端口，不保证等于服务器实际分配的端口（例如 `server.port=0`）。查询运行中 Web 服务器的实际端口应使用相应的 WebServer API/启动事件等机制。

### 8.5 `@ConfigurationProperties` 需要注册

```yaml
food:
  order:
    timeout: 30
```

```java
package com.example.food.config;

import org.springframework.boot.context.properties.ConfigurationProperties;

@ConfigurationProperties(prefix = "food.order")
public record OrderProperties(int timeout) {}
```

主类可以显式启用扫描：

```java
@SpringBootApplication
@ConfigurationPropertiesScan
public class FoodApplication { /* ... */ }
```

这里是说明性局部代码；若直接复制，还需补齐注解 import、主类方法，并确保包结构可被扫描。

---

## 9. Filter、Interceptor、AOP 和事务

### 9.1 主要区别

| 机制 | 所属层次 | 典型用途 | 关键特点 |
|---|---|---|---|
| Servlet Filter | Servlet 容器请求链 | 请求/响应包装、日志、安全过滤 | 在 `DispatcherServlet` 前后生效（具体取决于匹配和顺序） |
| Spring MVC `HandlerInterceptor` | MVC Handler 执行链 | 记录 Handler 信息、MVC 请求前后增强 | 拥有 `preHandle/postHandle/afterCompletion` |
| Spring AOP | Spring Bean 的方法调用 | 事务、指标、日志切面 | 默认基于代理拦截方法调用，不以 HTTP 为必要条件 |

**不是同一个维度：**Filter 和 Interceptor 围绕 Web 请求；AOP 围绕符合条件的 Bean 方法调用。

### 9.2 Interceptor 典型回调

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

这是接口**常用方法签名示意**，省略了相关 import 与已有接口默认实现细节，不是供粘贴到项目里重新定义的新接口。

注意：认证授权优先考虑 Spring Security 等安全框架，不要仅靠 MVC 拦截器构建完整安全边界。

### 9.3 AOP 与 `@Transactional`

真实的数据库服务可能写：

```java
@Service
public class OrderService {

    @Transactional
    public void createOrder() {
        // 插入订单、更新库存：需实际数据库访问代码
    }
}
```

典型调用路径：

```text
Controller
  ↓
OrderService 的 Spring AOP 代理
  ↓
TransactionInterceptor
  ↓
根据事务管理器及传播规则加入/创建事务
  ↓
目标业务方法 + 数据库操作
  ↓
依事务规则提交或标记回滚
```

**这里需要有合适的 `PlatformTransactionManager`、数据源等基础设施**。在纯 `ConcurrentHashMap` 服务上加注解并不会让 Map 操作具备数据库事务回滚能力。

### 9.4 为什么同类内部调用可能失效？

```java
@Service
public class ExampleService {

    public void a() {
        b(); // 默认代理式事务下通常不会通过代理
    }

    @Transactional
    public void b() {
        // ...
    }
}
```

默认代理模式只拦截**通过代理进入**的调用；目标对象内部的 `this.b()` 是普通 Java 调用，可能绕过事务拦截。使用独立 Service、重新组织事务边界等方式往往更清晰。

### 9.5 默认事务行为

- 默认传播行为通常是 `Propagation.REQUIRED`。
- 默认 `RuntimeException`、`Error` 触发回滚。
- 默认 checked exception 通常不触发回滚；可设置 `rollbackFor`。
- 事务仅对**参与该事务并受到事务管理的资源**产生效果。
- 事务不是分布式原子性的万能开关，支付/消息等跨系统动作需要另外设计。

---

## 10. 单例 Controller 与并发安全

```text
请求 A（Tomcat 工作线程 A）──┐
请求 B（Tomcat 工作线程 B）──┼─→ 同一个 DishController Bean
请求 C（Tomcat 工作线程 C）──┘
```

这里表示传统同步 MVC 的典型并发情况，不能引申为“Tomcat 每个 TCP 连接一定创建一个新线程”。

**有风险的代码：**

```java
@RestController
public class CounterController {
    private int count = 0;

    @GetMapping("/count")
    public int count() {
        return ++count;
    }
}
```

`++count` 是非原子复合操作，多线程调用可能丢失更新。

**相对安全的无状态代码：**

```java
@RestController
public class HelloController {
    @GetMapping("/hello")
    public String hello() {
        int count = 0;
        count++;
        return "count=" + count;
    }
}
```

方法局部变量不因为 Controller 是单例就天然变成实例共享字段。

项目实践：Controller、Service 尽量无状态；共享数据在适当的数据结构、缓存或数据库中管理，并按需要设计并发一致性。

---

## 11. 数据库访问与 MyBatis 代理

真实后端常见链路：

```text
HTTP 请求
  ↓
Controller
  ↓
Service（业务规则/事务边界）
  ↓
Mapper（MyBatis/MyBatis-Plus 代理）
  ↓
MyBatis 执行器 / JDBC / DataSource
  ↓
MySQL
```

常见示例（**要求已正确引入 MyBatis-Plus 并配置数据源和 Mapper 注册**）：

```java
@Mapper
public interface DishMapper extends BaseMapper<DishEntity> {
}
```

为什么接口没有手写实现类仍可调用？因为框架在配置合适时会为 Mapper 接口创建代理对象，由代理将方法调用转化为相应的持久化操作。

注意区分：

- **Spring AOP 代理**：拦截 Spring Bean 的方法调用，用于事务、切面增强等。
- **MyBatis Mapper 代理**：把 Mapper 接口调用转换成 MyBatis 执行流程。
- 都可能涉及代理，但不是同一种业务职责。

---

## 12. 配置、调试与源码追踪

### 12.1 查看 IoC 中拿到的是不是同一个实例

在启动类的 `run()` 返回后：

```java
ConfigurableApplicationContext context =
        SpringApplication.run(FoodApplication.class, args);

DishService a = context.getBean(DishService.class);
DishService b = context.getBean(DishService.class);
System.out.println(a == b); // 默认 singleton 情况预期 true
```

上述是**替换原有一行 `run()` 的局部代码**；需要导入 `ConfigurableApplicationContext`、`DishService`。

### 12.2 输出自动配置条件评估报告

```bash
java -jar target/food-demo-0.0.1-SNAPSHOT.jar --debug
```

此命令需要先执行 Maven 打包：

```bash
mvn package
```

`--debug` 的条件评估报告可以帮助观察为什么一些自动配置生效或不生效。

### 12.3 Actuator（可选）

增加 `spring-boot-starter-actuator` 后，可以按需求检查：

- `/actuator/health`
- `/actuator/beans`
- `/actuator/mappings`
- `/actuator/conditions`

但**不是所有端点默认暴露在 HTTP 上**。若要开放必须显式配置，同时评估认证、网络隔离及敏感信息泄露风险。官方文档说明默认 HTTP 暴露的通常仅有健康端点。

### 12.4 建议打断点的位置

1. `SpringApplication.run()`：观察引导过程。
2. `AbstractApplicationContext.refresh()`：观察容器刷新。
3. `DishService` 构造器：观察实例化。
4. `DishController` 构造器：观察构造器注入。
5. `DispatcherServlet.doDispatch()`：观察 MVC 分发。
6. `DishController.getById()`：观察参数传入。
7. `DishService.getById()`：观察业务处理。

**调试顺序建议**：先打自己业务方法的断点，再从调用栈回溯框架；不要一开始就试图读完全部 Spring 源码。

---

## 13. 练习路线、典型面试题和自测

### 13.1 学习优先级

**A. 必须会手写**

- [ ] 创建 Spring Boot 项目并配置 Maven 与端口。
- [ ] 编写 `@RestController`、`@Service`，使用构造器注入。
- [ ] 编写 GET/POST/PUT/DELETE 接口。
- [ ] 处理 `@PathVariable`、`@RequestParam`、`@RequestBody`。
- [ ] 引入 Validation，实现输入校验。
- [ ] 编写统一异常处理。
- [ ] 连接 MySQL，实现基本 CRUD。
- [ ] 用 Apifox、Postman 或 curl 验证请求/响应。

**B. 必须理解原理**

- [ ] `@SpringBootApplication` 三个组成注解。
- [ ] Starter 与自动配置分别发生在哪个阶段。
- [ ] BeanDefinition、Bean 创建、DI、Bean 生命周期。
- [ ] 请求从 Tomcat 到 Controller 再到 JSON 的完整链路。
- [ ] `@Transactional` 的代理机制和失效场景。
- [ ] singleton Bean 与线程安全。

**C. 进阶再看源码**

- [ ] `AutoConfigurationImportSelector` 和 `.imports` 文件。
- [ ] `AbstractApplicationContext.refresh()`。
- [ ] `BeanFactoryPostProcessor`、`BeanPostProcessor`。
- [ ] AOP 代理的创建与方法拦截。
- [ ] `DispatcherServlet.doDispatch()`、HandlerMapping/HandlerAdapter。
- [ ] Tomcat 连接处理与 Servlet 容器原理。

### 13.2 自测题与要点答案

**1. Spring Boot 和 Spring Framework 是什么关系？**  
Boot 是简化构建、自动配置、启动与整合的方案；底层依托 Spring Framework 的 IoC、AOP 等机制。

**2. 引入 Starter 后是否就完成自动配置？**  
否。Starter 负责依赖；启动时自动配置机制依据运行环境和条件注册需要的 Bean。

**3. `@Service` 为什么不需要自己 new？**  
组件扫描等方式注册 BeanDefinition，IoC 容器完成实例化、依赖装配与初始化。

**4. `@ComponentScan` 会扫描 Maven 依赖中的所有类吗？**  
不会。默认围绕启动类所在包及其子包扫描；自动配置有独立的候选类导入机制。

**5. 为什么单构造器通常不需要 `@Autowired`？**  
Spring 能识别唯一构造器并完成构造器注入。

**6. 为什么 Tomcat 不需要知道 `@GetMapping`？**  
Tomcat 按 Servlet 机制交付请求，Spring MVC 的 DispatcherServlet 与映射、适配器机制负责找到并调用 Controller 方法。

**7. 返回 `Dish` 为什么客户端看见 JSON？**  
因为 MVC 的响应体处理及消息转换器选择了 JSON 序列化器（通常 Jackson）。

**8. 默认 singleton Controller 能不能保存用户级可变字段？**  
不建议。多个请求可能共享同一个实例，容易出现串数据和竞态条件。

**9. `@Transactional` 是否只要标注就能回滚 Map 变更？**  
不能。需要实际事务管理基础设施且资源必须支持/参与事务。Map 更新不因此自动具备回滚。

**10. 为什么同类自调用可能让 `@Transactional` 不生效？**  
默认代理模式下绕过代理的内部方法调用不会触发事务拦截器。

**11. `@PostConstruct` 是不是固定在所有初始化前 BPP 后执行？**  
不是。它自身通常就是由 `CommonAnnotationBeanPostProcessor` 的初始化前处理机制触发。

**12. HTTP JSON 响应能不能在 `postHandle()` 中任意修改？**  
不能。响应在 HandlerAdapter 阶段可能已经写出或提交；用更合适的响应增强机制。

### 13.3 记忆总图

```text
【应用启动】
main()
   ↓
SpringApplication.run()
   ↓
Environment + ApplicationContext
   ↓
@ComponentScan + @EnableAutoConfiguration
   ↓
BeanDefinition → Bean 创建/注入/初始化
   ↓
ServletWebServerFactory → Tomcat + DispatcherServlet
   ↓
应用开始接收请求

【请求执行】
客户端
   ↓
Tomcat / Filter
   ↓
DispatcherServlet
   ↓
HandlerMapping（找方法）
   ↓
Interceptor（符合条件才执行）
   ↓
HandlerAdapter（准备参数、调用方法）
   ↓
Controller → Service → Mapper/数据库（如果有）
   ↓
返回值处理 / HttpMessageConverter / Jackson
   ↓
HTTP Response

【横切关注点】
Spring IoC/DI：管理对象依赖
Spring AOP：在适用的 Bean 方法调用处增强
@Transactional：基于事务基础设施及调用链实现事务语义
```

---

## 14. 官方参考资料

以下资料用于本次**聊天正确性核对**；以官方文档/官方发布消息为主。部分页面会显示当前最新版本，学习 3.5.x 时要看清版本路径。

1. Spring Boot 3.5.16 发布公告（含最后一版 3.5.x OSS 的说明）：<https://spring.io/blog/2026/06/25/spring-boot-3-5-16-available-now/>
2. Spring Boot 3.5.7 发布公告（确认聊天里另一版本真实存在）：<https://spring.io/blog/2025/10/23/spring-boot-3-5-7-available-now/>
3. 官方 `@SpringBootApplication` 文档：<https://docs.spring.io/spring-boot/reference/using/using-the-springbootapplication-annotation.html>
4. Spring Boot 4.x Starter 列表（含 `spring-boot-starter-webmvc`）：<https://docs.spring.io/spring-boot/reference/using/build-systems.html>
5. Spring Boot 3.5 Servlet Web 应用：<https://docs.spring.io/spring-boot/3.5/reference/web/servlet.html>
6. Spring Boot 3.5 内嵌服务器：<https://docs.spring.io/spring-boot/3.5/how-to/webserver.html>
7. Spring Boot 3.5 自动配置注解 API：<https://docs.spring.io/spring-boot/3.5/api/java/org/springframework/boot/autoconfigure/EnableAutoConfiguration.html>
8. 官方开发自动配置示例（候选类 `.imports` 文件）：<https://docs.spring.io/spring-boot/3.4/reference/features/developing-auto-configuration.html>
9. Spring MVC 关键 Bean 类型（HandlerMapping/HandlerAdapter）：<https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-servlet/special-bean-types.html>
10. Spring MVC 拦截器及 `postHandle()` 注意事项：<https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-servlet/handlermapping-interceptor.html>
11. Spring 事务注解、代理自调用与回滚默认规则：<https://docs.spring.io/spring-framework/reference/data-access/transaction/declarative/annotations.html>
12. Spring BeanPostProcessor：<https://docs.spring.io/spring-framework/reference/core/beans/factory-extension.html>
13. Spring Boot 外部化配置与 `@Value`：<https://docs.spring.io/spring-boot/reference/features/external-config.html>
14. Spring Boot 3.5 Actuator 端点暴露与安全：<https://docs.spring.io/spring-boot/3.5/reference/actuator/endpoints.html>

---

**总结：** Spring Boot 的关键不在于背诵注解名称，而在于把 **“引入依赖 → 解析配置 → 注册 Bean 定义 → 创建与注入 Bean → 启动 Servlet 容器 → 请求映射与参数解析 → 业务处理 → 序列化 HTTP 响应”** 这条主线串起来。能独立解释它，并在断点中验证，就是掌握了 Spring Boot 最重要的入门执行机制。
