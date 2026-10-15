# Spring、Spring MVC、Spring Boot：核对修订版学习笔记

> 核对日期：2026-10-14  
> 范围：针对本次聊天中对 Spring Framework、Spring MVC、Spring Boot 的讲解进行事实核对、示例修订和系统整理。  
> 适用对象：有 Java 基础、正在学习 Java 后端和 Spring Boot 的开发者。  
> 阅读原则：先弄懂三者的职责和请求链路，再学习 IoC / DI、AOP、Bean 生命周期与自动配置；源码细节放到项目实践之后。

---

## 0. 原聊天核对结论（先读）

**总体判断：核心方向正确，但部分描述过于绝对、代码不可直接使用，且把“框架职责”与“常见项目分层”混在了一起。**

| 原聊天说法 / 示例 | 核对结果 | 正确理解 |
|---|---|---|
| Spring 核心是 IoC、AOP | ✅ 基本正确 | 这两者是重要基础能力；Spring Framework 还提供事务、数据访问、Web、测试等模块，不能等同于仅有 IoC/AOP。 |
| Spring MVC 是 Spring 的 Web 模块 | ✅ 正确 | 更准确：基于 Servlet API 的 Spring Web MVC 框架，用于 Web 请求映射、参数绑定、响应处理、视图解析等。 |
| Spring Boot 简化 Spring 开发 | ✅ 正确 | 它在 Spring 基础上提供依赖管理、自动配置、Starter、可执行应用及运维支持；并不取代 Spring MVC 或 Spring IoC。 |
| `@Service` / `@Component` 就一定会被创建为 Bean | ⚠️ 不完整 | 类需要被组件扫描覆盖，或通过其他注册方式进入容器；默认 singleton Bean 一般在容器启动时预实例化，也有懒加载等例外。 |
| `@Autowired` 字段注入是主要写法 | ⚠️ 可工作，不推荐作首选 | 必需依赖优先使用**构造器注入**；单构造器通常可以省略 `@Autowired`。 |
| IoC 就是不写 `new` | ⚠️ 过度简化 | IoC 是对象创建、装配等控制权由应用转交容器；业务代码仍可正常 `new` 普通对象。依赖注入（DI）是 IoC 的一种实现方式。 |
| AOP 环绕通知 `return method.invoke()` | ❌ 原示例错误 | `@Around` 通知需要接收 `ProceedingJoinPoint`，通常调用 `joinPoint.proceed()`，并正确返回结果、传播异常。 |
| Spring MVC 请求只经过 `DispatcherServlet → Controller` | ⚠️ 省略关键步骤 | 还包括 `HandlerMapping`、`HandlerAdapter`、参数解析器、拦截器（视情况）、返回值处理器和 `HttpMessageConverter` 等。 |
| Spring MVC 负责 Controller → Service → DAO → 数据库 | ⚠️ 职责混淆 | **Controller → Service → Repository/Mapper** 是常见项目分层；Spring MVC 负责 Web 处理，不负责定义或强制要求业务、持久化层。 |
| Spring Boot 自动创建 `DataSource`、`JdbcTemplate`、事务管理器 | ⚠️ 条件缺失 | 自动配置依赖类路径、属性和用户已定义 Bean 等条件；**只写一个 URL 并不保证这些对象都出现**。 |
| Spring Boot 自带 Tomcat | ⚠️ 仅适用于常见 Servlet Web 场景 | Web MVC Starter 的常见默认服务器是内嵌 Tomcat，但可替换为兼容服务器；非 Web 应用不必运行 Web 服务器。 |
| 传统 Spring MVC 必须使用 XML | ❌ 过时 / 不准确 | 非 Boot 的 Spring MVC 也可以使用 Java 配置和注解；XML 是历史常见方式，不是必需条件。 |
| Spring Boot 的 Web Starter 永远叫 `spring-boot-starter-web` | ⚠️ 有版本差异 | Boot 3 常用 `spring-boot-starter-web`；**Boot 4 推荐 `spring-boot-starter-webmvc`**，旧名称在 Boot 4 已被弃用。 |
| 应尽快深入“三级缓存解决循环依赖” | ⚠️ 学习顺序不合理 | 先掌握 Bean 创建、注入、生命周期与 AOP 代理；循环依赖应优先通过设计避免，且 Boot 默认不允许循环引用。 |

**特别强调：** 原聊天里的 `UserDao` 类型声明前后不统一、`getUser()` 业务体为空、AOP 代码缺少变量定义等，都是**讲解性伪代码**，不能当成完整可运行示例。

---

## 1. 先建立三者关系的正确模型

### 1.1 一句话辨认

- **Spring Framework：** Java 应用基础框架，提供 IoC/DI 容器、AOP、事务管理以及丰富的应用组件。
- **Spring MVC：** Spring Framework 体系里的 Servlet Web 框架，主要处理 HTTP 请求与响应。
- **Spring Boot：** 构建、配置、启动与运维 Spring 应用的简化方案；可自动集成 Spring MVC 等技术。

```text
                Spring Boot（配置 / 启动 / 依赖管理）
                              │
                    基于 Spring Framework
                              │
            ┌─────────────────┼─────────────────┐
            │                 │                 │
        IoC / DI / AOP     Spring MVC         事务 / JDBC ...
          基础能力          Web 框架            其他模块
```

**不要把它们看成三个按顺序处理请求的独立服务器。** Spring Boot 主要负责项目装配和启动；请求到来后是 Web 服务器与 Spring MVC 配合处理，Controller/Service 等 Bean 则由 Spring 容器管理。

### 1.2 用一个外卖项目举例

- 下单时 `OrderController` 接收 `/orders` HTTP 请求：主要属于 **Spring MVC** 的职责。
- `OrderService` 需要 `OrderMapper`，由框架自动注入：主要是 **Spring IoC / DI** 的职责。
- 下单要保证订单、库存数据库更新的一致性，使用 `@Transactional`：主要是 **Spring 事务管理**，常见实现依赖 AOP 代理机制。
- 启动应用、自动装配 MVC、读取 `application.yml`、运行内嵌服务器：主要由 **Spring Boot** 协调完成。
- `OrderMapper` 具体如何执行 SQL，取决于 **MyBatis / JDBC / JPA** 等数据访问技术，不是 Spring MVC 的功能。

---

## 2. Spring Framework：IoC、DI 和 Bean

### 2.1 为什么要把对象交给容器管理？

传统 Java 写法：

```java
public class OrderService {
    private final OrderRepository repository = new OrderRepository();

    public void createOrder() {
        repository.save();
    }
}
```

`OrderService` 直接决定自己的依赖如何构造。替换实现、注入模拟对象进行测试时不够灵活。

不用 Spring，也能先通过构造器实现依赖倒置：

```java
public class OrderService {
    private final OrderRepository repository;

    public OrderService(OrderRepository repository) {
        this.repository = repository;
    }
}
```

Spring 进一步提供一个容器，负责创建和装配这些对象：

```java
public interface OrderRepository {
    void save();
}
```

```java
import org.springframework.stereotype.Repository;

@Repository
public class JdbcOrderRepository implements OrderRepository {
    @Override
    public void save() {
        System.out.println("保存订单（示意）");
    }
}
```

```java
import org.springframework.stereotype.Service;

@Service
public class OrderService {
    private final OrderRepository repository;

    // 单构造器注入，一般不需要额外写 @Autowired
    public OrderService(OrderRepository repository) {
        this.repository = repository;
    }

    public void createOrder() {
        repository.save();
    }
}
```

**要成立的前提：** 上述实现类位于组件扫描范围内，并且存在唯一可解析的 `OrderRepository` Bean。多个候选实现时可通过 `@Qualifier` 或 `@Primary` 等区分。

### 2.2 IoC 与 DI 的区别

- **IoC（Inversion of Control，控制反转）：** 原本代码自己管理的对象创建、生命周期、依赖组装等工作，转由容器协调。
- **DI（Dependency Injection，依赖注入）：** 让组件**声明依赖**，容器在创建 / 装配组件时提供依赖。它是实现 IoC 的常见方式。

```text
不使用容器：业务对象主动 new / 查找其依赖
使用容器： 业务对象声明其依赖 → Spring 容器提供所需 Bean
```

> 容器不会禁止你使用 `new`。DTO、VO、临时计算对象等通常仍然由业务代码创建；不需要“所有对象都变成 Bean”。

### 2.3 Spring 容器到底是什么？

- `BeanFactory`：Spring IoC 容器的基础接口。
- `ApplicationContext`：继承 `BeanFactory`，提供更多应用层功能（如事件、消息资源等），实际项目最常使用。
- **Bean**：由 Spring IoC 容器实例化、装配或注册并管理的对象。
- **BeanDefinition**：描述 Bean 的元信息，如类型、作用域、构造参数等；不要把它与 Bean 实例混为一谈。

```text
读取配置 / 扫描类 / 解析 @Bean
              ↓
        注册 BeanDefinition
              ↓
       根据需要创建 Bean
              ↓
         注入依赖、初始化
              ↓
   应用通过容器管理的引用访问 Bean
```

默认常见 `singleton` 作用域表示**每个 Spring 容器中、每个 Bean 定义通常对应一个共享实例**，不是“整个 JVM 永远只能有一个该类的对象”。此外还有 `prototype`、请求作用域等。

### 2.4 常用 Bean 注册方式

**组件扫描（适合业务类）：**

```java
@Component   // 通用组件
@Service     // 业务层语义
@Repository  // 持久层语义
@Controller  // MVC 控制器语义
@RestController // REST 控制器：@Controller + @ResponseBody
```

这些不是写在同一个类上必须同时使用的注解，而是不同类根据用途选择。

**配置类 + `@Bean`（适合需要显式创建的第三方对象）：**

```java
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import java.time.Clock;

@Configuration
public class TimeConfig {
    @Bean
    public Clock clock() {
        return Clock.systemUTC();
    }
}
```

`@Bean` 的方法返回值会作为 Spring Bean 注册（在有效配置上下文中），不是普通业务方法一旦加了注解就可以在任何地方产生容器管理效果。

### 2.5 为什么建议构造器注入？

```java
@Service
public class OrderService {
    private final OrderRepository repository;

    public OrderService(OrderRepository repository) {
        this.repository = repository;
    }
}
```

优点：依赖明确、方便单元测试、支持 `final`、对象更容易保持完整初始化状态。字段注入 `@Autowired private OrderRepository repository;` 虽然能工作，但不适合作为新项目的默认习惯。可选依赖才考虑合适的 Setter 等注入方案。

### 2.6 Bean 生命周期（初学阶段掌握概览即可）

```text
读取 Bean 定义
     ↓
实例化对象（调用构造器 / 工厂方法）
     ↓
属性填充 / 依赖注入
     ↓
Aware 回调、BeanPostProcessor 初始化前处理（简化）
     ↓
@PostConstruct / InitializingBean / init-method 等初始化逻辑
     ↓
BeanPostProcessor 初始化后处理（可能包装为代理）
     ↓
Bean 可被正常使用
     ↓
容器关闭时：对适用作用域执行销毁回调
```

这是一张**典型流程的简化图**。具体生命周期还和 Bean 来源、作用域、代理处理器有关。默认不会替每个 `prototype` Bean 自动完成完整的销毁管理。

---

## 3. Spring AOP：将横切逻辑与业务逻辑分离

### 3.1 AOP 解决什么问题？

多个业务方法都需要日志、耗时统计或某些横切逻辑时，不想在每个业务方法里复制一套代码。AOP 允许为匹配到的方法统一配置通知。

**常见术语：**

| 术语 | 理解 |
|---|---|
| Aspect（切面） | 组织横切逻辑的类 |
| Advice（通知） | 实际执行的增强逻辑，如前置、后置、环绕 |
| Pointcut（切点） | 哪些方法需要增强的匹配规则 |
| Join Point（连接点） | 在 Spring AOP 中通常是方法执行 |
| Proxy（代理） | 在不直接修改原业务代码的情况下，拦截受支持的方法调用 |

### 3.2 原聊天中 AOP 代码的正确改法

原聊天示例写了 `@Around`，却没有定义要调用的 `method`，也没有传入 `ProceedingJoinPoint`。正确示例：

```java
package com.example.demo.aspect;

import org.aspectj.lang.ProceedingJoinPoint;
import org.aspectj.lang.annotation.Around;
import org.aspectj.lang.annotation.Aspect;
import org.springframework.stereotype.Component;

@Aspect
@Component
public class TimeCostAspect {

    @Around("execution(* com.example.demo.service..*.*(..))")
    public Object measure(ProceedingJoinPoint joinPoint) throws Throwable {
        long start = System.nanoTime();
        try {
            return joinPoint.proceed(); // 执行真正的目标方法
        } finally {
            long elapsed = System.nanoTime() - start;
            System.out.println(joinPoint.getSignature() + " cost(ns)=" + elapsed);
        }
    }
}
```

**前提：** 项目已添加相应 AOP/AspectJ 支持的依赖，并启用所需的自动代理机制（Spring Boot 相应 Starter 的典型默认配置可自动处理）。

- Spring Boot 3：常用 `spring-boot-starter-aop`。
- Spring Boot 4：官方 Starter 列表提供 `spring-boot-starter-aspectj`。

**理解 `proceed()`：** 它继续执行目标调用链；如果环绕通知没有调用它，目标方法可能根本不会执行。这与“简单地在方法前后打印两行”不完全相同。

### 3.3 代理的一个重要陷阱

Spring AOP 通常基于代理：

```java
@Service
public class PaymentService {
    public void outer() {
        inner();  // 同一对象内的自调用，可能绕过代理增强
    }

    @Transactional
    public void inner() {
        // 数据库操作
    }
}
```

**默认代理模式下**，通过 `this.inner()`（包括省略 `this`）执行的方法调用不会重新经过代理，因此方法上的事务/AOP 增强可能不生效。应优先重新设计调用边界，例如由另一个 Spring 管理的服务通过代理调用它。不要把“写了 `@Transactional` 就必定生效”当成结论。

---

## 4. Spring MVC：处理 HTTP 请求和响应

### 4.1 Spring MVC 与 Servlet / Tomcat 的关系

先区分三个层次：

1. **Tomcat**：常用 Servlet 容器/Web 服务器，处理连接与 Servlet 请求分发等工作。
2. **Servlet API**：Java Web 的标准接口体系，Spring MVC 建立在它之上。
3. **Spring MVC**：基于 Servlet API 的 Web 框架，提供请求映射、控制器调用、参数绑定、消息转换等功能。

`DispatcherServlet` 是一个 Servlet，同时也是 Spring MVC 的前端控制器（Front Controller），负责协调 MVC 请求处理。

> “所有请求都会进入 DispatcherServlet”只是简化说法。更准确地说：**匹配该 Servlet 映射、并经前置过滤链处理后继续传递的请求**才会到达它。其他 Servlet、静态资源处理或 Filter 拦截也会影响请求路线。

### 4.2 关键组件

| 组件 | 职责 |
|---|---|
| `DispatcherServlet` | 请求处理的总调度入口 |
| `HandlerMapping` | 根据请求寻找匹配的处理器及拦截器链 |
| `HandlerAdapter` | 适配并调用处理器（如 `@RequestMapping` 方法） |
| `HandlerMethodArgumentResolver` | 解析控制器方法参数，如 `@PathVariable`、`@RequestParam` 等 |
| `HandlerInterceptor` | 在 MVC 处理链中执行前后置逻辑（与 Filter 不是同一层） |
| `HttpMessageConverter` | 请求体反序列化、响应体序列化（如 JSON） |
| `HandlerExceptionResolver` | 参与异常处理 / 异常到响应的解析 |
| `ViewResolver` | 在视图型 MVC 返回中解析视图；REST JSON 响应通常不走模板视图解析 |

### 4.3 一个 REST 查询接口的主要请求流程

```text
HTTP 客户端
   │  GET /users/1
   ▼
Tomcat（接收请求并交给 Servlet 链）
   │
   ▼
Servlet Filter 链（如适用）
   │
   ▼
DispatcherServlet
   │
   ├── HandlerMapping：定位 UserController#getUser
   │
   ├── HandlerInterceptor.preHandle（如有）
   │
   └── HandlerAdapter
          │
          ├── 解析 @PathVariable、@RequestBody 等参数
          │
          ▼
      UserController
          │
          ▼
      UserService（业务逻辑）
          │
          ▼
      Repository / Mapper（需要数据时）
          │
          ▼
      数据库（如存在）
          │
          └── 结果逐层返回 Controller
   │
   ▼
返回值处理（如 ResponseEntity / @ResponseBody）
   │
   ▼
HttpMessageConverter（如序列化为 JSON）
   │
   ▼
HTTP 状态码、响应头、响应体 → 客户端
```

上述流程省略了部分内部细节。MVC 拦截器的 `postHandle`、`afterCompletion` 何时运行，还受返回处理、异步及异常流程影响；**不要简单理解为所有拦截器都固定地在 JSON 序列化之后运行**。

### 4.4 常见注解

```java
@RestController // 声明 REST 风格的 Controller
@RequestMapping("/users") // 类级别请求路径
public class UserController {

    @GetMapping("/{id}")
    public String find(@PathVariable Long id) {
        return "user=" + id;
    }

    @GetMapping
    public String search(@RequestParam String name) {
        return "name=" + name;
    }

    @PostMapping
    public String create(@RequestBody CreateUserRequest request) {
        return "created=" + request.name();
    }
}

record CreateUserRequest(String name) {}
```

- `@PathVariable`：URL 路径变量，例如 `/users/10` 中的 `10`。
- `@RequestParam`：查询参数等，例如 `/users?name=Tom`。
- `@RequestBody`：从 HTTP 请求体读取并转换数据。
- `@ResponseBody`：把方法返回值写入响应体（通过相应转换器）；`@RestController` 已包含这一语义。
- `@Controller`：更常见于需要视图返回的 MVC 场景；并不等于 REST Controller。

**注意：** Java 的 `return User` 与 HTTP 中的 JSON 文本不是同一回事。一般先返回一个 Java 对象，再由返回值处理流程中的消息转换器生成响应内容；是否转为 JSON 取决于依赖、内容协商、配置和返回值类型。

---

## 5. Spring Boot：依赖管理、自动配置、启动

### 5.1 Spring Boot 不是另一个独立的 Web MVC 框架

- 使用 Boot 的 Web MVC 项目，本质上仍会用到 Spring 的 IoC 容器与 Spring MVC。
- Boot 帮你选择合理默认值、装配常见组件、管理依赖版本、简化启动和外部配置。
- 不使用 Boot，也可以使用纯 Spring Framework + Spring MVC 开发 Web 应用。
- 使用 Boot，也可以构建非 Web 应用。

### 5.2 Starter 解决什么问题？

Starter 通过 Maven/Gradle 的**传递依赖**打包常见功能所需的依赖组合。例如 Servlet REST API 常需要 Spring MVC、JSON 转换相关组件和服务器。

**版本特别提示（截至 2026-10-14）：**

| 使用的 Spring Boot 主版本 | Web MVC 常见依赖 |
|---|---|
| Spring Boot 3.x | `org.springframework.boot:spring-boot-starter-web` |
| Spring Boot 4.x | `org.springframework.boot:spring-boot-starter-webmvc`（推荐） |

Boot 4 的 `spring-boot-starter-web` 在官方文档中已被标记为弃用，**不要把 Boot 3 视频中的依赖名不加区分地当成永远适用**。

### 5.3 自动配置如何工作？

**并不是“Boot 看到一个配置文件就给你创建全部对象”。** 典型逻辑是：

```text
@SpringBootApplication
        │
        └── 启用自动配置功能
                    │
                    ▼
           检查当前类路径依赖
                    │
             检查配置属性
                    │
         检查用户是否已有相关 Bean
                    │
                    ▼
         满足条件 → 注册相应 Bean
         条件不满足 → 不进行该项配置
```

两个常见的条件注解：

```java
@ConditionalOnClass(SomeLibrary.class) // 某类存在时生效
@ConditionalOnMissingBean(SomeType.class) // 容器缺少指定 Bean 时生效
```

举例：要自动装配 JDBC 相关设施，通常需要所需 JDBC 依赖、驱动与正确数据源属性等条件；**只配置 `spring.datasource.url` 不意味着一定自动生成 `JdbcTemplate`。**

### 5.4 `@SpringBootApplication` 究竟是什么？

它组合了三类关键功能：

```text
@SpringBootApplication
   ├── @SpringBootConfiguration  // 声明配置类能力
   ├── @EnableAutoConfiguration  // 启用自动配置
   └── @ComponentScan            // 启用组件扫描
```

主类示例：

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

**作用不能仅概括为“三行代码：创建容器、扫描 Bean、启动 Tomcat”。** `SpringApplication.run()` 还会涉及环境准备、应用上下文创建与刷新、自动配置、事件/Runner 回调等启动过程；内嵌服务器仅在合适的 Web 应用类型、依赖与配置下启动。

### 5.5 包扫描范围为什么容易出错？

```text
com.example.demo
├── DemoApplication.java     <- @SpringBootApplication
├── controller/UserController.java
├── service/UserService.java
└── repository/UserRepository.java
```

默认情况下，启动类所在包及其子包中的可扫描组件通常会被找到。如果把业务类放到不相关的兄弟包，可能导致 Bean 未注册和注入失败。

### 5.6 内嵌服务器和部署

经典非 Boot 项目曾常把 `war` 部署到外部 Tomcat。Boot Servlet Web 项目常把应用及所需服务器一起打包并通过以下命令运行：

```bash
java -jar app.jar
```

但需要区分：

- **Boot 不等于 Tomcat**；它可以使用其他受支持的服务器配置。
- **Boot 不等于只能用 JAR**；特定场景仍可采用其他部署方式。
- **不使用 Boot 不等于必须写 XML**；Java 配置在传统 Spring 中同样可用。

---

## 6. 一个可直接跟着写的最小 REST 项目

目标：`GET /users/1` 返回 JSON；`GET /users/999` 返回 404。不依赖数据库，便于专注分清 Spring、Spring MVC 和 Spring Boot 的职责。

### 6.1 项目准备

在 [Spring Initializr](https://start.spring.io/) 创建 Maven 项目，Java 17 或更新的兼容版本。使用所选 Spring Boot 版本对应的 Web MVC Starter：

**Boot 3.x：**

```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-web</artifactId>
</dependency>
```

**Boot 4.x：**

```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-webmvc</artifactId>
</dependency>
```

> 上面展示的是 `pom.xml` 的依赖片段；完整 `pom.xml` 的父 POM / 插件 / 版本管理以 Initializr 生成的项目为准，不要只把片段当作整个 `pom.xml`。

### 6.2 源码结构

```text
src/main/java/com/example/demo/
├── DemoApplication.java
├── model/User.java
├── repository/UserRepository.java
├── service/UserService.java
└── controller/UserController.java
```

**1）`DemoApplication.java`：启动项目（Spring Boot）**

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

**2）`model/User.java`：普通 Java 数据对象，不是 Bean**

```java
package com.example.demo.model;

public record User(Long id, String name) {}
```

**3）`repository/UserRepository.java`：模拟数据访问层**

```java
package com.example.demo.repository;

import com.example.demo.model.User;
import org.springframework.stereotype.Repository;
import java.util.Map;
import java.util.Optional;

@Repository
public class UserRepository {
    private final Map<Long, User> users = Map.of(
        1L, new User(1L, "Alice"),
        2L, new User(2L, "Bob")
    );

    public Optional<User> findById(Long id) {
        return Optional.ofNullable(users.get(id));
    }
}
```

**4）`service/UserService.java`：业务层，使用构造器注入（Spring IoC/DI）**

```java
package com.example.demo.service;

import com.example.demo.model.User;
import com.example.demo.repository.UserRepository;
import org.springframework.stereotype.Service;
import java.util.Optional;

@Service
public class UserService {
    private final UserRepository repository;

    public UserService(UserRepository repository) {
        this.repository = repository;
    }

    public Optional<User> findById(Long id) {
        return repository.findById(id);
    }
}
```

**5）`controller/UserController.java`：HTTP 接口（Spring MVC）**

```java
package com.example.demo.controller;

import com.example.demo.model.User;
import com.example.demo.service.UserService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/users")
public class UserController {
    private final UserService service;

    public UserController(UserService service) {
        this.service = service;
    }

    @GetMapping("/{id}")
    public ResponseEntity<User> findById(@PathVariable Long id) {
        return ResponseEntity.of(service.findById(id));
    }
}
```

### 6.3 运行与结果

在生成的 Maven 项目目录执行：

```bash
./mvnw spring-boot:run   # macOS/Linux；Windows 可运行 mvnw.cmd spring-boot:run
```

浏览器或命令行测试：

```bash
curl -i http://localhost:8080/users/1
curl -i http://localhost:8080/users/999
```

- 第一个请求：通常返回 `HTTP 200` 和 `{"id":1,"name":"Alice"}`。
- 第二个请求：返回 `HTTP 404`。
- 这里只用了 `Map` 模拟数据，没有真正访问数据库。

### 6.4 思考：这五个类各是谁负责？

| 类 / 行为 | 主要角色 |
|---|---|
| `DemoApplication` 以及项目启动 | Spring Boot |
| `UserController` 中的请求映射、参数、HTTP 响应 | Spring MVC |
| `UserService`、`UserRepository` 的注册和自动注入 | Spring IoC/DI |
| `User` record | 普通 Java 语言特性，与 Spring 无必然关系 |
| JSON 序列化 | Spring MVC 消息转换机制 + 对应 JSON 库 |
| 此示例内的数据查询 | 普通 Map 模拟；没有 MyBatis/JPA/SQL |

---

## 7. 典型混淆点集中澄清

### Q1：Spring Boot 与 Spring MVC 能同时使用吗？

可以，而且这是大量 Spring Boot REST 服务的常见组合。Boot 负责自动装配 MVC，MVC 负责处理请求。

### Q2：只有 Spring MVC，没有 Spring Boot 能写 HTTP 接口吗？

可以。只是需要更多显式配置和服务器部署/启动安排。

### Q3：`@RestController` 是 Spring Boot 提供的吗？

不是。它是 Spring Web MVC 体系里的注解；Boot 项目广泛使用，但它并非 Boot 独有。

### Q4：`@Autowired` 是 Spring Boot 的核心能力吗？

不是。依赖注入源于 Spring Framework 的 IoC 容器。

### Q5：使用 `@Service` 就会自动产生事务吗？

不会。`@Service` 主要标识组件角色；是否建立事务边界取决于事务配置和使用（例如 `@Transactional`）以及执行路径。

### Q6：`@Transactional` 是 AOP 吗？

在常见的声明式代理事务模式下，其方法调用通常由 Spring 事务拦截器通过代理实现。它属于 Spring 事务管理能力，不等于必须亲自写一个 `@Aspect`。

### Q7：Bean 必须是全局单例吗？

不是。默认 singleton 作用域是 Spring 容器内的共享实例；也支持 prototype 和 Web 相关作用域等。

### Q8：为什么 Spring Boot 应用已经有 `application.yml`，还需要 `@Configuration`？

YAML / Properties 主要是外部化**配置值**；`@Configuration` + `@Bean` 等可定义和组装**对象**。自动配置能覆盖的场景不一定需要自己写配置类。

### Q9：MyBatis 是 Spring MVC 的一部分吗？

不是。MyBatis 是独立持久化框架，通常通过相应集成方式注册 Mapper；MVC 不负责执行 SQL。

### Q10：什么是 Spring 循环依赖？“三级缓存”重要吗？

例如 A 构造时必须拿到 B，B 构造时又必须拿到 A，会发生依赖循环。某些注入/作用域/代理场景在底层具备处理机制，但**构造器循环依赖不能简单靠三级缓存解决**。Spring Boot 的 `spring.main.allow-circular-references` 默认是 `false`。实际项目应先消除错误设计，而非把三级缓存当作业务开发必备技巧。

---

## 8. 面试回答模板（准确、简洁）

**问：Spring 是什么？**  
Spring Framework 是一个 Java 应用框架，核心基础包括 IoC/DI 容器和 AOP，并提供事务、数据访问和 Web 等多类模块，用于降低组件耦合并支持企业应用开发。

**问：Spring MVC 是什么？**  
Spring MVC 是基于 Servlet API 的 Spring Web 框架。它以 `DispatcherServlet` 为核心，借助 `HandlerMapping`、`HandlerAdapter`、参数/返回值处理器等机制，将 HTTP 请求分派给 Controller 并生成响应。

**问：Spring Boot 是什么？**  
Spring Boot 基于 Spring，借助 Starter、依赖版本管理、条件化自动配置、可执行应用等能力减少项目配置和启动成本；它不取代 IoC 容器或 Spring MVC。

**问：IoC 和 DI 区别？**  
IoC 是控制权交给容器的设计思想，DI 是容器将组件需要的依赖传入其中的一种实现方式；构造器注入是常用的依赖注入形式。

**问：Spring MVC 处理请求的流程？**  
请求经 Web 服务器及相关 Filter 到达 `DispatcherServlet`；通过 `HandlerMapping` 定位处理器，交给 `HandlerAdapter` 完成参数解析并执行 Controller；业务层返回结果后由返回值处理器与消息转换器/视图机制产生 HTTP 响应，异常则可能通过异常解析器处理。

**问：Spring Boot 自动配置原理？**  
启动时加载可用的自动配置，并依据类路径、配置属性、Bean 存在与否等条件决定注册哪些配置和组件，实现合理默认行为并允许用户自定义覆盖。

---

## 9. 学习顺序（面向 Java 后端项目）

**第一阶段：能独立写 CRUD**

1. 理解 HTTP / Servlet / Tomcat 的分工。
2. 用 Spring Boot 创建项目，写 `@RestController`、GET/POST/PUT/DELETE。
3. 掌握 `@RequestParam`、`@PathVariable`、`@RequestBody`、`ResponseEntity` 和 JSON 处理。
4. 搞懂 IoC/DI、构造器注入、`@Component`/`@Service`/`@Repository`/`@Bean`。
5. 集成 MyBatis 或其他数据访问技术，实现真正的数据库 CRUD。

**第二阶段：写对真实项目**

6. 参数校验、统一异常处理、响应设计、日志。
7. Spring 事务：`@Transactional`、回滚规则、传播行为、自调用失效问题。
8. AOP 代理、Bean 作用域与生命周期。
9. Spring MVC 请求链路、Filter 与 Interceptor 区别。
10. Spring Boot 自动配置、配置优先级、条件注解。

**第三阶段：面试与底层**

11. BeanFactory 与 ApplicationContext、Bean 初始化与后置处理器。
12. JDK 动态代理 / CGLIB、Spring AOP 与事务原理。
13. 自动配置加载与条件评估机制。
14. 循环依赖、单例缓存等源码问题：以理解原理与设计边界为目的，不要替代项目能力。

> 核心建议：对当前阶段来说，**能解释请求从 HTTP 到数据库、再到 JSON 响应的完整路径，并能自行实现和排错**，比直接背三级缓存源码细节更有价值。

---

## 10. 自测问题（先尝试不看答案）

1. `Spring Framework` 和 `Spring Boot` 的关系是什么？Spring Boot 是否替代了 Spring 容器？
2. `@RestController` 是 Spring Framework、Spring MVC 还是 Spring Boot 的功能？
3. 为什么要使用构造器注入？它和直接 `new` 的关键区别是什么？
4. `@Component` 标注的类在什么情况下仍可能没有进入容器？
5. `BeanFactory` 与 `ApplicationContext` 的关系是什么？
6. 一个 `/users/1` 请求如何找到 `UserController.findById`？
7. `HandlerMapping` 与 `HandlerAdapter` 的职责有什么不同？
8. `@RequestBody` 为什么能把 JSON 转成 Java 对象？
9. `@Controller` 返回模板视图与 `@RestController` 返回 JSON 有什么不同？
10. `@Around` 为什么通常需要 `ProceedingJoinPoint.proceed()`？
11. 为什么 `this.method()` 可能使 `@Transactional` 不生效？
12. 自动配置是否意味着“引入 Boot 就会创建 DataSource”？
13. 为什么不能把 `Controller → Service → Mapper` 称为“Spring MVC 的内部调用链”？
14. 在 Boot 3 和 Boot 4 中，常用 MVC Starter 命名有什么区别？
15. Spring Boot 默认允许循环引用吗？遇到循环依赖首先该怎么做？

---

## 11. 参考资料（官方文档）

以下链接用于核对和进一步学习；文中“原聊天核对结论”是**对本次聊天原有表述的修订**，不是把官方文档里的某段文字原样翻译。

- [Spring Framework：IoC 容器与 Bean](https://docs.spring.io/spring-framework/reference/core/beans/introduction.html)
- [Spring Framework：依赖注入与构造器注入](https://docs.spring.io/spring-framework/reference/core/beans/dependencies/factory-collaborators.html)
- [Spring Framework：AOP Around Advice](https://docs.spring.io/spring-framework/reference/core/aop/ataspectj/advice.html)
- [Spring Framework：AOP 代理与自调用问题](https://docs.spring.io/spring-framework/reference/core/aop/proxying.html)
- [Spring MVC：Servlet Request Processing](https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-servlet/sequence.html)
- [Spring MVC：特殊组件（HandlerMapping / HandlerAdapter）](https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-servlet/special-bean-types.html)
- [Spring MVC：@ResponseBody 与消息转换](https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-controller/ann-methods/responsebody.html)
- [Spring Boot：Starter 列表和版本差异](https://docs.spring.io/spring-boot/reference/using/build-systems.html)
- [Spring Boot：@SpringBootApplication](https://docs.spring.io/spring-boot/reference/using/using-the-springbootapplication-annotation.html)
- [Spring Boot：自动配置与条件注解](https://docs.spring.io/spring-boot/reference/features/developing-auto-configuration.html)
- [Spring Boot：应用包结构及组件扫描](https://docs.spring.io/spring-boot/reference/using/structuring-your-code.html)
- [Spring Boot：配置属性（循环依赖默认值）](https://docs.spring.io/spring-boot/appendix/application-properties/)

**核对完成标记：** 已纠正 AOP 示例、字段注入写法、MVC 职责边界与请求过程、Boot 的自动配置与服务器关系，并补充了 Boot 3/4 的 Starter 差异。
