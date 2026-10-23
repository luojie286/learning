---
id: java-validation
title: "DTO、校验与统一错误响应"
description: "分离结构转换、字段约束、业务规则和数据库约束。"
course: java-web
order: 5
date: 2026-10-23
tags: [Java, Spring]
---

# DTO、校验与统一错误响应

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 数据边界比类名更重要

DTO 负责某次操作的输入或传输，持久化模型映射数据库，VO 选择对外字段。这是工程约定，不是 Java 的特殊类型。不要把含密码哈希的用户 Entity 直接返回，也不要让客户端通过任意字段绑定给自己修改 role。

```java
public record RegisterRequest(
    @NotBlank @Size(min = 3, max = 50) String username,
    @NotBlank @Email String email,
    @NotBlank @Size(min = 12, max = 100) String password) {}
```

示例长度仅为教学约定，真实密码规则还需结合编码器能力。Boot 3 使用 jakarta.validation，通常加入 starter-validation。标准 @Size 没有 value 属性，@Size(50) 不可编译；长度约束也通常不拒绝 null，需要额外非空约束。

## 校验在什么阶段发生

```java
@PostMapping("/registrations")
ResponseEntity<Void> register(@Valid @RequestBody RegisterRequest dto) {
    registrationService.register(dto);
    return ResponseEntity.status(201).build();
}
```

这是 Controller 方法片段，registrationService 需由应用实现和注入。消息转换先读 JSON，再根据适用校验机制检查 DTO。无效 JSON 不等于字段违反 @NotBlank；在默认情况下对象校验常抛 MethodArgumentNotValidException，Spring 6.1+ 方法级约束还可能抛 HandlerMethodValidationException。

@Valid 是级联校验标记，不是 @NotNull。嵌套地址对象既要必填又要校验内部字段时，组合 @NotNull 和 @Valid。@Validated 可选择分组，方法校验与类级 AOP 校验需按版本配置，不能机械叠注解。

## 实际开发：注册用户名冲突

DTO 验证用户名长度，Service 检查业务规则，数据库 UNIQUE(username) 抵抗并发重复注册。先 selectCount 再 insert 不能代替唯一约束。发生唯一键冲突时转换成可理解的 409 响应，同时保留内部诊断证据。

统一异常处理可用 @RestControllerAdvice、@ExceptionHandler 或 ResponseEntityExceptionHandler。框架不会因为写了 message 属性就自动返回项目自定义 code/msg 结构。输出字段、状态码与错误码须明确约定，系统异常日志留服务端，不把栈、SQL 或密钥发给客户端。

## Lombok 的边界

@Data 组合 getter、setter、toString、equals/hashCode 与必需参数构造等能力，不是全参构造器。自动 toString 可能泄漏密码，自动 equals 也可能不符合持久化对象身份；按需使用细粒度注解，或 record 明确响应模型。

## 参考资料

- [Spring 6.2 MVC 校验](https://docs.spring.io/spring-framework/reference/6.2/web/webmvc/mvc-controller/ann-validation.html)
- [Jakarta Validation 3.0 规范](https://jakarta.ee/specifications/bean-validation/3.0/jakarta-bean-validation-spec-3.0.html)
- [Lombok @Data](https://projectlombok.org/features/Data)

## 模拟面试问题

1. 【基础】@NotNull、@NotEmpty 和 @NotBlank 有何不同？
2. 【应用】@Valid 和 @NotNull 为什么可能需要同时使用？
3. 【应用】预先查询用户名存在与数据库唯一约束各起什么作用？
4. 【深入】无效 JSON 与 Bean Validation 错误分别在哪一步发生？
5. 【深入】@Data 用在密码 DTO 和 Entity 上有哪些隐患？

