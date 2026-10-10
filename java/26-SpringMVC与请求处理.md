# 26 · Spring MVC 与请求处理

> 一句话概括：Spring MVC 是 Spring 的 Web 模块——DispatcherServlet 接收请求，HandlerMapping 找方法、HandlerAdapter 调方法、HttpMessageConverter 把返回值变成响应体。

## 一、一次请求的完整旅程

```text
浏览器/App
  │ HTTP 请求（DNS→TCP/TLS→HTTP）
  ▼
[Nginx，可选：反向代理/HTTPS/静态资源]
  ▼
Tomcat：接收连接、解析 HTTP、执行 Filter 链
  ▼
DispatcherServlet（Spring MVC 核心 Servlet，前端控制器）
  ├─ HandlerMapping：根据路径+方法找 Handler（"该执行谁"）
  ├─ HandlerInterceptor：preHandle →（false 则拦截）
  ├─ HandlerAdapter：参数解析、"123"→Long、调用方法（"怎样调"）
  ▼
Controller → Service → Mapper → MySQL
  ▼
返回值 → HttpMessageConverter（如 Jackson 序列化 JSON）→ HTTP 响应
```

**区分三个维度**：Tomcat/Servlet 是 Web 运行基础；Spring MVC 负责路由、参数绑定、响应组织；Spring/Spring Boot 管对象和启动。

## 二、Controller 注解精讲

```java
@RestController
@RequestMapping("/users")
public class UserController {
    @GetMapping("/{id}")
    public String getUser(@PathVariable("id") Long id) {   // GET /users/123
        return "用户ID：" + id;
    }
}
```

| 注解 | 用途 |
|---|---|
| `@GetMapping` / `@PostMapping` / `@PutMapping` / `@PatchMapping` / `@DeleteMapping` | 查询 / 创建 / 整体替换 / 部分更新 / 删除 |
| `@PathVariable` | 路径变量 `/users/1` |
| `@RequestParam` | 查询或表单参数 `/users?id=1` |
| `@RequestBody` | JSON 请求体 → Java 对象（经 Jackson） |

要点：类上 `@RequestMapping("/users")` + 方法上 `@GetMapping("/{id}")` ⇒ 完整路径 `/users/123`；`/{id}` 是路径模板不是字符串插值；初学建议显式写 `@PathVariable("id")`，不依赖编译参数名保留。`@RestController` 不一定返回 JSON——`String` 直接写成文本，对象在配置了消息转换器后序列化为 JSON。

## 三、启动阶段 vs 请求阶段

**启动**（`SpringApplication.run`）：创建容器 → 扫描注册 Bean → 自动配置 → 启动内嵌服务器 → 注册 `DispatcherServlet` → 建立请求映射。

**请求**：按已建立的映射执行。默认同步 Servlet 模型中业务代码跑在容器工作线程上，多请求并发——**单例 Bean 里不要保存请求私有的可变状态**。

## 四、HandlerInterceptor 拦截器

Spring MVC 体系的拦截器，在 Controller 前后执行公共逻辑（鉴权、日志、耗时）：

| 回调 | 时机 | 用途 |
|---|---|---|
| `preHandle` | 处理器执行前 | 鉴权；返回 false 阻止继续 |
| `postHandle` | 处理器执行后、视图渲染前 | 调整模型；REST 场景用途有限 |
| `afterCompletion` | 请求完成清理阶段 | 日志、清理 ThreadLocal |

注册要显式配置路径，只写 `@Component` 不代表自动拦截所有请求：

```java
registry.addInterceptor(loginInterceptor)
        .addPathPatterns("/**")
        .excludePathPatterns("/api/auth/login");
```

**三种"拦截"的分工**：Servlet Filter（Servlet 层，DispatcherServlet 之前）/ MVC Interceptor（围绕 Handler）/ Spring AOP（方法级切面）。Filter 也能使用 Spring Bean，不能说"Filter 访问不了容器"。（JWT 登录校验的完整链路见 `33-密码安全与JWT鉴权.md`）

## 五、接口文档

接口文档 = 前后端协作的 API 说明书：地址与方法、认证要求、参数结构与校验规则、成功/失败状态码与响应示例。用 Apifox/Postman/Swagger 管理。**"导入接口文档"只是把 OpenAPI 描述导进工具方便测试，不等于后端接口已实现**；反过来可用 springdoc-openapi 从代码生成文档。

## 📌 易错点

- "Spring MVC 就是服务器"——它是框架；Tomcat 提供 HTTP/Servlet 运行环境。
- "Tomcat 直接按 `@GetMapping` 找 Controller"——通常先经 `DispatcherServlet` 分发。
- "`preHandle` 返回 false 就自动 401"——不会，要自己设置状态或抛异常。
- "Controller 直接收发网络包"——网络由 Tomcat 处理，Controller 只处理映射到的逻辑入口。
- "Java 对象直接传给浏览器"——必须序列化为 JSON 等格式。
- "`afterCompletion` 总会执行"——受 preHandle 是否通过等限制。

## 🧠 速记口诀

```text
DispatcherServlet 是入口：Mapping 找人、Adapter 调人、Converter 写响应
参数三件套：Path 取路径、Param 取查询、Body 取 JSON
拦截三回调：pre 拦、post 改、after 清理
单例 Bean 不存请求私有状态——线程并发会打架
```

## ✅ 自测问题

1. 访问 `http://localhost:8080/user/123`，从 URL 到 Java 方法调用中间发生哪些事？
2. `HandlerMapping` 和 `HandlerAdapter` 各解决什么问题？
3. `@PathVariable`、`@RequestParam`、`@RequestBody` 分别处理什么参数？
4. Filter、Interceptor、AOP 的作用范围和典型用途有何不同？
5. 为什么单例 Controller 里不能存"当前请求"的可变字段？

> 来源：原始《Java 后端学习笔记：Spring、Web 请求处理与数据库设计》(Java后端学习笔记_Spring与Web请求处理_2026-10-08.md §0/3/4/9)、《Java 后端学习笔记：从 HTTP 请求到 Spring Boot》(Java后端学习笔记_从HTTP请求到SpringBoot.md §6/7/9)、《Spring 基础学习笔记》(20261010-Spring基础学习笔记.md §6/7)，重叠部分已去重合并。
