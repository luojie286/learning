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

## 五、完整链路

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

> 来源：原始《Java 后端学习笔记：JWT、Filter 与 Interceptor》(Java后端学习笔记_JWT与FilterInterceptor.md)、《Java 后端学习笔记：密码安全、JWT、Interceptor 与 ThreadLocal》(Java后端学习笔记_密码安全JWT与ThreadLocal.md)、《Java 后端基础学习笔记》(Java后端基础_本次聊天学习笔记.md §4/§12，MD5 部分)，重叠部分已去重合并；概念区分（编码/哈希/加密）详见 `16-单元测试与安全基础.md`。
