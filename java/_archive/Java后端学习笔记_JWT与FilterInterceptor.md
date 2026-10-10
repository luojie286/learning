# Java 后端学习笔记：JWT、Filter 与 Interceptor

> 整理范围：本次聊天中的 **JWT 令牌是什么、Filter / Interceptor 是什么，以及三者如何协作**。  
> 最早讨论时间：2026-09-20 08:49（UTC+8）。  
> 阅读顺序：**整体用途 → 请求流程 → 局部语法 → 实战建议 → 自测**。

## 1. 这三个东西整体是用来干什么的？

假设你正在做一个 Spring Boot 项目，用户登录之后，要访问 `/user/profile`、`/orders` 等接口。你不希望每一个 Controller 都手动重复判断“用户登录了没有”。

一种常见的解决方案是：

1. **JWT（JSON Web Token）**：后端在用户登录成功后签发的身份凭证。
2. **Filter（过滤器）**：Servlet 层面的请求过滤机制，可以在请求进入 Spring MVC 前统一处理。
3. **Interceptor（拦截器）**：Spring MVC 层面的请求拦截机制，可在执行 Controller 之前进行登录校验等处理。

其中 **JWT 是数据/凭证格式**，Filter 和 Interceptor 是**在请求处理链中执行代码的机制**，不是同一类东西。可以在 Filter 中验证 JWT，也可以在 Interceptor 中验证 JWT，实际项目通常不需要重复校验两次。

## 2. 一次完整的 JWT 登录流程

```text
【首次登录】
前端提交用户名和密码
        ↓
Controller 接收 /login 请求
        ↓
Service 校验密码（通常查询数据库中的密码哈希）
        ↓
登录成功，后端签发 JWT
        ↓
前端收到 JWT

【后续访问受保护接口】
前端 GET /user/profile
Header: Authorization: Bearer <JWT>
        ↓
Filter（如果项目注册了对应过滤器）
        ↓
Spring MVC / DispatcherServlet
        ↓
Interceptor.preHandle()（如果配置了对应拦截器）
        ↓
JWT 验签、检查有效期等
        ↓
合法 → Controller → Service → Mapper
非法 → 返回 HTTP 401，停止进入 Controller
```

**最重要的理解：**不用每次重新传用户名、密码，而是携带后端签发的 JWT。后端必须验证 JWT，不能仅仅因为收到了 Token 就相信用户身份。

## 3. JWT 详解

### 3.1 JWT 结构

常见的签名 JWT（JWS 紧凑格式）包含三个由点号分隔的部分：

```text
Header.Payload.Signature
```

| 部分 | 含义 | 例子 |
| --- | --- | --- |
| Header | 指定令牌类型、签名算法等 | `{"alg":"HS256","typ":"JWT"}` |
| Payload | 存放声明（claims），如用户 ID、过期时间 | `{"sub":"1001","role":"user","exp":1790000000}` |
| Signature | 使用密钥和算法计算出的签名 | 用于检验令牌是否被篡改 |

JWT 的 Header 和 Payload 通常通过 **Base64URL 编码**，**不是加密**。拿到 JWT 的人通常可以解码查看其中内容。

因此 **不要在 Payload 存放密码、身份证号、银行卡号等敏感数据**。`exp` 一般是 Unix 时间戳（秒）。

### 3.2 Signature 为什么能防篡改？

以 HS256 为例，它的签名计算可以概括为：

```text
Signature = Base64URL(
    HMAC-SHA256(
        Base64URL(Header) + "." + Base64URL(Payload),
        secretKey
    )
)
```

若有人把 `"role":"user"` 改为 `"role":"admin"`，却不知道签名密钥，就无法生成能通过验证的新签名。

**注意：签名保证完整性和来源可验证，不负责隐藏 Payload。** 服务端还应验证算法、有效期，以及业务需要的签发者、受众等声明；有签名不等于一定允许访问所有接口。

### 3.3 JWT 怎样放进 HTTP 请求？

登录成功后，服务端可以返回：

```json
{
  "token": "<JWT 字符串>"
}
```

后续请求通常携带：

```http
GET /user/profile HTTP/1.1
Authorization: Bearer <JWT 字符串>
```

- `Authorization`：HTTP 请求头，通常用于携带认证凭证。
- `Bearer`：表示持有此令牌即可作为认证依据的一种使用方式；它不是 JWT 的一部分。
- `<JWT 字符串>`：真正的 Token。

服务端会提取 `Bearer ` 后面的内容，再进行验证。

### 3.4 JWT 与 Session 的区别

| 比较项 | Session 方案 | 常见的无状态 JWT 方案 |
| --- | --- | --- |
| 客户端携带 | Session ID | 完整 JWT |
| 服务端是否保存登录状态 | 通常保存 Session 数据 | 通常不用为每个令牌保存会话记录 |
| 验证依据 | 用 Session ID 查服务端保存的数据 | 验证 JWT 的签名、有效期等 |
| 退出登录/立即撤销 | 通常删除 Session 即可 | 一般需要短有效期、撤销名单或其他服务端状态机制 |

JWT **可以用于无状态认证**，但不是使用 JWT 就自动拥有“可撤销、可强制退出、完全无状态”等全部特性。

## 4. Filter（过滤器）

### 4.1 整体作用

Filter 来自 Servlet 体系，作用位置通常在请求进入 Spring MVC 的 `DispatcherServlet` 之前。它也能处理响应阶段。

典型用途：通用请求日志、字符编码、跨域处理、请求封装，以及安全过滤等。

### 4.2 关键语法

```java
import jakarta.servlet.Filter;
import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.ServletRequest;
import jakarta.servlet.ServletResponse;
import java.io.IOException;

public class LogFilter implements Filter {
    @Override
    public void doFilter(
            ServletRequest request,
            ServletResponse response,
            FilterChain chain
    ) throws IOException, ServletException {
        System.out.println("请求进入 Filter");

        // 让请求继续执行后面的过滤器或目标资源
        chain.doFilter(request, response);

        System.out.println("请求处理完成，返回 Filter");
    }
}
```

逐行看重点：

- `implements Filter`：实现 Servlet 定义的过滤器接口。
- `doFilter(...)`：过滤器处理请求的核心方法。
- `ServletRequest` / `ServletResponse`：请求与响应对象。
- `FilterChain chain`：过滤器链，负责把请求交给后续处理。
- `chain.doFilter(request, response)`：**继续执行后面的过滤器/Servlet**。

若请求被拒绝，可以**设置状态码并直接返回，不调用** `chain.doFilter(...)`，请求就不会继续进入后续处理链。

> 上例仅展示 Filter 的结构。要让它真正生效，还必须把它注册到 Servlet 容器，例如在 Spring Boot 项目中使用适当的组件注册或 `FilterRegistrationBean`。上例使用 Spring Boot 3 的 `jakarta.servlet` 包。

## 5. Interceptor（拦截器）

### 5.1 整体作用

Interceptor 是 Spring MVC 提供的机制，通常围绕 Controller（准确说是 Spring MVC 选中的 handler）的执行进行处理。常用于：

- 登录认证、接口访问权限检查。
- 获取 Controller 执行前后的信息。
- 记录接口耗时、审计等。

它比 Servlet Filter 更接近具体的 Controller 方法；当 `handler` 为 `HandlerMethod` 时，可以读取目标方法、注解等信息。

### 5.2 关键语法

```java
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.web.servlet.HandlerInterceptor;

public class LoginInterceptor implements HandlerInterceptor {
    @Override
    public boolean preHandle(
            HttpServletRequest request,
            HttpServletResponse response,
            Object handler
    ) throws Exception {
        System.out.println("Controller 执行前");
        return true;
    }
}
```

- `implements HandlerInterceptor`：实现 Spring MVC 的拦截器接口。
- `preHandle(...)`：选定 handler 后、执行 handler 前调用。
- `return true`：允许继续执行。
- `return false`：停止当前 handler 的正常执行；**应由拦截器负责设置合适的 HTTP 响应**，否则前端可能拿不到明确错误信息。

Spring MVC 还提供 `postHandle`（handler 正常返回后、视图渲染前的钩子）与 `afterCompletion`（请求处理完成后的清理钩子）。其中 `postHandle` 不是在所有异常/响应场景都执行。

### 5.3 在 Spring Boot 中注册拦截器

只写一个 `HandlerInterceptor` 类，通常**不会自动拦截请求**。需要配置拦截路径：

```java
import org.springframework.context.annotation.Configuration;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

@Configuration
public class WebConfig implements WebMvcConfigurer {
    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(new LoginInterceptor())
                .addPathPatterns("/**")
                .excludePathPatterns("/login", "/register");
    }
}
```

这段代码的整体作用：**让拦截器处理绝大多数 MVC 请求，但放行登录、注册接口**。

- `@Configuration`：标记 Spring 配置类。
- `WebMvcConfigurer`：用于自定义 Spring MVC 配置。
- `addInterceptors(...)`：登记拦截器。
- `addPathPatterns("/**")`：匹配路径。
- `excludePathPatterns(...)`：排除公开接口。

实际项目中通常会将需要注入依赖的拦截器交给 Spring 管理，而不是简单地直接 `new`。

## 6. Filter 与 Interceptor 对比

| 比较项 | Filter | Interceptor |
| --- | --- | --- |
| 所属体系 | Servlet | Spring MVC |
| 所处位置 | 通常早于 DispatcherServlet | 位于 Spring MVC handler 执行链 |
| 拦截对象 | Servlet 请求/响应 | 经过 Spring MVC 映射的请求与 handler |
| 典型能力 | 通用预处理、包装请求/响应、过滤器链 | 针对 Controller/handler 做细粒度处理 |
| 放行方式 | 调用 `chain.doFilter(...)` | `preHandle()` 返回 `true` |
| 拦截方式 | 不继续调用链，并完成响应 | `preHandle()` 返回 `false`，并完成响应 |

**一句话记忆：Filter 更靠外层；Interceptor 更靠近 Controller。**

示意顺序（简化）：

```text
HTTP 请求
   ↓
Servlet Filter 1 → Filter 2
   ↓
DispatcherServlet
   ↓
HandlerMapping（找到目标 handler）
   ↓
Interceptor.preHandle()
   ↓
Controller / Handler
   ↓
Interceptor.postHandle()（满足条件时）
   ↓
Interceptor.afterCompletion()（满足条件时）
   ↓
沿 Filter 链返回 HTTP 响应
```

## 7. 用 Interceptor 进行 JWT 登录校验（伪代码）

先理解**业务逻辑**，再考虑具体依赖库：

```java
@Override
public boolean preHandle(
        HttpServletRequest request,
        HttpServletResponse response,
        Object handler
) throws Exception {
    String auth = request.getHeader("Authorization");

    if (auth == null || !auth.startsWith("Bearer ")) {
        response.sendError(401, "Missing token");
        return false;
    }

    String token = auth.substring("Bearer ".length());

    // jwtService 是示意性的业务组件：需由自己实现/注入。
    // 应在其中检查签名、算法、exp 等必要条件。
    if (!jwtService.isValid(token)) {
        response.sendError(401, "Invalid token");
        return false;
    }

    return true;
}
```

**注意：这是一段用来学习业务逻辑的伪代码，不是可直接复制运行的完整类**；需要自己提供 `jwtService`、异常处理和组件注册。

几个容易混淆的概念：

- **401 Unauthorized**：未通过身份认证，例如没有 Token、Token 过期、验签失败。
- **403 Forbidden**：已经识别身份，但没有访问资源的权限。
- **认证（Authentication）**：确定“你是谁”。
- **授权（Authorization）**：判断“你能做什么”。

如果希望在 Controller 中使用用户 ID，还需要在验证成功后安全地传递认证结果；例如使用 Spring Security 的安全上下文等机制，而不是重复解析或随意相信请求中的 `userId` 参数。

## 8. 学习时需要特别注意的安全点

1. **JWT 不是加密**：Payload 可解码，不放敏感信息。
2. **必须验签和校验时效**：不能只 Base64URL 解码 Payload 就认为认证通过。
3. **需要 HTTPS**：Bearer Token 泄露后，攻击者可能直接冒用。
4. **认证不等于授权**：`role=admin` 也需要按服务器可信规则判断，不是客户端自己传什么就信什么。
5. **退出登录问题**：已经签发且未过期的无状态 JWT，通常无法只靠前端删除就让所有副本失效；应考虑短有效期、刷新令牌和撤销机制。
6. **不要重复无意义校验**：理解 Filter 和 Interceptor 都能做认证，但项目里最好明确由哪一层负责。
7. **大型实际项目**：通常优先了解 Spring Security 的标准认证/授权机制，而不必长期维护一整套自行编写的安全过滤流程。

## 9. 手写练习建议

按照从简单到复杂的顺序：

1. 写一个 `GET /hello` Controller，确认能访问。
2. 实现并注册一个 Filter，打印进入/返回时的日志，观察执行位置。
3. 实现并注册一个 Interceptor，先返回 `true`，再返回 `false`，观察 Controller 是否执行。
4. 设置 `/login` 不被拦截、`/user/profile` 被拦截。
5. 使用成熟的 JWT 库签发和验证 JWT，配置密钥、有效期。
6. 在拦截器中读取 `Authorization` 头，分别测试缺失、伪造、过期和有效令牌。
7. 再添加 `/admin` 接口，区分 **401（未认证）** 与 **403（已认证但无权限）**。

## 10. 自测问题

- 为什么登录成功后不需要每次继续传账号和密码？
- JWT 的 `Header / Payload / Signature` 分别是什么？
- JWT 的 Payload 为什么不能存密码？
- 修改 Payload 中的角色字段，为什么会导致验签失败？
- `chain.doFilter(request, response)` 做什么？
- `preHandle()` 返回 `true` 与 `false` 的区别是什么？
- Filter 与 Interceptor 各属于哪一个技术体系？
- 为什么写了 Interceptor 类还需要配置注册？
- Token 过期、Token 有效但没有管理员权限，分别返回什么状态码？
- JWT 为什么不天然解决“立即退出登录”？

---

## 最后总结

```text
JWT         = 后端签发的身份凭证格式
Filter      = Servlet 层的请求过滤机制
Interceptor = Spring MVC 层的请求拦截机制

前端携带 JWT
   ↓
请求进入后端
   ↓
统一校验（Filter 或 Interceptor）
   ↓
校验通过才执行受保护的业务 Controller
```

> **核心记忆：JWT 负责证明身份；Filter / Interceptor 提供统一处理请求的执行位置。**
