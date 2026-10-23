# Spring MVC、Tomcat、Servlet 与三层架构：核对版学习笔记

> 核对日期：2026-10-23  
> 适用场景：典型 Spring Boot + Spring MVC + Tomcat + MyBatis + MySQL 同步 REST API 项目。  
> 来源范围：本次聊天的讲解、代码示例，以及 Spring Framework / Spring Boot / MyBatis 官方文档。**并非对其他历史聊天中所有 Spring 知识点的核查。**

## 1. 本次聊天的准确性核对

**整体判断：核心思路正确，存在少量为便于入门而采用的简化表述。**

| 原说法 | 结论 | 更准确的理解 |
|---|---|---|
| Tomcat 接收 HTTP 请求，运行 Servlet | 正确 | Tomcat 是 Web 服务器和 Servlet 容器，管理 Servlet 生命周期并处理 HTTP 交互。 |
| Servlet 是处理 HTTP 请求、响应的规范 | 基本正确 | Servlet 是 Java 服务端组件规范；`Servlet` 是接口，HTTP 请求常通过 `HttpServlet` 处理，不要将 Servlet 等同于某一个 API 方法。 |
| DispatcherServlet 是 Spring MVC 的前端控制器 | 正确 | 它本身就是 Servlet，由容器调用；实际路由与方法调用还涉及 HandlerMapping、HandlerAdapter。 |
| Tomcat 不直接调用 Controller | 对于所述场景正确 | Tomcat 调用匹配的 Servlet（例如 DispatcherServlet），后者通过 MVC 组件调用 Controller 方法。 |
| Controller → Service → Mapper 是 Spring MVC 三层 | **应避免这种说法** | 这是项目常见的三层组织，**不是 Spring MVC 自带且强制要求的三层**；Spring MVC 主要处理 Web 层。 |
| Mapper 执行 SQL | 通俗讲法可用 | Mapper 接口通常由 MyBatis 创建代理对象，最终通过执行器、JDBC 等访问数据库；接口本身不是手动写的 SQL 执行引擎。 |
| Spring Boot 自动整合 Tomcat | 有条件正确 | 典型 `spring-boot-starter-web` Servlet 技术栈默认引入嵌入式 Tomcat；Spring Boot 不限于 Web 应用，也可配置其他服务器或使用 WebFlux。 |
| Controller 返回对象后会转成 JSON | 在示例中正确 | `@RestController` 场景由 MVC 返回值处理及 `HttpMessageConverter`（常见 Jackson）序列化；不是所有 Controller 返回值都转 JSON。 |
| 返回链路就是 Mapper→Service→Controller→Spring MVC→Tomcat | 简化正确 | 还会涉及返回值处理、异常解析、过滤器/拦截器及 HTTP 响应写出等步骤，具体取决于配置。 |

## 2. 五个概念各自的位置

### 2.1 Tomcat：服务器与容器

- 在 HTTP 连接层面接收请求、组织响应。
- 根据 Servlet 映射交给对应 Servlet 处理。
- 管理 Servlet 初始化、调用和销毁等生命周期。
- **Tomcat 不是业务代码层，也不是 Service/Mapper 的上级。**

### 2.2 Servlet：Java Web 组件规范

- 核心接口：`jakarta.servlet.Servlet`（Spring Boot 3+ 常见）；旧版 Java EE 使用 `javax.servlet.*`。
- 常见抽象类：`HttpServlet`，提供 `doGet()`、`doPost()` 等方法。
- Servlet 容器（例如 Tomcat）负责运行符合规范的 Servlet。
- **Spring MVC 并非绕过 Servlet，而是构建在 Servlet 之上。**

### 2.3 Spring MVC：Web 请求处理框架

- `DispatcherServlet`：统一入口，负责调度。
- `HandlerMapping`：根据请求寻找处理器以及相关拦截器。
- `HandlerAdapter`：以合适方式执行处理器（包括注解式 Controller 方法）。
- 参数解析与数据绑定：例如 `@PathVariable`、`@RequestParam`、`@RequestBody`。
- 返回值处理：例如 `@RestController` 返回 JSON。
- 异常处理：例如 `@ExceptionHandler`、`@ControllerAdvice`。

### 2.4 Controller / Service / Mapper：项目代码分层

| 层 | 主要职责 | 典型技术 | 注意 |
|---|---|---|---|
| Controller | HTTP 接口、参数接收、响应 | Spring MVC | 不宜塞入大量业务代码 |
| Service | 业务规则、业务流程、事务边界 | Spring Bean / `@Service` / `@Transactional` | 不必为每个接口机械建立无意义的转发层 |
| Mapper | 数据持久化接口、数据操作 | MyBatis / MyBatis-Plus | 并非 Spring MVC 自带 |

**关键区分：**Spring MVC 是框架，三层架构是开发者的组织方式。Service 可以使用 Mapper，也可以访问其他服务、缓存或消息系统；Mapper 也并非所有项目必有。

### 2.5 Spring Boot：自动配置和启动整合

- 方便配置和启动 Spring 应用。
- 在典型 Servlet Web 项目中自动配置 Spring MVC、DispatcherServlet、嵌入式 Tomcat 等。
- 不是 Spring MVC/Tomcat/Servlet 的替代品。
- 一个普通非 Web 的 Spring Boot 应用可以不启动 HTTP 服务器。

## 3. 一次 GET /dish/1 的典型真实处理过程

```text
浏览器 / Apifox / App
  │  GET /dish/1
  ▼
Tomcat：接收 HTTP 请求
  │
  ├─ 可能先经过 Servlet Filter（例如鉴权/编码过滤器）
  ▼
DispatcherServlet（本身就是 Servlet）
  │
  ├─ HandlerMapping：找到匹配 /dish/{id} 的处理方法
  ├─ HandlerInterceptor：如配置了拦截器，执行预处理
  ├─ HandlerAdapter + 参数解析：将路径中的 1 绑定为 Long id
  ▼
DishController.getDish(1)
  │
  ▼
DishService.getById(1)
  │
  ▼
DishMapper.selectById(1)
  │
  ▼
MyBatis 代理 / SQL 执行器 / JDBC → MySQL
  │                          │
  └──────── 查询结果返回 ──────┘
  ▼
Service → Controller 返回 Dish 对象
  │
  ▼
Spring MVC 返回值处理器 / HttpMessageConverter
  │  将 Dish 序列化为 JSON（此处以 @RestController 为前提）
  ▼
Servlet Response / Tomcat
  │
  ▼
客户端收到 HTTP 状态码、响应头、JSON 响应体
```

> 注：此图是单个同步请求的逻辑路径，未展开线程池、网络协议栈、异步处理或全部异常分支；过滤器与拦截器是否执行取决于配置。

## 4. 一个最小代码示例

以下为**示意代码**；省略了 `Dish` 实体、构造函数或实际项目中的依赖注入配置。推荐采用构造器注入。

### 4.1 Controller

```java
@RestController
@RequestMapping("/dish")
public class DishController {
    private final DishService dishService;

    public DishController(DishService dishService) {
        this.dishService = dishService;
    }

    @GetMapping("/{id}")
    public Dish getDish(@PathVariable Long id) {
        return dishService.getById(id);
    }
}
```

解释：`@GetMapping("/{id}")` 交给 Spring MVC 做请求匹配；`@PathVariable` 把路径变量绑定到形参。

### 4.2 Service

```java
@Service
public class DishService {
    private final DishMapper dishMapper;

    public DishService(DishMapper dishMapper) {
        this.dishMapper = dishMapper;
    }

    public Dish getById(Long id) {
        return dishMapper.selectById(id);
    }
}
```

解释：本例业务简单，Service 仅转发调用；真实项目可加入权限、库存、业务规则与事务处理。`@Service` 让类在符合组件扫描条件时注册为 Spring Bean。

### 4.3 Mapper

```java
@Mapper
public interface DishMapper {
    @Select("SELECT id, name, price FROM dish WHERE id = #{id}")
    Dish selectById(Long id);
}
```

解释：MyBatis 创建实现 Mapper 接口的代理对象；`#{id}` 对应参数绑定（而不是直接字符串拼接），随后执行 SQL 并映射结果。需正确配置 MyBatis、数据库连接及表结构。

### 4.4 返回结果

对于一个存在的菜品，响应体可能为：

```json
{
  "id": 1,
  "name": "宫保鸡丁",
  "price": 25
}
```

若查询不到记录、出现异常或字段序列化配置不同，实际响应并不一定是上述 JSON。

## 5. 为什么不用自己写 Servlet？

原生 Servlet 通常需要自行读取参数、判断请求路径、处理响应数据；Spring MVC 通过注解与框架组件完成常见的 HTTP 请求分发、参数绑定和 JSON 序列化。你**仍然通过 Servlet 体系处理 HTTP**，只是不用为每个业务接口手写一个 Servlet。

原生 Servlet 认识示例：

```java
@WebServlet("/hello")
public class HelloServlet extends HttpServlet {
    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        response.setContentType("text/plain;charset=UTF-8");
        response.getWriter().write("hello");
    }
}
```

注意：Spring Boot 中使用 `@WebServlet` 往往还需要启用对应的 Servlet 组件扫描或注册机制；此段主要用于对比原生写法。Spring Boot 3+ 中上述 Servlet 类应导入 `jakarta.servlet` 包。

## 6. 六个易错问题

**Q1：Spring MVC = Controller + Service + Mapper 吗？** 不是。Spring MVC 主要管理 Web 请求处理，三层是项目结构组织方式。

**Q2：Tomcat 能直接执行 Controller 吗？** 在本例中不是，Tomcat 把请求交给 DispatcherServlet，后者委托 MVC 组件调用 Controller。

**Q3：DispatcherServlet 与 Servlet 是并列技术吗？** 不是。DispatcherServlet **本身实现了 Servlet 体系中的规范**（它继承了 Servlet 相关基类）。

**Q4：Mapper 等于 JDBC 吗？** 不是。Mapper 是 MyBatis 的接口抽象；实际数据库访问的底层通常会走 JDBC。

**Q5：Controller 返回 Java 对象必然是 JSON 吗？** 不是。对于本例的 `@RestController`，有合适转换器时通常序列化为 JSON。普通 `@Controller` 还可能进行视图解析。

**Q6：所有 Spring Boot 项目都含 Tomcat 吗？** 不是。Spring Boot 可以构建非 Web 应用，也可以使用其他服务器或响应式 Web 技术栈。

## 7. 建议学习顺序

1. 先熟悉 `@RestController`、`@GetMapping`、`@PostMapping`、`@RequestBody`、`@PathVariable`、`@RequestParam`。
2. 自己写通一个 Controller → Service → Mapper → MySQL 的 CRUD 接口。
3. 理解 Tomcat → DispatcherServlet → HandlerMapping / HandlerAdapter → Controller 的流程。
4. 理解 Filter 和 HandlerInterceptor 的位置、区别及常见使用场景。
5. 进一步学习统一异常处理、Bean Validation、DTO/VO、事务、分页与权限鉴权。
6. 最后按需要阅读 DispatcherServlet、MyBatis Mapper 动态代理的源码。

## 8. 自测题

1. Tomcat 接收 `/dish/1` 后，为什么通常不直接调用 `DishController`？
2. `DispatcherServlet` 到底是不是 Servlet？
3. `@GetMapping` 和 `@PathVariable` 分别负责什么？
4. Service 与 Mapper 属不属于 Spring MVC 的强制要求？
5. `@RestController` 返回对象时，JSON 是在哪一步生成的？
6. 为什么 Mapper 接口只有方法声明也能访问数据库？
7. Spring Boot 与 Spring MVC、Tomcat 分别是什么关系？

### 自测参考答案（简版）

1. 因为 Tomcat 调用 Servlet；本例统一交给 DispatcherServlet，再由 MVC 组件定位执行 Controller。
2. 是，DispatcherServlet 是 Spring MVC 的前端控制器，同时是 Servlet。
3. 前者映射 GET 请求，后者将路径变量绑定到方法参数。
4. 不属于；它们是常见的项目架构/持久层组织方式。
5. Controller 返回后，由 MVC 返回值处理及消息转换器完成。
6. MyBatis 使用 Mapper 代理执行映射语句，通过底层数据库访问组件查询数据。
7. Spring Boot 提供启动和自动配置，Spring MVC 负责 Web 请求处理，Tomcat 提供 HTTP 服务与 Servlet 容器。

## 9. 官方核查资料

- Spring Framework — DispatcherServlet 特殊组件：https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-servlet/special-bean-types.html
- Spring Framework — DispatcherServlet 请求处理过程：https://springframework.org/spring-framework/reference/web/webmvc/mvc-servlet/sequence.html
- Spring Boot — 嵌入式 Web 服务器：https://docs.spring.io/spring-boot/how-to/webserver.html
- MyBatis — MapperProxy：https://mybatis.org/mybatis-3/apidocs/org/apache/ibatis/binding/MapperProxy.html

---

**一句话速记：Tomcat 接请求，Servlet 定规范，Spring MVC 做 Web 调度，Controller 接口接入，Service 处理业务，Mapper 负责数据访问，Spring Boot 负责简化启动和整合。**
