# Java 对象引用、Spring IoC 与 JWT 拦截器：核对版学习笔记

> 面向 Spring 零基础学习者。依据本次聊天逐条校核；示例默认 Java 17+、Spring Boot 3+（Servlet 包名为 `jakarta.servlet`）。
>
> 核对时间：2026-10-23。本文区分 **正确概念、旧回答需要修正的地方、实际编程示例**。

## 0. 先看核对结论

本次聊天的学习主线总体正确：Java 引用 → 接口多态 → Spring 创建和注入 Bean → Bean 共享 → JWT 拦截器。但有一些为了简化而造成的误解。

| 聊天中的表述 | 核对结果 | 准确说法 |
|---|---|---|
| “正常创建对象要用 `new`” | 基本正确，不完整 | `new` 是普通 Java 代码最常见的实例化方式；反射、克隆、反序列化等也可获得新实例。 |
| “`Student s = null` 初始化对象” | 不准确 | 初始化的是 **引用变量**，没有创建 `Student` 对象。 |
| “`Student s;` 指向 ? ” | 易误导 | 局部变量没有默认值，赋值前不可读取；成员引用字段默认值是 `null`。 |
| “Spring 主要靠反射扫描注解、创建所有对象” | 过度简化 | Spring 可扫描类元数据，实例化 Bean 可用反射、工厂方法、Supplier 等；并非每一步都依赖反射。 |
| “DI 就是反射修改 private 字段” | 不完整 | 构造器注入靠调用构造器传参；字段注入可能使用反射；setter 注入调用 setter。 |
| “Spring AOP 一定通过反射执行真实方法” | 过度绝对 | Spring AOP 核心是 **代理和方法拦截**，可能使用 JDK 动态代理或类代理；目标调用不能全部概括成反射调用。 |
| “`PasswordEncoder` 是接口” | 正确 | Spring Security 的 `org.springframework.security.crypto.password.PasswordEncoder` 是接口；`BCryptPasswordEncoder` 是其实现类之一。 |
| “有 `PasswordEncoder` 就会自动得到 BCrypt” | 条件缺失 | 必须存在可注入的 `PasswordEncoder` Bean（来自手动配置或自动配置等），实际实现依配置而定。 |
| “调用 `encode()` 相当于 `new BCryptPasswordEncoder().encode()`” | 容易误导 | 通常调用的是 **已注入并存在的对象** 的 `encode()`，不在每次调用时 `new`。 |
| “Spring 单例：一个类整个程序只有一个对象” | 不准确 | **每个 IoC 容器中，每个 singleton Bean 定义对应一个共享实例**；同类可有多个 Bean，也可手动 `new`。 |
| “构造器注入时对象不能为空” | 不准确 | `final` 保证字段最终完成赋值，不保证非空；手动 `new UserServiceImpl(null, null)` 在 Java 层面合法。 |
| “静态工具类无法替换实现” | 过度绝对 | 可以替换，但通常比接口 + DI 更难做到灵活替换与单元测试。 |
| “`return false` 就自动返回 401” | 不准确 | 它只是中断 Spring MVC 处理链；需自己设置 401 状态码/响应体，或使用合适的异常处理。 |
| “JWT 只要解析出来就是合法” | 错误（应避免） | 必须验证签名、可信算法、过期等；解码 Payload 不等于校验。 |
| “MVC 拦截器适合作为完整认证安全层” | 不推荐 | 适合学习请求拦截原理；生产认证优先考虑 Spring Security 的过滤器链。 |

---

## 1. 对象、引用变量和 `new`：三个概念别混

### 1.1 类、对象、引用

```java
Student s1 = new Student();  // 创建对象，将其引用赋给 s1
Student s2 = s1;             // 只复制引用，不创建第二个对象
s1 = null;                    // s1 不再指向原对象；s2 仍然指向它
```

可以想成：

```text
s1 ─── null
s2 ───► Student 对象 A
```

`Student s = null;` 声明并初始化的是变量 `s`，并 **未创建** Student 对象。`s` 若仍为 null，调用 `s.toString()` 会抛 `NullPointerException`。

### 1.2 局部变量与成员字段的默认值不同

```java
void demo() {
    Student x;
    // System.out.println(x); // 编译错误：局部变量可能尚未初始化
}

class Holder {
    Student x;  // 对象成员字段默认是 null
}
```

### 1.3 创建对象不一定直接写 `new Xxx()`

```java
Student s = Student.class.getDeclaredConstructor().newInstance();
```

这是反射创建实例。其他路线如 `clone()`、部分反序列化机制；细节因机制不同而异，不能说“任何创建方式都会调用构造器”。注意 `new Student[10]` 创建的是 **数组对象**，其中 10 个引用元素默认都是 null，而不是 10 个 Student 实例。

### 1.4 `final` 限制的是引用变量重新赋值

```java
private final PasswordEncoder encoder;
```

成员 `encoder` 必须在构造器或字段初始化阶段被赋值一次；以后不能让这个字段改指向其他对象。但所引用对象自身不必不可变。`final` 也 **不自动防止赋值 null**。

---

## 2. 接口和实现类：`PasswordEncoder` 到底是什么

`PasswordEncoder` 是 Spring Security Crypto 中的 **接口类型**；核心操作是：

```java
String encode(CharSequence rawPassword);
boolean matches(CharSequence rawPassword, String encodedPassword);
```

常见实现是 `BCryptPasswordEncoder`。示意：

```java
PasswordEncoder encoder = new BCryptPasswordEncoder();
String hash = encoder.encode("123456");
boolean ok = encoder.matches("123456", hash);
```

这里叫 **向上转型**：接口类型的引用指向实现类实例。并不是“父类向下兼容”。表达式允许调用哪些方法由 **编译期引用类型** 决定，而重写方法的实际执行由 **运行时对象类型** 决定（动态分派）。

BCrypt 更准确地称为 **密码哈希**，不是可逆的“加密”；验证时用 `matches` 比较密码与已有哈希，而非解密哈希。重复 `encode("123456")` 通常会得到不同结果（含随机盐），因此不可直接比较两次哈希字符串。

---

## 3. Spring、Java 反射、IoC、DI 的关系

### 3.1 先把名词分开

- **反射**：Java 运行时查看类信息、获取构造器/字段/方法并操作的能力。
- **IoC（控制反转）**：应用不再主要由业务代码主动管理依赖对象的创建和装配；由容器统筹。
- **DI（依赖注入）**：容器将所需依赖交给对象，例如通过构造器传入。
- **Bean**：由 Spring IoC 容器管理的对象。

反射是 Spring 可以使用的底层技术，**不是 IoC 的定义**。Spring 通过组件扫描、配置类、工厂方法等收集 Bean 定义，然后创建和装配对象。

### 3.2 `@Bean` 和 `@Service` 的区别（入门理解）

```java
@Configuration
public class SecurityConfig {
    @Bean
    public PasswordEncoder passwordEncoder() {
        return new BCryptPasswordEncoder();
    }
}
```

`@Bean` 告诉 Spring：通过调用该方法得到一个要管理的 Bean。这里真正 `new BCryptPasswordEncoder()` 的代码 **写在工厂方法里**，不是 `encode()` 时执行。

```java
@Service
public class UserServiceImpl {
    // ...
}
```

`@Service` 是一种组件标记；在相应的组件扫描范围内可被注册为 Bean。**仅给普通类中的任意方法加 `@Bean`，并不能保证该方法被 Spring 发现**；所在类需要以合适方式参与 Spring Bean 定义注册。`@Configuration` 不是唯一可能方式。

### 3.3 构造器注入：此时传进来的到底是什么

```java
@Service
public class UserServiceImpl {
    private final UserMapper userMapper;
    private final PasswordEncoder passwordEncoder;

    public UserServiceImpl(UserMapper userMapper,
                           PasswordEncoder passwordEncoder) {
        this.userMapper = userMapper;
        this.passwordEncoder = passwordEncoder;
    }
}
```

流程示意：

```text
1. Spring 容器有 PasswordEncoder Bean（例如 BCryptPasswordEncoder 实例 A）
2. MyBatis 与 Spring 集成后注册 UserMapper 代理 Bean（需要正确配置扫描）
3. Spring 构造 UserServiceImpl，向构造器传入这些 Bean 的引用
4. this.passwordEncoder = passwordEncoder
5. 调用 this.passwordEncoder.encode(...) → 对已注入对象 A 调用方法
```

并非“先存一个将来会自动 new 的占位引用”。**常见 singleton + 非懒加载场景中，依赖在服务对象构造时已准备好。** 但 Spring 也支持懒加载、代理、prototype 等模式，不应认为全部 Bean 都会在启动瞬间直接创建。

重要细节：只有**容器里存在合适 Bean** 才能注入。一个接口可能有多个实现 Bean，遇到歧义可能需要 `@Qualifier` 或 `@Primary`；一个都没有则通常启动报依赖无法满足。构造器只有一个时，Spring 通常无需写 `@Autowired`。

### 3.4 Mapper 接口为什么也能注入

MyBatis 的 Mapper 通常是接口，不能 `new UserMapper()`；MyBatis-Spring 可为 Mapper 创建 **代理对象** 并将其注册为 Bean，因此 Service 收到的是真正可调用的代理引用，而不是接口本身。

---

## 4. Spring 单例 Bean：共享的究竟是什么

假设容器只定义了一个 `PasswordEncoder` Bean，且使用默认 singleton 作用域：

```text
                          Spring IoC 容器
                      PasswordEncoder Bean A
                              ▲
                           同一实例
                         ↗         ↖
                  UserService     AdminService
                  的引用字段       的引用字段
```

两个服务 **各自有一个字段**，字段中放的是 **同一个 Bean 实例的引用**；不是每个服务各创建一个 BCrypt 对象。

准确限定是：**每个容器、每个 Bean 定义，一份 singleton 实例**。这 **不等于** 一个 Java 类在 JVM 中只能创建一个对象：可以手动 `new`，也可以给同一个类声明多个不同 Bean 定义。

默认单例 Bean 通常适合无请求级可变状态的 Service、Encoder 等；共享一个 Bean 并不自动保证任意可变字段的线程安全。

更多作用域：`prototype`（多实例），以及 Web 环境下的 `request`、`session` 等。

---

## 5. 为什么是接口 + DI，而不是全部写静态工具类

静态工具方法没有绝对错误，例如：

```java
Math.max(1, 2);
```

但对于希望通过配置选择算法、单元测试时替换假实现、对接其他业务依赖的服务，通常更适合：

```java
private final PasswordEncoder passwordEncoder;
```

好处：

1. **切换实现**：业务类只依赖接口，不直接依赖 `BCryptPasswordEncoder`。
2. **可测性**：构造器参数可以传 Mock/Fake，不需要真的计算昂贵密码哈希。
3. **生命周期和配置统一管理**：Spring 管理 Bean，默认共享。

这不意味着“静态方法不能更换实现”，也不意味着“每个工具一定要写成 Bean”。小型、纯函数、无可替换需求的工具完全可以保持静态。

---

## 6. Spring MVC 请求经过哪里

简化后的典型 Servlet Web 请求路径：

```text
客户端 HTTP 请求
    ↓
Servlet 容器（例如嵌入式 Tomcat）
    ↓
Servlet Filter 过滤器链
    ↓
DispatcherServlet（Spring MVC 前端控制器）
    ↓
HandlerMapping 选择处理器、关联的 HandlerInterceptor
    ↓
Interceptor.preHandle(request, response, handler)
    ├─ true  → Controller 方法 → 响应
    └─ false → 停止后续 Handler 执行（需妥善设置响应）
```

`HandlerInterceptor` 是 Spring MVC 层面的拦截扩展；Servlet `Filter` 则位于 Servlet 过滤器链，通常更早介入。它们并非同一个接口或同一层级。

### 6.1 Spring MVC 拦截器的核心方法

- `preHandle`：Controller/handler 调用前运行，返回 `true` 继续、`false` 中断。
- `postHandle`：handler 正常执行后回调；不能认为这里总能更改已经写出的响应。
- `afterCompletion`：请求处理完成后回调，常用于清理资源；调用规则依前面的执行阶段而异。

### 6.2 注册一个简单拦截器（先不写 JWT）

```java
@Component
public class DemoInterceptor implements HandlerInterceptor {
    @Override
    public boolean preHandle(HttpServletRequest request,
                             HttpServletResponse response,
                             Object handler) throws Exception {
        System.out.println("访问路径：" + request.getRequestURI());
        return true;
    }
}
```

```java
@Configuration
public class WebConfig implements WebMvcConfigurer {
    private final DemoInterceptor demoInterceptor;

    public WebConfig(DemoInterceptor demoInterceptor) {
        this.demoInterceptor = demoInterceptor;
    }

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(demoInterceptor)
                .addPathPatterns("/**")
                .excludePathPatterns("/user/login");
    }
}
```

这里假定 `DemoInterceptor` 所在包在组件扫描范围内。以上代码用于理解 MVC 机制，尚未做任何认证。

---

## 7. JWT 是什么：与拦截器是两回事

JWT（JSON Web Token）是 Token 格式，不是“拦截器”本身。典型紧凑 JWS 由三部分组成：

```text
Base64Url(Header).Base64Url(Payload).Base64Url(Signature)
```

- Header：令牌类型/签名算法等信息。
- Payload：声明（claims），例如 `sub`（主体）、`exp`（到期时间）。Payload 通常可被解码查看，**不要放密码、敏感明文**。
- Signature：用于检测内容是否被修改并验证来源，需使用受信任的密钥及 **服务端限定的签名算法** 校验。

**解码 ≠ 验证。** 验证时除了签名及 `exp`，还要按系统要求检查 `iss`、`aud`、`nbf` 等约束；不要盲信令牌自报的算法。

登录流程示意：

```text
用户提交账号密码
    → 服务端验证密码（PasswordEncoder.matches）
    → 登录成功签发 JWT
    → 客户端保存并在后续请求携带 Token
    → 服务端校验 JWT，识别用户身份
```

常见请求头：

```http
Authorization: Bearer <JWT字符串>
```

`Bearer ` 是认证方案前缀，不能把整个 Header 值无处理地当作 JWT 字符串提交给解析器。

JWT 不能自动解决令牌撤销、登出、续期、细粒度权限控制；这些需要单独设计。客户端如何保存 Token 也涉及 XSS、CSRF 和凭证泄漏防护。

---

## 8. JWT 拦截器：最小学习骨架

建议 **先让拦截器运行，再接入可信 JWT 库**，而不是自己实现签名密码学。

```java
@Component
public class JwtInterceptor implements HandlerInterceptor {
    // JwtVerifier 是示意接口：由你使用可靠 JWT 库实现
    private final JwtVerifier jwtVerifier;

    public JwtInterceptor(JwtVerifier jwtVerifier) {
        this.jwtVerifier = jwtVerifier;
    }

    @Override
    public boolean preHandle(HttpServletRequest request,
                             HttpServletResponse response,
                             Object handler) throws Exception {
        String authorization = request.getHeader("Authorization");

        if (authorization == null ||
                !authorization.startsWith("Bearer ") ||
                authorization.substring(7).isBlank()) {
            response.sendError(HttpServletResponse.SC_UNAUTHORIZED);
            return false;
        }

        String jwt = authorization.substring(7);
        try {
            jwtVerifier.verify(jwt); // 必须校验签名、过期与应用要求的 claims
            return true;
        } catch (InvalidTokenException ex) {
            response.sendError(HttpServletResponse.SC_UNAUTHORIZED);
            return false;
        }
    }
}
```

**注意**：上面 `JwtVerifier`、`InvalidTokenException` 是为说明职责而写的自定义占位类型，**不能单独复制就编译通过**；真实项目须定义它们并使用经过维护的 JWT 库完成验证。不要在异常处理时把所有服务器内部错误伪装成 401。

`sendError(401)` 使“认证失败”成为真正的 HTTP 响应；`return false` 单独使用并不会自动设置状态码。

验证成功识别了“是谁”不等于有权限访问任何资源：还需对用户与资源的归属、角色和权限作授权校验。

### 8.1 生产实践边界

Spring 官方提醒：MVC Interceptor **不适合作为完整的安全层**，可能与 Controller 映射匹配产生不一致。生产项目更推荐 Spring Security 的 `SecurityFilterChain`，或者在 Servlet Filter 层实现统一认证保障。

如果系统已启用 Spring Security，也不能因为注册了 MVC 拦截器就假设所有请求均安全受控。

---

## 9. 学习路径：每一关只解决一个问题

1. **Java 引用**：能解释 `new`、`null`、`s2 = s1` 和 `final` 的差异。
2. **接口多态**：能解释 `PasswordEncoder p = new BCryptPasswordEncoder()`。
3. **构造器注入**：不使用 Spring，手写 `new UserServiceImpl(mapper, encoder)` 并理解引用传递。
4. **IoC/Bean**：写 `@Configuration` + `@Bean`，用两个服务注入同一 `PasswordEncoder`，理解 singleton。
5. **Spring MVC**：实现 `HandlerInterceptor.preHandle()` 并配置 `/api/**`。
6. **HTTP**：会读取 `Authorization`，返回 `401`；能区分 `401`（未有效认证）与 `403`（已认证但无权限）。
7. **JWT**：区分签发与验证，验证签名、有效期、发行者/受众等；了解撤销策略。
8. **生产升级**：了解 Servlet Filter / Spring Security 和授权策略。

### 自测（先独立回答）

1. `Student s = null` 有没有创建对象？
2. `PasswordEncoder p = new BCryptPasswordEncoder()` 调用的是谁的 `encode()`？
3. 在构造器 `this.passwordEncoder = passwordEncoder` 里，等号两边分别是什么？
4. 同一 singleton Bean 注入两个 Service，共创建几个该 Bean 实例？限定条件是什么？
5. `@Bean` 方法里 `new BCryptPasswordEncoder()` 和 `encoder.encode()` 分别在什么阶段执行？
6. 只把 JWT Payload Base64 解码，能判断用户身份真实吗？
7. `preHandle()` 返回 `false`，浏览器是否自动得到 401？
8. `HandlerInterceptor` 和 Servlet Filter 在请求链中谁先参与？

<details>
<summary>参考答案</summary>

1. 没有，只赋值引用变量为 null。
2. 运行时由指向的 `BCryptPasswordEncoder` 对象执行实现方法。
3. 左边是当前 `UserServiceImpl` 实例的字段，右边是构造器参数引用。
4. 同一容器、同一 Bean 定义、默认 singleton 时只有一个共享实例。
5. 前者通常在容器创建该 Bean 时；后者在业务代码调用时。具体创建时点受懒加载等设置影响。
6. 不能，必须校验签名和相关声明。
7. 不会，需要设置正确响应状态/响应体。
8. Servlet Filter 链先于 MVC 内部的 HandlerInterceptor。

</details>

---

## 10. 官方参考资料（用于核对）

- Spring Framework — Bean scopes: https://docs.spring.io/spring-framework/reference/core/beans/factory-scopes.html
- Spring Framework — MVC Interceptors: https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-config/interceptors.html
- Spring Framework — HandlerInterceptor lifecycle: https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-servlet/handlermapping-interceptor.html
- Spring Security — Password Storage: https://docs.spring.io/spring-security/reference/features/authentication/password-storage.html
- RFC 7519 — JSON Web Token (JWT): https://www.rfc-editor.org/rfc/rfc7519
- RFC 8725 — JSON Web Token Best Current Practices: https://www.rfc-editor.org/rfc/rfc8725

> 笔记所述 Java/Spring/JWT 基础是对聊天内容的 **纠错与教学重组**；其中生产安全实践和 JWT 规范补充来自官方资料，不是聊天原本已经讲清的内容。
