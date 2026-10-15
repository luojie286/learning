# 34 · 密码安全与 JWT 鉴权

> 一句话概括：注册用 BCrypt 存密码哈希，登录验密码后签发 JWT，之后每个请求由拦截器校验 JWT 并把 userId 放进 ThreadLocal——这是 Java 后端最经典的认证链路。

## 一、密码存储：三类技术别混淆

| 技术 | 可逆性 | 用途 |
|---|---|---|
| AES 等加密 | 持钥可解密 | 需要还原的敏感数据 |
| SHA-256 等通用哈希 | 不可逆向 | 文件完整性、摘要 |
| **BCrypt / Argon2id 密码哈希** | 不可逆向 + 故意算得慢 | 存登录密码 |

`Base64` 只是编码不是加密；MD5、裸 SHA-256 计算太快，易被离线猜测——**都不能存密码**。密码哈希没"解密 API"，验证靠 `matches`：哈希串里自带盐和成本参数，同一密码每次 `encode()` 结果不同，所以**绝不能重新 encode 再 equals**，要用 `matches(输入, 已存哈希)`。

```java
BCryptPasswordEncoder encoder = new BCryptPasswordEncoder();
String hash = encoder.encode(raw);            // 注册/改密时存库
boolean ok = encoder.matches(raw, storedHash); // 登录验证
```

表设计：`admin(id, username UNIQUE, password_hash)`。登录先按用户名查出哈希，再 Java 侧 `matches`——**不要在 SQL 里比较明文密码**。注意：哈希 ≠ 绝对安全（弱密码仍可被离线猜），还需 HTTPS、强密码、MFA、登录限流、数据库最小权限。SQL 注入防护和密码哈希解决的是不同问题，两者都要。

## 二、JWT：登录后的身份凭证

`Header.Payload.Signature` 三段，Header/Payload 是 **Base64URL 编码不是加密**，谁都能解码看——**Payload 绝不放密码等敏感数据**。签名（如 HS256 = HMAC-SHA256）防篡改：改了内容没有密钥就过不了验签。

```http
Authorization: Bearer <token>
```

后端必须验证**签名、有效期 exp、算法、签发者/受众**等，"能解码出 userId"不等于认证通过。JWT 有效 ≠ 有权访问——授权（Authorization）还要另行判断。对比 Session：JWT 无状态（服务端不存会话），代价是**无法天然"立即退出"**——要短有效期 + 刷新/撤销机制。401 = 未认证，403 = 已认证但无权限。

## 三、Filter 与 Interceptor

| | Filter | Interceptor |
|---|---|---|
| 体系 | Servlet | Spring MVC |
| 位置 | DispatcherServlet 之前（更外层） | 围绕 Handler（更靠近 Controller） |
| 放行 | `chain.doFilter(...)` | `preHandle()` 返回 true |
| 拦截 | 不调 chain 并自行完成响应 | 返回 false 并自行完成响应 |

写了拦截器类**还要注册**才生效：`WebConfig implements WebMvcConfigurer` 里 `addPathPatterns("/**").excludePathPatterns("/login")`（放行登录注册）。项目里明确一层负责校验即可，别两边重复。

## 四、ThreadLocal 传递当前用户

```java
public class BaseContext {
    private static final ThreadLocal<Long> tl = new ThreadLocal<>();
    public static void setCurrentId(Long id) { tl.set(id); }
    public static Long getCurrentId() { return tl.get(); }
    public static void removeCurrentId() { tl.remove(); }
}
```

`static` 共享的是 ThreadLocal **对象**，不同线程读到的**值**相互独立。两个必须记住的坑：

1. **线程池复用**：Tomcat 工作线程是池化的，请求结束不 `remove()`，下一个请求可能读到上一个用户的 ID——`afterCompletion` 里必须清理，preHandle 开头也先清一次。
2. **不跨线程**：`@Async`/线程池任务/异步 Servlet 不会自动继承，要另行设计上下文传递。

## 五、ThreadLocal 深挖（2026-10-14 增补）

### 5.1 标准工具类写法

真实项目（如苍穹外卖的 `BaseContext`）长这样：

```java
public final class BaseContext {
    private static final ThreadLocal<Long> CURRENT_ID = new ThreadLocal<>();
    private BaseContext() {}          // 私有构造器：禁止实例化
    public static void setCurrentId(Long id) { CURRENT_ID.set(id); }
    public static Long getCurrentId() { return CURRENT_ID.get(); }
    public static void removeCurrentId() { CURRENT_ID.remove(); }
}
```

三个修饰符各自的作用：`private` 只类内可访问；`static` 字段属于类；`final` 引用不能重新赋值——但 `CURRENT_ID.set()` 改的是**当前线程绑定的值**，与 `final` 不冲突。

### 5.2 为什么不用 Lombok @Data

**`@Data` 会完全跳过 `static` 字段**，不会生成 `getCURRENT_ID()`。而且业务要的是"存/取/清一个 Long"，不是暴露 ThreadLocal 对象本身——这是封装。

### 5.3 get/set/remove 的精确语义

- `get()` 不是"按时间变化"，而是读"**当前执行线程**此时关联的值"；未设置过默认返回 `null`（`withInitial` 提供初始值时例外）。
- `remove()` 只解除当前线程的绑定，**不会终止线程**。
- 共享的是 ThreadLocal **对象引用**，不是值——`static` 不等于"所有线程共享同一个用户 ID"。
- 泛型不能写 `ThreadLocal<long>`，必须 `ThreadLocal<Long>`；`set(100L)` 时 `long` 自动装箱。
- C++ 对照：`thread_local long long id` 可帮助理解"每线程一份"，但语义不同（C++ 有初值 0，Java 未设置是 null）。

### 5.4 JWT 拦截器逐段要点

```text
preHandle:
  1. BaseContext.removeCurrentId()     ← 防御性清理（线程复用防旧身份）
  2. OPTIONS 预检 / 非 HandlerMethod 放行
  3. 取 header（项目约定 "token"，不是 Authorization）
  4. parseToken → 验 Claims（sub/role/exp）
  5. 查库确认账号仍存在且未停用（JWT 是签发时快照）
  6. 全部通过 → setCurrentId(employeeId) → return true
afterCompletion:
  7. removeCurrentId()
```

- **401 vs 403**：401 = 身份未认证/凭证无效；403 = 身份已认证但无权限。状态码设计要和前端统一。
- **`afterCompletion` 的触发条件**：只有本拦截器 `preHandle()` 返回 `true` 才会调用；返回 `false` 或异步处理有特例。所以"请求结束必清理"的说法不准确——`preHandle` 开头的防御性 `remove()` 才有保障。
- **校验边界不能凭注释**：`parseToken` 是否真的验签、验 `exp`/issuer，必须看 `JwtUtil` 实现；JWT 的 `exp` 是 NumericDate（秒级数值），不是日期字符串。
- **JWT 签名 ≠ 加密**：Payload 是明文，别存敏感信息。

## 六、完整链路

```text
登录：查用户名 → BCrypt.matches → 签发 JWT → 返回前端
请求：Authorization: Bearer <JWT> → Interceptor.preHandle
      → 验签+查 exp → ThreadLocal.set(userId) → 放行
Controller/Service：BaseContext.getCurrentId() 拿当前用户
完成：afterCompletion → ThreadLocal.remove()
```

## 📌 易错点

- "MD5/SHA-256 加密密码"——术语和做法都错。
- 重新 `encode()` 再比较字符串——盐的随机性让这永远失败。
- JWT Payload 当保险箱——它只是签名过的明文声明。
- `preHandle` 返回 false 就自动 401——不会，要自己设置响应。
- ThreadLocal 当全局变量用——线程级存储，必须清理、不跨线程。
- 长期维护自研安全体系——生产优先 Spring Security。

## 🧠 速记口诀

```text
密码：BCrypt 加盐慢哈希，matches 验证不复 encode
JWT：三段两点，编码非加密，验签验期再谈信任
拦截：Filter 在门外，Interceptor 在门内，注册了才拦
ThreadLocal：线程私有，用完 remove，异步不传递
```

## ✅ 自测问题

1. AES、SHA-256、BCrypt 存密码的区别？BCrypt 不能解密为何能验证？
2. JWT 三段结构？为什么 Payload 不能存密码？
3. 改 Payload 里的 role 为什么会导致验签失败？
4. `preHandle` 返回 true/false 各发生什么？Filter 如何放行？
5. ThreadLocal 的 static 会让不同线程共享用户 ID 吗？为什么必须 remove？
6. `@Data` 能给 `static ThreadLocal` 字段生成 Getter 吗？业务上为什么也不需要？
7. 验证了 JWT 为什么还查数据库？401 和 403 分别什么场景？
8. `afterCompletion` 在 `preHandle` 返回 false 后还会执行吗？那清理责任在哪？

> 来源：原始《Java 后端学习笔记：JWT、Filter 与 Interceptor》(Java后端学习笔记_JWT与FilterInterceptor.md)、《Java 后端学习笔记：密码安全、JWT、Interceptor 与 ThreadLocal》(Java后端学习笔记_密码安全JWT与ThreadLocal.md)、《Java 后端基础学习笔记》(Java后端基础_本次聊天学习笔记.md §4/§12，MD5 部分)、《Java ThreadLocal 与 Spring MVC JWT 拦截器：对话纠错学习笔记》(Java_ThreadLocal与JWT拦截器_对话纠错学习笔记_2026-10-14.md，ThreadLocal 深挖/拦截器逐段/边界条件)，重叠部分已去重合并；概念区分（编码/哈希/加密）详见 `16-单元测试与安全基础.md`。
