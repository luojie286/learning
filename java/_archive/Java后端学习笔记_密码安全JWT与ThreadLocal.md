# Java 后端学习笔记：密码安全、JWT、Interceptor 与 ThreadLocal

> 整理范围：本次聊天关于管理员密码泄露、密码存储安全、BCrypt、JWT、Spring MVC Interceptor 和 ThreadLocal 的讨论。  
> 学习目标：理解从登录到接口访问的完整认证链路，能看懂并实现一个基础的 Spring Boot 登录认证方案。

## 1. 先建立整体认识

在电商管理后台中，常见的安全需求包括：

1. 用户的密码即使数据库泄露，也不应直接暴露原文。
2. 登录成功后，客户端需要获得可供后续请求使用的身份凭证。
3. 每次调用受保护接口时，后端都要检查身份凭证。
4. Controller 和 Service 需要知道当前请求对应哪个用户。

可以用以下技术组合解决这些问题：

| 技术 | 主要用途 | 解决的问题 |
|---|---|---|
| BCrypt / Argon2id | 专用密码哈希 | 密码如何安全存入数据库 |
| JWT | 签名身份令牌 | 登录后如何证明身份 |
| Spring MVC `HandlerInterceptor` | 请求前后拦截处理 | 在进入 Controller 前检查 JWT |
| `ThreadLocal` | 每线程独立数据 | 当前请求的业务代码如何取得用户 ID |

**关键区分：密码哈希用于保护长期保存的密码；JWT 是登录后的身份凭证，两者不是一回事。**

## 2. 外部人员为什么可能获取管理员密码？

### 2.1 明文密码存储的风险

假设管理员表存储：

| id | username | password |
|---|---|---|
| 1 | admin | admin123456 |

如果数据库数据泄露，原始密码随之泄露。明文存储不存在“解密”过程，因为根本没有加密。

在**有合法数据库访问权限的测试环境**中，以下 SQL 能查询相关字段：

```sql
SELECT id, username, password
FROM admin
WHERE id = 1;
```

这不意味着互联网上的任何人都能查询：必须首先有合法数据库访问权限。未经授权获取他人账号或数据库内容属于另一回事。

### 2.2 常见风险入口

- **SQL 注入**：程序把用户输入直接拼到 SQL 中，可能改变 SQL 语义；某些情况下导致认证绕过或数据泄露。
- **服务器或应用漏洞**：例如不安全的文件读取、越权访问、软件依赖漏洞，以及数据库凭据泄露。
- **数据库不当暴露**：MySQL 3306 对不可信网络开放，并结合弱密码、不当授权等问题，会扩大风险。
- **钓鱼、密码复用、凭证泄露**：不一定需要攻破数据库，也可能获取账号访问能力。

一个存在风险的示例：

```java
String sql = "SELECT * FROM admin WHERE username = '"
        + username + "' AND password = '"
        + password + "'";
```

**问题在于拼接不可信输入**，并不是 SQL 的 `SELECT` 本身不安全。查询应使用预编译参数绑定：

```java
String sql = "SELECT * FROM admin WHERE username = ?";
PreparedStatement ps = connection.prepareStatement(sql);
ps.setString(1, username);
ResultSet rs = ps.executeQuery();
```

对于采用 BCrypt 的系统，应先按用户名查询密码哈希，再通过 Java 代码验证密码；**不要在 SQL 中直接比较用户输入的明文密码**。

## 3. 密码存储：加密、普通哈希与密码哈希

| 技术 | 是否通常可逆 | 典型用途 |
|---|---|---|
| AES 等加密算法 | 持有正确密钥时可解密 | 需要还原的敏感数据 |
| SHA-256 等通用哈希 | 不可直接逆向还原 | 文件完整性、数据摘要等 |
| BCrypt / Argon2id 等密码哈希 | 不可直接逆向还原，设计上增加猜测成本 | 存储登录密码 |

**用户密码首选专用密码哈希，而不是可逆加密。**

注意：`Base64` 仅是编码，不是加密；直接使用 MD5 或 SHA-256 存密码不合适，因为计算过快，容易受到离线猜测攻击。

### 3.1 为什么密码哈希不能直接解密，仍能验证登录？

注册时：

```text
用户输入密码 → BCrypt（自动生成盐）→ 存储哈希
```

登录时：

```text
用户输入密码 + 数据库存储的哈希
                    ↓
                matches()
                    ↓
                true / false
```

BCrypt 的哈希字符串包含验证所需的盐和成本参数，因此可以验证输入，而不需要恢复原密码。同样的密码在不同次 `encode()` 时通常产生不同的哈希。

### 3.2 Spring Boot 中使用 BCrypt

如果项目尚未引入相关依赖，可在 Maven 中添加：

```xml
<dependency>
    <groupId>org.springframework.security</groupId>
    <artifactId>spring-security-crypto</artifactId>
</dependency>
```

在由 Spring Boot 管理依赖版本的项目中，版本号通常可省略；普通 Maven 项目则需自行指定兼容版本。

```java
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;

BCryptPasswordEncoder encoder = new BCryptPasswordEncoder();

String rawPassword = "admin123456";
String passwordHash = encoder.encode(rawPassword);  // 注册/改密

boolean ok = encoder.matches(rawPassword, passwordHash); // 登录验证
```

- `encode(rawPassword)`：生成哈希以保存。
- `matches(rawPassword, storedHash)`：验证输入的密码是否与已存储哈希匹配。
- **不要**登录时重新调用 `encode()` 然后直接用字符串相等比较：盐是随机的，新生成的哈希很可能不同。

### 3.3 数据库表设计

```sql
CREATE TABLE admin (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL
);
```

注册保存 `password_hash`；登录先按 `username` 查询记录，再调用 `matches()`。

**安全边界**：密码哈希不意味着“永远无法猜出密码”。如果数据库泄露，攻击者仍可能对弱密码进行离线猜测，所以还需要强密码、多因素认证、登录限流与数据库最小权限。

## 4. JWT：登录后的身份凭证

JWT 全称 **JSON Web Token**。在常见的签名 JWT 场景中，它是由三个 Base64URL 编码部分构成的字符串：

```text
Header.Payload.Signature
```

| 部分 | 内容 | 作用 |
|---|---|---|
| Header | 令牌类型、签名算法等 | 告诉验证方如何解析与验证 |
| Payload | 例如 `userId`、`role`、`exp` | 传递身份与过期信息 |
| Signature | 对前两部分的签名 | 检测内容是否被篡改 |

示意载荷：

```json
{
  "userId": 1001,
  "role": "admin",
  "exp": 1791500000
}
```

- `userId`：用户标识。
- `role`：角色声明。
- `exp`：到期时间，通常是 Unix 秒级时间戳。

**JWT 的 Payload 默认不是加密内容，不能在其中放原始密码或其他不应暴露的数据。**

典型请求头：

```http
GET /admin/profile HTTP/1.1
Authorization: Bearer eyJhbGciOi...
```

`Bearer ` 后面是 JWT。服务器不能仅仅“解码” Payload 就信任其中的用户 ID，**必须验证签名、有效期以及其他适用的令牌约束**；令牌权限也应接受服务端的授权检查。

## 5. Interceptor：Spring MVC 拦截器

### 5.1 它解决什么问题？

每个 Controller 都手动解析 JWT 会有大量重复代码，因此可以使用 Spring MVC 的 `HandlerInterceptor` 在请求进入 Controller 之前统一验证。

三个常见方法：

```java
public class LoginInterceptor implements HandlerInterceptor {

    @Override
    public boolean preHandle(HttpServletRequest request,
                             HttpServletResponse response,
                             Object handler) {
        return true;
    }

    @Override
    public void postHandle(HttpServletRequest request,
                           HttpServletResponse response,
                           Object handler,
                           ModelAndView modelAndView) {
    }

    @Override
    public void afterCompletion(HttpServletRequest request,
                                HttpServletResponse response,
                                Object handler,
                                Exception ex) {
    }
}
```

- `preHandle()`：Controller 之前执行；`true` 放行，`false` 中断后续处理。
- `postHandle()`：Controller 成功处理后、视图渲染前执行；REST API 通常不需要依赖这个方法处理认证。
- `afterCompletion()`：对应拦截器成功通过 `preHandle()` 后，在同步请求完成时执行，适合清理资源。

### 5.2 如何注册拦截器？

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
                .excludePathPatterns("/admin/login");
    }
}
```

- `@Configuration`：标识 Spring 配置类。
- `WebMvcConfigurer`：提供 MVC 配置扩展点。
- `addInterceptors()`：添加拦截器。
- `addPathPatterns("/**")`：匹配全部路径。
- `excludePathPatterns("/admin/login")`：排除登录接口，避免尚未登录就被拦截。

该示例假设 `LoginInterceptor` 已注册为 Spring Bean。实际项目还要安排健康检查、公开接口等路径，并区分用户端与管理员端权限。

## 6. ThreadLocal：当前线程的局部数据

### 6.1 为什么要用它？

在典型的同步 Servlet 请求中，Controller、Service 等代码通常运行在处理该请求的同一条线程上。拦截器先解析 JWT 中的 `userId`，后续业务代码就能通过 `ThreadLocal.get()` 取到此用户 ID，而不需要每层方法都增加 `userId` 参数。

```text
工作线程 A：ThreadLocal → userId = 1001
工作线程 B：ThreadLocal → userId = 1002
```

虽然两个线程访问的是同一个 `ThreadLocal` 对象，它们读到的值相互独立。

### 6.2 常见封装

```java
public class BaseContext {
    private static final ThreadLocal<Long> threadLocal = new ThreadLocal<>();

    public static void setCurrentId(Long id) {
        threadLocal.set(id);
    }

    public static Long getCurrentId() {
        return threadLocal.get();
    }

    public static void removeCurrentId() {
        threadLocal.remove();
    }
}
```

- `ThreadLocal<Long>`：当前线程对应一个 `Long` 值。
- `static`：所有代码共享同一 `ThreadLocal` 引用，但**不共享每个线程内部的数据值**。
- `set()`：设置当前线程的数据。
- `get()`：读取当前线程的数据。
- `remove()`：删除当前线程的数据。

### 6.3 为什么 `remove()` 非常重要？

Tomcat 通常使用线程池复用工作线程。如果处理用户 A 的请求后不清理身份数据，之后复用相同线程处理其他请求时，有可能读到残留的用户 ID。

**必须保证清理时机和异常路径被正确处理。** `ThreadLocal` 不能天然跨线程传递；`@Async`、线程池任务、异步 Servlet 分发等场景需要另行设计上下文传递和清理，不能假设新线程会自动读到原用户 ID。

## 7. 三者如何协作：完整请求链路

```text
（首次登录）
浏览器输入用户名、密码
       ↓
POST /admin/login
       ↓
Spring Boot 按用户名查询管理员
       ↓
BCrypt.matches(明文输入, 数据库存储哈希)
       ↓
验证成功 → 签发 JWT → 返回浏览器

（后续调用接口）
浏览器携带 Authorization: Bearer <JWT>
       ↓
Tomcat 接收 HTTP 请求
       ↓
Spring MVC 匹配请求并调用 Interceptor.preHandle()
       ↓
验证 JWT 签名 / 时间等条件
       ↓
解析 userId → ThreadLocal.set(userId)
       ↓
Controller → Service → 数据访问层
       ↓
业务代码可通过 ThreadLocal.get() 取得 userId
       ↓
返回 HTTP 响应
       ↓
请求完成时 ThreadLocal.remove()
```

### 7.1 拦截器代码示意

以下 `JwtUtil.parseAndVerify(token)` 是**假设你已自行实现的工具方法**，不是 Java 或 Spring 的内置 API。它应完成安全校验并返回用户 ID。

```java
@Component
public class LoginInterceptor implements HandlerInterceptor {

    @Override
    public boolean preHandle(HttpServletRequest request,
                             HttpServletResponse response,
                             Object handler) {
        // 避免当前线程意外保留先前请求的数据
        BaseContext.removeCurrentId();

        String authorization = request.getHeader("Authorization");
        if (authorization == null ||
                !authorization.startsWith("Bearer ")) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return false;
        }

        String token = authorization.substring(7);
        try {
            Long userId = JwtUtil.parseAndVerify(token);
            if (userId == null) {
                response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
                return false;
            }
            BaseContext.setCurrentId(userId);
            return true;
        } catch (JwtException | IllegalArgumentException e) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return false;
        }
    }

    @Override
    public void afterCompletion(HttpServletRequest request,
                                HttpServletResponse response,
                                Object handler,
                                Exception ex) {
        BaseContext.removeCurrentId();
    }
}
```

**说明**：这是为了展示三种技术配合关系的教学示例，不是能直接复制运行的完整生产方案。`JwtException` 由所选 JWT 库提供或由项目自行定义；实际系统还需考虑权限授权、认证失败响应格式、异步请求、令牌撤销、密钥管理等。

Controller：

```java
@RestController
@RequestMapping("/user")
public class UserController {

    @GetMapping("/info")
    public String getInfo() {
        Long userId = BaseContext.getCurrentId();
        return "当前用户ID：" + userId;
    }
}
```

`getInfo()` 不需要自行解析 JWT：在同步请求中，前面的拦截器已经校验并设置当前用户 ID。

## 8. 必须掌握的安全边界与常见误区

1. **SQL 参数化与密码哈希解决不同问题**：前者防 SQL 注入；后者降低数据库泄露导致的密码暴露风险。两者都需要。
2. **密码哈希 ≠ 加密**：BCrypt 不存在一个用于恢复原始密码的“解密 API”。
3. **JWT 编码 ≠ JWT 加密**：载荷通常可被读取；只有正确验证签名后才可信任声明。
4. **JWT 有效 ≠ 有权访问**：管理员权限、资源归属仍要进行授权验证。
5. **拦截器 ≠ 唯一安全方案**：复杂生产系统往往使用 Spring Security 的安全过滤链统一处理认证和授权；Spring MVC Interceptor 更适合学习或轻量场景。
6. **ThreadLocal ≠ 全局共享 Map**：同一个对象在不同线程保存独立的数据。
7. **ThreadLocal 不自动跨线程传播**：使用异步任务或线程池时必须额外处理。
8. **线程池复用要求清理**：同步场景可在请求完成回调清理；还需正确设计前置拒绝、异常、异步等路径。
9. **管理员密码保护不能只靠 BCrypt**：HTTPS、强密码、多因素认证、最小权限、登录限流、监控与审计同样重要。

## 9. 建议的手写练习

**练习 A：密码存储**

- 创建 `admin(id, username, password_hash)` 表。
- 用 `BCryptPasswordEncoder.encode()` 为测试管理员生成哈希。
- 用 `matches()` 检验正确与错误密码。
- 验证同一个密码重复编码时产生的哈希往往不同，而两者都能验证成功。

**练习 B：基础认证接口**

- 实现 `POST /admin/login`。
- 登录成功签发带 `userId`、过期时间等声明的 JWT。
- 实现 `GET /admin/profile` 并要求携带令牌。
- 测试缺失、错误、过期令牌分别返回 401。

**练习 C：ThreadLocal**

- 让拦截器校验 JWT 后 `setCurrentId()`。
- 让 Controller 和 Service 分别调用 `getCurrentId()`。
- 验证请求结束后会清理上下文。
- 思考：为什么新建线程不会自动读取到父线程的 ThreadLocal 值？

## 10. 自测问题（建议脱离笔记口述）

1. 明文密码、AES 加密、SHA-256、BCrypt 的区别是什么？
2. BCrypt 无法直接解密，为什么还能验证登录？
3. 为什么使用 SQL 预编译参数不能替代密码哈希？
4. JWT 的三段分别是什么？Payload 是否保密？
5. 用户登录成功后，后续请求如何将 JWT 发给后端？
6. `preHandle()` 返回 `true` 与 `false` 分别代表什么？
7. `ThreadLocal` 的 `static` 字段会导致不同线程共享同一个用户 ID 吗？
8. 为什么必须对线程池中的 `ThreadLocal` 做清理？
9. JWT 有效是否意味着可以查看任意用户的订单？为什么？
10. `@Async` 中使用普通 `ThreadLocal` 会出现什么问题？

## 11. 总结与自检

**一句话总结整个认证流程**：

> 注册时用 BCrypt 安全存储密码；登录时验证密码并签发 JWT；后续请求由 Interceptor 校验 JWT，把用户 ID 写入 ThreadLocal；Controller/Service 在当前同步线程内读取用户 ID；请求完成后清理 ThreadLocal。

**自检结论**：

- 已区分“获得数据库密码”与“未经授权入侵数据库”两个问题。
- 已区分可逆加密、通用哈希、专用密码哈希与 JWT 的用途。
- 已涵盖 Spring MVC 拦截器常见方法、注册方式及 ThreadLocal 的读写和清理。
- 代码均围绕教学流程展开；JWT 工具类为占位接口，并未误称为框架内置实现。
- 明确指出同步/异步边界与生产环境的安全限制。

