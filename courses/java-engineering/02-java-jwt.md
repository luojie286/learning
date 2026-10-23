---
id: java-jwt
title: "JWT 验证、认证与资源授权"
description: "避免把可解码声明当可信身份，并明确令牌生命周期。"
course: java-engineering
order: 2
date: 2026-10-23
tags: [Java, 后端]
---

# JWT 验证、认证与资源授权

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 格式、传输与框架

JWT 是声明格式，Bearer 是持有凭据的使用方式，Spring Security 是安全框架。常见三段是签名 JWT 的紧凑 JWS，不是所有 JWT 都恰好三段；加密 JWE 有另一种结构。

JWS 的 Header 和 Payload 通常只是 Base64URL 编码，可被读取，不能放密码、密钥或不应公开的隐私。仅解码出 sub=1001 没有建立可信身份，必须验证可信算法、密钥与应用声明。

## 验证链路

接收 Authorization: Bearer token，规范解析前缀，验证签名、exp/nbf、issuer、audience 和令牌用途，再按应用检查撤销与账户状态。不要接受 token 自报的任意算法，也不要从用户提交的 URL 任意下载验证密钥。

JJWT 0.12.x 解析片段：

```java
Claims claims = Jwts.parser()
    .verifyWith(trustedKey)
    .requireIssuer("shop-api")
    .build()
    .parseSignedClaims(token)
    .getPayload();
```

trustedKey 由服务端可信配置提供；这是验证 API 片段，实际还要按业务校验 audience、要求的声明与算法集合，不能当完整认证模块。模块依赖版本需一致，旧 parserBuilder 等教程 API 不与 0.12.x 混写。HS256 需足够强随机密钥材料，不能把人类短口令随意当密钥。

## 实际开发：查询自己的订单

```sql
SELECT id, amount, status
FROM orders
WHERE id = ? AND user_id = ?;
```

user_id 从可信认证上下文取得，不能直接由客户端指定后就信任。Token 有效只解决认证，订单归属与管理员权限是授权；否则用户 A 修改 URL 就可能查看用户 B 的订单。

401 表示缺少或没有有效凭据，403 表示拒绝授权等情形；Bearer 方案还应按协议形成适用认证响应头。不要把数据库断线、密钥配置错误等所有异常捕获成 401，系统故障需保留自己的诊断路径。

## 撤销、刷新与存储

短期 access token 限制暴露窗口；刷新凭据需独立生命周期、轮换与重放防护。黑名单、版本号或会话状态可支持撤销，但会改变“完全无状态”的假设。JWT 不天然优于服务端 Session。

浏览器 Cookie 与可由脚本读取的存储各有 XSS/CSRF 边界，需按应用架构选择；JWT 本身不防这些攻击。正式 Servlet 项目优先使用 SecurityFilterChain 或成熟资源服务器验证，不只靠 MVC Interceptor 匹配路径。

## 参考资料

- [RFC 7519 JWT](https://www.rfc-editor.org/rfc/rfc7519.html)
- [RFC 8725 JWT 最佳实践](https://www.rfc-editor.org/rfc/rfc8725.html)
- [JJWT 官方说明](https://github.com/jwtk/jjwt)
- [Spring Security JWT 资源服务器](https://docs.spring.io/spring-security/reference/servlet/oauth2/resource-server/jwt.html)

## 模拟面试问题

1. 【基础】JWT、JWS 与 Bearer 分别指什么？
2. 【应用】Token 验证成功后为什么还要检查订单所属用户？
3. 【应用】如何测试篡改、过期、错误发行者和撤销令牌？
4. 【深入】登出如何影响已经签发的自包含 JWT？
5. 【深入】为什么不能把所有异常统一当成认证失败？

