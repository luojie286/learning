---
id: java-passwords
title: "密码存储、编码与密码学用途"
description: "把表示、摘要、加密与身份验证分清，构建合理登录基础。"
course: java-engineering
order: 1
date: 2026-10-23
tags: [Java, 后端]
---

# 密码存储、编码与密码学用途

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 四种能力不同

Base64 是可逆编码，不保密；普通哈希用于摘要，不能“解密”；对称加密如 AES 使用密钥保护机密性；数字签名通常私钥签、公钥验，HMAC 则双方共享密钥。ECC 是算法家族，不代表某个统一的“加解密接口”。

TLS 还涉及证书身份验证与密钥协商。只做摘要不能阻止有能力重算摘要的攻击者改数据；需要可信摘要来源、MAC 或签名。不要自己实现密码算法或拼凑生产协议。

## 实际开发：注册与登录

```java
String stored = encoder.encode(rawPassword);
// 将 stored 保存为密码哈希，避免记录 rawPassword
boolean matched = encoder.matches(loginPassword, stored);
```

PasswordEncoder 是接口，encoder 是已注入实现对象，不是每次 encode 自动 new 的占位符。盐和工作因子通常包含在编码后的格式中；同一密码两次 encode 可能不同，不能比较两个新生成的哈希字符串。

专用密码哈希故意增加计算或内存成本，降低离线猜测速度；裸 SHA-256(password) 即便有盐通常仍太快。按安全需求与库支持选择 Argon2id、bcrypt、PBKDF2 等，工作参数需在目标硬件验证，并保留未来升级能力。

## 密码边界不是只有数据库

bcrypt 常见实现存在输入字节长度边界，不能只用 Java 字符数判断后就假设任何长密码都完整处理。统一编码与库行为，拒绝不支持的输入，不能静默截断。认证接口还应控制请求速率、失败尝试和账号状态。

日志、异常、DTO.toString、埋点和请求抓取同样可能泄漏密码。不要把哈希当普通资料返回；泄露哈希仍有离线攻击风险。对用户不存在和密码错误的响应可保持一致，减少枚举信息，但内部监控仍要区分故障。

## 密钥与盐

盐不需保密，目的是让相同密码不表现为相同哈希并抵抗预计算；密钥必须保密，轮换和分发是不同问题。密钥走环境或专门管理服务，不写进 Git；环境变量也不是绝对防泄漏方案，仍需访问控制与日志管理。

随机重置 Token 可用 SecureRandom 生成足够熵字节，服务端只存其摘要，并设置短有效期与单次使用规则。唯一资源 ID 不自动等于授权凭据。

## 参考资料

- [Spring Security 密码存储](https://docs.spring.io/spring-security/reference/features/authentication/password-storage.html)
- [PasswordEncoder](https://docs.spring.io/spring-security/reference/servlet/authentication/passwords/password-encoder.html)
- [JCA 安全架构](https://docs.oracle.com/en/java/javase/17/security/java-cryptography-architecture-jca-reference-guide.html)

## 模拟面试问题

1. 【基础】Base64、哈希与加密分别解决什么问题？
2. 【应用】为什么不能比较两次 encode 的字符串验证密码？
3. 【应用】盐和服务端密钥有什么区别？
4. 【深入】为什么裸 SHA-256 不适合直接保存用户密码？
5. 【深入】密码长度限制为何需要注意字节而非只有字符？

