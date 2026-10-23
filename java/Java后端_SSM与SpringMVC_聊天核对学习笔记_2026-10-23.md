---
title: Java 后端学习笔记：参数校验、请求映射、SSM 与工程设计
created: 2026-10-23
status: 已核对并修订
scope: 本次聊天内容（@Valid、@RequestMapping、SSM 学习与架构设计建议）
tags: [Java, Spring, Spring-MVC, Spring-Boot, SSM, MyBatis, Validation, 后端架构]
---

# Java 后端学习笔记：`@Valid`、`@RequestMapping` 与 SSM 学习路线

> **笔记说明**：根据本次聊天记录整理，并对照 Spring Framework、Spring Boot、Jakarta Validation、MyBatis 官方资料核对。保留原聊天中的核心解释与学习建议，对过于绝对或不够准确的地方作了修订。技术事实与个人学习规划已明确区分。
>
> **核对日期**：2026-10-23。代码主要按照 **Spring Boot 3.x + Spring MVC 6.x + Jakarta Validation** 的常见项目风格撰写；不同版本可能有少量差异。

## 目录

1. [正确性核查结论](#一正确性核查结论)
2. [`@Valid`：请求参数校验](#二valid请求参数校验)
3. [`@RequestMapping`：HTTP 请求映射](#三requestmappinghttp-请求映射)
4. [一个请求到数据库的完整流程](#四一个请求到数据库的完整流程)
5. [SSM、Spring Boot 与工程设计的关系](#五ssmspring-boot-与工程设计的关系)
6. [适合当前阶段的学习路径](#六适合当前阶段的学习路径)
7. [小项目练习与验收标准](#七小项目练习与验收标准)
8. [易错点与自测题](#八易错点与自测题)
9. [官方参考资料](#九官方参考资料)

---

## 一、正确性核查结论

**总体评价**：聊天中对 `@Valid`、`@RequestMapping`、SSM 学习顺序与三层架构的**主体解释正确**，但有以下需要补充的细节。

| 原聊天的说法/主题 | 核查结果 | 更准确的结论 |
|---|---|---|
| `@Valid` 用于触发校验 | ✅ 正确 | 它是 Jakarta Validation 注解，用来触发级联校验；在 Spring MVC 的 `@RequestBody`、`@ModelAttribute` 等参数上，也可使该对象执行 Bean Validation 约束校验。它本身**不是**“非空”等约束。 |
| `@RequestBody` 把 JSON 转 Java 对象 | ✅ 常见场景正确 | 更精确地说，`@RequestBody` 使用 `HttpMessageConverter` 读取并转换请求体；JSON 转换只是最常见的场景。 |
| 校验失败通常抛 `MethodArgumentNotValidException` 并返回 400 | ⚠️ 需要限定场景 | 在典型的 `@Valid @RequestBody DTO` 对象参数场景成立；若触发 Spring MVC **方法级校验**，可抛 `HandlerMethodValidationException`（Spring Framework 6.1+）。使用紧邻参数的 `BindingResult` 还可由方法自行处理。 |
| `@Email` 验证邮箱 | ⚠️ 需要补充 | **`null` 默认视为有效**；字段必填时加 `@NotBlank` 或合适的非空约束。 |
| `@Valid` 和 `@Validated` 的区别 | ✅ 方向正确，但过于简化 | `@Valid` 是 Jakarta 标准，支持对象级联校验；`@Validated` 是 Spring 扩展，支持校验组，并常用于开启 Spring Bean 的方法级校验。Spring MVC 6.1+ 的 Controller 方法校验还有额外规则。 |
| `@RequestMapping` 定义 URL 映射 | ✅ 正确 | 除路径，还可用 HTTP 方法、查询参数、请求头、请求/响应媒体类型等条件匹配请求；不指定 `method` 时默认不限具体 HTTP 方法。 |
| 类与方法上的映射组合 | ✅ 正确 | 类上的路径是共享映射，方法上进一步指定接口路径与条件。 |
| 系统学 SSM 有利于从项目学习困境中脱身 | ✅ 合理建议 | 属于**学习策略判断**，不是普遍的技术定律；推荐“系统补基础 + 小项目同步验证”，而不是长期彻底停止项目实践。 |
| 12～17 天学 SSM | ℹ️ 经验估计 | 不是官方标准，不代表“学会全部内容”。应按掌握程度调整。 |
| Controller → Service → Mapper → MySQL | ✅ 合理入门模型 | 是常见业务分层，不是 Spring 强制的唯一架构。事务通常应放在**业务操作边界**，常见于 Service 层。 |

**特别提醒版本差异**：

- Spring Boot **3.x** 使用 `jakarta.validation.*`，而旧 Spring Boot 2.x 项目常见 `javax.validation.*`，不要混用。
- Spring Boot 3.x 常使用 `spring-boot-starter-web`；Spring Boot 4.x 的官方 Starter 列表出现更明确的 `spring-boot-starter-webmvc`，并将旧 `spring-boot-starter-web` 标为弃用。照教程配置依赖时先确认自己的 Boot 版本。
- MyBatis-Spring-Boot-Starter 要选择与 Spring Boot 主版本兼容的版本，不能盲目复制网上的 Starter 版本号。[7]

---

## 二、`@Valid`：请求参数校验

### 2.1 它解决什么问题？

前端可能传来：用户名为空、年龄太小、邮箱不符合格式等数据。我们希望在进入具体业务逻辑前执行校验，而不是每个接口都写大量 `if`。

**一句话记忆**：

> 约束注解（如 `@NotBlank`、`@Min`）描述“**什么是合法数据**”；`@Valid` 在支持它的校验环境中参与“**执行这些校验**”。

`@Valid` 的导入：

```java
import jakarta.validation.Valid;
```

Spring Boot 3.x 项目通常需要校验实现（一般通过以下 Starter 提供）：

```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-validation</artifactId>
</dependency>
```

### 2.2 典型例子：注册接口

**DTO（数据传输对象）**：

```java
import jakarta.validation.constraints.Email;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;

public class UserRegisterDTO {
    @NotBlank(message = "用户名不能为空")
    private String username;

    @NotNull(message = "年龄不能为空")
    @Min(value = 18, message = "年龄不能小于18岁")
    private Integer age;

    @NotBlank(message = "邮箱不能为空")
    @Email(message = "邮箱格式错误")
    private String email;

    // 省略 getter/setter（也可通过 Lombok @Data 生成）
}
```

**Controller**：

```java
import jakarta.validation.Valid;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class RegisterController {
    @PostMapping("/register")
    public String register(@Valid @RequestBody UserRegisterDTO user) {
        return "注册成功";
    }
}
```

**流程**：

```text
客户端发送 JSON
    ↓
Spring MVC 根据 @RequestBody 使用消息转换器生成 UserRegisterDTO
    ↓
@Valid 触发 DTO 字段约束校验
    ├─ 通过 → 进入 Controller 方法 → 业务处理
    └─ 不通过 → 进入校验错误处理流程（典型默认结果为 HTTP 400）
```

> 注意：若输入不是合法 JSON、字段类型根本无法转换，可能在消息转换环节失败（如 `HttpMessageNotReadableException`），和 Bean Validation 的“字段约束不通过”不是同一个问题。

### 2.3 常用校验注解

| 注解 | 作用 | 典型使用场景 |
|---|---|---|
| `@NotNull` | 值不能是 `null` | `Integer`、`Long`、对象引用 |
| `@NotBlank` | `CharSequence` 非 `null`，并含至少一个非空白字符 | 用户名、标题 |
| `@NotEmpty` | 字符串、数组、集合、Map 非 `null` 且非空 | 非空列表 |
| `@Min(18)` | 数值 ≥ 18 | 年龄、最低数量 |
| `@Max(100)` | 数值 ≤ 100 | 评分上限 |
| `@Size(min = 2, max = 20)` | 字符串长度或集合/数组大小区间 | 用户名长度 |
| `@Email` | 邮箱格式约束；`null` 默认通过 | 邮箱格式 |
| `@Pattern(regexp = "...")` | 正则匹配；`null` 默认通过 | 手机号格式等 |

**最常混淆：**

- `@NotNull`：`""`、`"   "` 仍不为 `null`，所以它们可以通过。
- `@NotEmpty`：`"   "` 不是空字符串，因此可能通过。
- `@NotBlank`：`"   "` 不通过。用于必须提供内容的字符串通常更合适。
- `@Email` / `@Size` / `@Pattern` 并不意味着“必填”；必填条件要单独声明。

### 2.4 `@Valid` 与 `@Validated`

| 比较 | `@Valid` | `@Validated` |
|---|---|---|
| 来源 | `jakarta.validation.Valid` | `org.springframework.validation.annotation.Validated` |
| 主要作用 | 校验级联对象；在 Spring MVC 参数绑定场景可触发对象校验 | Spring 的校验扩展，支持校验分组等 |
| 分组选择 | 不直接指定分组 | 可指定校验组，如 `@Validated(CreateGroup.class)` |
| 常见位置 | `@RequestBody` DTO 参数、对象字段 | Controller 的分组校验参数；Service 类上的方法级校验 |

**嵌套对象校验的关键：**

```java
public class OrderDTO {
    @NotNull
    @Valid
    private AddressDTO address;
}
```

这里 `@Valid` 表示还要继续校验 `address` 对象内部的约束；`@NotNull` 保证 `address` 本身不能缺失。仅用 `@Valid`，`null` 引用一般不会因为级联校验而失败。

**进阶补充（了解即可）**：Controller 方法级校验可能由直接写在方法参数上的 `@NotBlank`、`@Min` 等约束触发，此时异常类型与普通 DTO 对象校验不同。Spring Framework 6.1+ 内置了 MVC 方法级校验；如果要使用这一机制，官方建议不要在 Controller 类上额外保留用于 AOP 方法校验的类级 `@Validated`。[2]

### 2.5 校验失败怎么处理？

在典型的 `@Valid @RequestBody DTO` 场景：

- 不传 `BindingResult` 时，失败通常触发 `MethodArgumentNotValidException`，默认响应 HTTP 400。
- 如果在该参数**紧邻其后**声明 `BindingResult` / `Errors`，可以在 Controller 方法中自行处理该参数的绑定和校验错误。
- Spring MVC 方法级校验可能触发 `HandlerMethodValidationException`。项目做统一异常处理时要结合自己使用的 Spring 版本覆盖相应异常。[1][2]

**实战掌握目标**：用 Postman、Apifox 或 `curl` 发一个空用户名请求，能解释它为什么没有进入正常业务逻辑，并看到请求状态码及异常处理结果。

---

## 三、`@RequestMapping`：HTTP 请求映射

### 3.1 核心作用

`@RequestMapping` 告诉 Spring MVC：**符合哪些条件的 HTTP 请求，应该交给哪个 Controller 方法。**

```java
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/admin/employee")
public class EmployeeController {

    @GetMapping("/list")
    public String list() {
        return "员工列表";
    }

    @PostMapping("/save")
    public String save() {
        return "保存员工";
    }
}
```

对应的请求（假设应用没有额外的 context path）：

| 请求方式 | 请求路径 | 方法 |
|---|---|---|
| GET | `/api/admin/employee/list` | `list()` |
| POST | `/api/admin/employee/save` | `save()` |

类上的 `@RequestMapping` 表示**共享路径/映射条件**，方法上的映射进一步细化接口。单个方法也可以直接使用 `@RequestMapping`。

### 3.2 与其他映射注解的关系

```java
@RequestMapping(value = "/list", method = RequestMethod.GET)
```

与下面的常见写法在这个示例的映射语义上等价：

```java
@GetMapping("/list")
```

| 注解 | 映射的 HTTP 方法 | 典型业务（不是强制规则） |
|---|---|---|
| `@RequestMapping` | 默认不限方法，可自行限定 | 通用/共享映射 |
| `@GetMapping` | GET | 查询 |
| `@PostMapping` | POST | 创建、提交 |
| `@PutMapping` | PUT | 更新或替换资源 |
| `@PatchMapping` | PATCH | 部分更新 |
| `@DeleteMapping` | DELETE | 删除 |

> 同一个**方法上**不要叠加多个 `@GetMapping` / `@PostMapping` / `@RequestMapping` 来企图合并处理；它们都是请求映射注解，Spring 并不会按这种方式同时生效。如果要处理多种 HTTP 方法，可在一个 `@RequestMapping(method = {...})` 中声明，或使用不同方法分别映射。[3]

### 3.3 几个容易混淆的注解

| 注解 | 作用 | 示例 |
|---|---|---|
| `@RequestMapping` | 匹配请求并定位接口 | 类上 `/api/employee` |
| `@RequestBody` | 从**请求体**读取并转换内容 | JSON → DTO |
| `@RequestParam` | 读取**请求参数** | `/list?page=1` |
| `@PathVariable` | 读取路径里的**变量** | `/employee/10` → `id=10` |
| `@RestController` | 声明返回响应体的 Controller（组合了 `@Controller` + `@ResponseBody`） | REST API 类 |

代码示例：

```java
@GetMapping("/{id}")
public String detail(@PathVariable Long id) {
    return "员工ID=" + id;
}

@GetMapping("/list")
public String list(@RequestParam(defaultValue = "1") int page) {
    return "第" + page + "页";
}
```

**口诀**：

> `@RequestMapping` 负责“**请求去哪儿**”；`@RequestBody` / `@RequestParam` / `@PathVariable` 负责“**参数从哪儿来**”。

---

## 四、一个请求到数据库的完整流程

只理解单个注解还不够，应该将它们放进 Spring MVC 的请求处理链路中。

```text
浏览器 / 前端 / Apifox
         │ HTTP 请求
         ▼
      Web 容器（如 Tomcat）
         ▼
 Servlet Filter 链（如鉴权过滤等）
         ▼
 DispatcherServlet（Spring MVC 前端控制器）
         │
         ├─ HandlerMapping：查找匹配的 Controller 方法
         ├─ HandlerInterceptor：适用时执行拦截逻辑
         └─ HandlerAdapter + 参数解析/消息转换
                   │
                   ├─ 解析 @PathVariable / @RequestParam / @RequestBody
                   └─ 按规则执行 @Valid 等参数校验
                           │
                           ▼
                     Controller
                           ▼
                        Service
                           ▼
                         Mapper
                           ▼
                         MySQL
                           │
                           ▼
                    返回业务结果
                           ▼
                消息转换 / HTTP 响应
                           ▼
                         客户端
```

**这张图是便于入门的简化图**：实际运行时涉及更多组件、异常解析器、拦截器的前后回调，且 HandlerAdapter 与参数解析不是严格独立的网络节点。

建议能口述以下问题：

1. Tomcat 和 Spring MVC 分别负责什么？
2. `DispatcherServlet` 根据什么找到对应方法？
3. JSON 如何变成 DTO？
4. `@Valid` 在什么位置生效？
5. Controller 为什么通常不直接写 SQL？
6. 校验异常和业务异常如何统一返回响应？

---

## 五、SSM、Spring Boot 与工程设计的关系

### 5.1 什么是 SSM？

SSM 是常见的技术组合名称：

| 组成 | 主要职责 |
|---|---|
| Spring（Framework） | IoC、依赖注入、AOP、事务等基础能力 |
| Spring MVC | HTTP 请求映射、参数解析、接口响应等 Web MVC 能力 |
| MyBatis | SQL 映射、Mapper 接口、数据库访问 |

**Spring Boot 不等于 Spring MVC，也不是 MyBatis 的替代品**。它通过自动配置、Starter 等机制，减少 Spring 应用的初始化和配置负担。

因此更合理的学习方式是：

> 用 **Spring Boot 启动项目**，系统学习项目底层用到的 **Spring Core + Spring MVC + MyBatis**；不用为了“正统 SSM”刻意回到大量 XML 配置的老项目。

### 5.2 为什么直接跟项目学容易吃力？

原聊天讨论的核心困难是：你还在查基础注解的含义，实际大型项目就已经涉及 JWT、Redis、事务、Agent、权限、缓存、接口与分层等内容。

这会让三类问题同时出现：

- **框架机制**：Bean 如何创建？Controller 如何接请求？Mapper 怎么执行 SQL？
- **业务逻辑**：订单该怎么创建？库存何时扣减？评价能否重复提交？
- **工程设计**：模块如何划分？哪些逻辑放 Service？如何定义事务边界？

缺乏第一层知识时，做第二层和第三层会不断卡住。因此建议短期补齐基础，但不必等“所有源码研究完毕”再动手。

### 5.3 架构设计分三个层次

| 层次 | 具体内容 | 当前优先级 |
|---|---|---|
| 代码/模块设计 | 分层、DTO/VO/Entity、接口职责、依赖方向 | **必须先掌握** |
| 应用架构 | 权限、事务、缓存、异常、日志、配置与模块协作 | **紧接着学习** |
| 分布式架构 | 微服务、消息队列、分布式事务、高并发治理 | **暂不作为主线** |

示例：

```text
POST /orders
  Controller：接收 OrderCreateDTO、执行入参校验
      ↓
  OrderService：检查商品状态、校验库存、计算金额、生成订单
      ↓
  OrderMapper / StockMapper：操作订单表、库存表
      ↓
  MySQL：提交同一数据库事务
```

**工程边界提醒**：多表写入可考虑在合适的 Service 操作上使用 `@Transactional`。但 `@Transactional` **不是给所有外部系统提供原子性的万能开关**：例如真实第三方支付、远程服务调用不会仅凭本地数据库事务就自动一起回滚。

Spring 声明式事务还常依赖代理调用：同一个对象内部的“自调用”可能绕过代理，导致被调用方法上的事务注解没有按期生效；默认回滚通常针对 `RuntimeException` 和 `Error`，受检查异常是否回滚还要看配置。[6]

### 5.4 DTO、VO、Entity 的职责

| 类型 | 主要用途 | 例子 |
|---|---|---|
| DTO | 接收/传递业务数据，通常带请求校验 | `EmployeeCreateDTO` |
| Entity | 数据持久化模型，通常映射数据库表 | `Employee` |
| VO | 返回给客户端的展示数据 | `EmployeeVO` |

这些是**常见工程命名约定**，并不是由 Spring 强制规定的特殊语法。它们是否需要单独定义，取决于系统规模和数据边界；入门可先用明确的三个类型理解职责。

---

## 六、适合当前阶段的学习路径

> 这一节属于**结合聊天背景提出的学习安排**，不是框架官方推荐的固定课时。目标是能够独立构建一个简单 Spring Boot + MyBatis 业务模块，并解释设计理由。

### 建议：以 Spring Boot 运行示例，按原理逐步学习

| 阶段 | 建议学习时间（参考） | 核心知识 | 完成标志 |
|---|---|---|---|
| 1. Spring 核心 | 2～3 天 | IoC、DI、Bean、生命周期、AOP/代理基础 | 能解释 Bean 从哪里来、依赖如何注入 |
| 2. Spring MVC | 2～3 天 | Controller、参数绑定、请求映射、校验、异常、拦截器、DispatcherServlet | 能自己写并调通 REST 接口 |
| 3. Spring Boot | 2～3 天 | Starter、自动配置、`application.yml`、环境配置、包扫描 | 能解释为什么少写配置仍可启动 |
| 4. MyBatis + MySQL | 3～4 天 | SQL、Mapper、动态 SQL、关联查询、分页、事务、MyBatis-Plus | 能自己完成持久化 CRUD |
| 5. 工程设计 | 3～4 天 | Controller/Service/Mapper、DTO/VO/Entity、统一响应、异常、事务边界 | 能独立设计一个业务模块 |

**时间合计**：约 12～17 天，仅供安排节奏；不应理解为两周能“精通 SSM”。MySQL 和 SQL 基础如较薄弱，要额外补充练习。

学习方式建议采用一个循环：

```text
学一个机制（例如 Bean 注入）
    ↓
自己写一个最小例子
    ↓
解释它为什么能工作
    ↓
放进员工管理项目中验证
    ↓
发现不懂的再针对性补基础
```

### 学到什么程度可以继续大型项目？

不用等懂 Spring 全部源码。只要能比较独立地完成以下内容，就可以回到园区点餐等大项目：

- 从零创建可启动的 Spring Boot Web 项目；
- 用 `@RestController` 实现增删改查接口；
- 会用 `@RequestMapping`、`@RequestBody`、`@PathVariable`、`@RequestParam`；
- 使用 `@Valid` 和常见约束注解；
- Controller → Service → Mapper → MySQL 的完整调用链能自己搭起来；
- 能解释 DTO、VO、Entity 和事务的职责；
- 能独立排查常见 400、404、500、SQL 异常；
- 接口失败时能区分参数错误、业务错误、系统错误。

---

## 七、小项目练习与验收标准

### 项目：`employee-management`（员工管理）

不要另开一个庞大项目，先把 SSM 关键点都落实在一个小项目上。

**建议业务范围**：

1. 员工新增：校验姓名、年龄、邮箱等字段；
2. 员工列表：支持分页和姓名关键字查询；
3. 员工详情：使用路径参数查询；
4. 员工更新和删除；
5. 部门管理：部门与员工关联查询；
6. 全局异常处理、统一响应；
7. 至少一个明确需要事务的业务操作；
8. 使用 Apifox、`curl` 或测试代码验证正常与异常输入。

**建议目录结构**（示意，不是唯一标准）：

```text
com.example.employee
├─ EmployeeApplication.java
├─ controller/
│  └─ EmployeeController.java
├─ service/
│  ├─ EmployeeService.java
│  └─ impl/EmployeeServiceImpl.java
├─ mapper/
│  └─ EmployeeMapper.java
├─ dto/
│  └─ EmployeeCreateDTO.java
├─ vo/
│  └─ EmployeeVO.java
├─ entity/
│  └─ Employee.java
├─ exception/
│  └─ GlobalExceptionHandler.java
└─ common/
   └─ Result.java
```

**验收任务（不要只看视频）：**

- [ ] 自己解释 Spring 为什么能注入 `EmployeeService`。
- [ ] 自己写出 `@RequestMapping` + `@GetMapping` 的路径组合。
- [ ] 给创建员工 DTO 添加 `@NotBlank`、`@Email`、`@Min` 等校验。
- [ ] 用无效 JSON、合法 JSON 但字段违规的数据分别测试。
- [ ] 自己写一个 Mapper SQL 并验证数据库发生变化。
- [ ] 解释为什么事务通常定义在 Service 的完整业务操作上。
- [ ] 修改一个接口时能判断代码应该改 Controller、Service 还是 Mapper。
- [ ] 能分析一次“调用失败”的实际处理链路和日志。

**和聊天中提及的大型项目连接**：完成小项目后，优先将同一套分层、参数校验、异常管理与事务思路迁移到园区点餐项目。JWT、Redis、WebSocket 和 Agent 功能在掌握核心业务链路后逐项引入，避免同时引入太多新概念。

---

## 八、易错点与自测题

### 易错点速查

1. **`@Valid` 不等于 `@NotNull`**。对象是否必填需要单独声明。
2. **`@Email` 不是“必填邮箱”**。需要和 `@NotBlank` 组合。
3. **JSON 解析失败 ≠ Bean Validation 失败**。它们可能抛不同异常。
4. **`@RequestMapping` 不只是匹配 URL**，还能匹配 HTTP 方法等条件。
5. **不要在同一方法上堆多个请求映射注解**，以为会组合成多路由。
6. **Spring Boot 不是 SSM 三个框架的替代合集**，它主要负责简化应用启动和配置。
7. **三层架构是常见工程实践，不是 Spring 强制结构**。
8. **`@Transactional` 不保证所有网络/外部支付操作统一原子回滚**。
9. **框架学多久没有统一标准**，以能解释与独立实现为准。

### 自测（先想再看答案）

**Q1：`@RequestMapping("/api/admin/employee")` 放在类上，方法是 `@GetMapping("/list")`，访问路径是什么？**  
A：GET `/api/admin/employee/list`（假定应用没有额外的 context path）。

**Q2：`@Valid` 本身会拒绝所有 `null` 吗？**  
A：不会。它不是非空约束，字段非空需要 `@NotNull`、`@NotBlank` 等。

**Q3：只加 `@Email`，邮箱传 `null` 一定失败吗？**  
A：不会；按标准 `null` 被视为有效。

**Q4：`@RequestBody` 与 `@RequestParam` 最大区别是什么？**  
A：前者读取 HTTP 请求体并经消息转换；后者读取请求参数。

**Q5：在 Service 中扣库存后写订单，两步要么都成功要么都回滚，应先想什么？**  
A：先判断是否属于同一数据库事务边界，通常考虑在 Service 业务操作上使用 `@Transactional`，并确认调用路径和异常回滚条件。

**Q6：能使用 Spring Boot 后，是否可以完全不懂 Spring MVC？**  
A：不建议。Spring Boot 能简化配置，但 HTTP 请求映射、参数解析等核心工作仍由 Spring MVC 等框架承担。

**Q7：什么时候开始学架构设计？**  
A：从独立设计代码和业务分层开始，而不是等学完微服务或分布式再开始。

---

## 九、官方参考资料

以下是本次核对使用的官方或规范资料。为了避免版本不一致，使用示例前请确认项目中的 Spring Framework / Spring Boot / MyBatis 版本。

1. **Spring Framework — `@RequestBody`**（请求体转换、参数校验和默认 400）：  
   https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-controller/ann-methods/requestbody.html
2. **Spring Framework — MVC Validation**（对象校验与方法级校验、两类异常、`@Valid`）：  
   https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-controller/ann-validation.html
3. **Spring Framework — Mapping Requests**（`@RequestMapping`、HTTP 方法快捷注解、类与方法映射）：  
   https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-controller/ann-requestmapping.html
4. **Jakarta Validation — 约束注解规范**（`@NotBlank`、`@NotEmpty`、`@Email` 等）：  
   https://jakarta.ee/specifications/bean-validation/3.1/jakarta-validation-spec-3.1.pdf
5. **Spring Boot — Validation / Starters**（方法级校验、Validator 与 Starter）：  
   https://docs.spring.io/spring-boot/3.5/reference/io/validation.html  
   https://docs.spring.io/spring-boot/reference/using/build-systems.html
6. **Spring Framework — Using `@Transactional`**（代理调用、默认回滚规则等）：  
   https://docs.spring.io/spring-framework/reference/data-access/transaction/declarative/annotations.html
7. **MyBatis — MyBatis Spring Boot Starter**（自动集成和版本兼容）：  
   https://mybatis.org/spring-boot-starter/mybatis-spring-boot-autoconfigure/
8. **Spring Framework — Java Bean Validation**（Bean Validation 的基本概念）：  
   https://docs.spring.io/spring-framework/reference/core/validation/beanvalidation.html

---

## 十、最终复习结论

**本次聊天最值得带走的四句话：**

1. `@Valid` 让 Spring MVC 在合适的参数场景中执行 DTO 约束校验；`@NotBlank`、`@Min` 等才是具体规则。
2. `@RequestMapping` 决定请求对应的处理方法；`@RequestBody`、`@RequestParam`、`@PathVariable` 决定参数来源。
3. Spring Boot 简化搭建，但 Spring、Spring MVC、MyBatis 的工作机制仍然值得系统学习。
4. 当前优先级是**从零实现一个小而完整的业务模块**，然后在真实项目中逐步提高事务、模块设计和架构能力，而不是先追求复杂微服务。
