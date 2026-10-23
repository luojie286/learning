---
id: java-time
title: "时间、正则与边界输入"
description: "用订单截止时间和表单校验理解时间模型与字符串规则。"
course: java-basics
order: 11
date: 2026-10-23
tags: [Java, 后端]
---

# 时间、正则与边界输入

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 时间轴与日历

LocalDate 表示日期，LocalDateTime 表示无时区的日期时间，Instant 表示绝对时刻；OffsetDateTime 带 UTC 偏移，ZonedDateTime 带地区时区及历史规则。相同 LocalDateTime 在不同时区对应不同 Instant，夏令时切换处甚至可能不存在或对应两个时刻。

```java
Instant createdAt = clock.instant();
Instant expiresAt = createdAt.plus(Duration.ofMinutes(15));
boolean expired = !clock.instant().isBefore(expiresAt);
ZonedDateTime display = createdAt.atZone(ZoneId.of("Asia/Shanghai"));
```

订单过期按绝对时刻比较，用户展示时再转换时区。Clock 由调用方提供，可在测试中固定时间，避免 sleep 等待 15 分钟。每日结算日则需要先定义业务时区和日历边界，不应一律把“一天”视为任何环境下都等于 24 小时。

## 格式与存储

DateTimeFormatter 不可变且线程安全；自定义严格日期解析可用 uuuu-MM-dd，注意 MM 月份、mm 分钟、HH 24 小时制、YYYY 周基年。旧 SimpleDateFormat 可变且不适合直接共享。

API 应注明秒还是毫秒、是否含偏移。MySQL DATETIME 不自动保存地区时区语义，数据库与 JDBC 的约定须共同检查。跨区展示不要依赖服务器默认时区。

## 正则两层转义

```java
Pattern digits = Pattern.compile("\\d{6}");
boolean valid = input != null && digits.matcher(input).matches();
Matcher matcher = Pattern.compile("order-(\\d+)").matcher("order-123");
if (matcher.find()) System.out.println(matcher.group(1));
```

Java 字符串先解释反斜杠，正则引擎再解释。matches 检查完整输入，find 查找下一处匹配。固定 Pattern 可复用，Matcher 带匹配状态不应跨线程共享。

## 实际开发的限制

注册邮箱或手机号的正则只能校验格式，不证明地址真实、号码归属或用户身份。处理不可信长输入时先限制长度，避免复杂回溯正则耗尽 CPU；不要用一个巨大正则解析完整 HTML 或复杂协议。

UUID.randomUUID 使用强伪随机来源，不能错误写成“没有安全随机性”；但 ID 和登录凭证用途不同。需要可撤销、一次性重置凭据时，除了随机性还要设计到期、哈希存储、使用次数和泄漏处理。

## 参考资料

- [java.time 包](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/time/package-summary.html)
- [Pattern API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/regex/Pattern.html)
- [UUID API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/UUID.html)

## 模拟面试问题

1. 【基础】LocalDateTime 与 Instant 的区别是什么？
2. 【应用】怎样测试订单恰好在到期时刻的行为？
3. 【应用】matches 与 find 有什么区别？
4. 【深入】为什么有时区的一个日历日不总是 24 小时？
5. 【深入】邮箱正则校验通过后还缺什么验证？

