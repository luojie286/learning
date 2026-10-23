---
id: java-aop
title: "AOP 代理与横切关注点"
description: "用耗时监控理解代理边界、切面回调和自调用。"
course: java-web
order: 3
date: 2026-10-23
tags: [Java, Spring]
---

# AOP 代理与横切关注点

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## AOP 增强什么

日志、事务与耗时统计横跨多个业务方法，AOP 可以将公共逻辑从核心业务提取。Spring AOP 通常基于代理：调用者先到代理，再由代理决定增强与调用目标。注解是元数据，只有对应基础设施读取并应用才生效。

JDK 动态代理围绕接口，CGLIB 类代理通过子类增强。Spring Framework 的选择规则与 Boot 的默认配置需分开：Boot 3 常默认使用类代理，即使实现了接口也不应断言一定是 JDK 代理。final 类与不能覆写的方法限制类代理增强，private 方法也不是普通代理可拦截的外部入口。

## 实际开发：订单接口计时

```java
@Aspect
@Component
class TimingAspect {
    @Around("execution(* com.example.shop.service..*(..))")
    Object time(ProceedingJoinPoint point) throws Throwable {
        long start = System.nanoTime();
        try {
            return point.proceed();
        } finally {
            System.out.println("耗时 ns=" + (System.nanoTime() - start));
        }
    }
}
```

片段要求 AOP 相关依赖与正确扫描。nanoTime 测量经过时间，不用于日历时间戳。真实日志不要打印密码、完整 Token 或用户敏感信息。环绕通知必须明确何时调用 proceed，漏调会导致业务不执行。

@Before 在进入前，@After 类似完成时 finally，@AfterReturning 只在正常返回后，@AfterThrowing 在匹配异常后。统一修改响应体不应误放在 MVC postHandle；不同扩展点有各自生命周期。

## 自调用为什么绕过

外部调用 beanProxy.placeOrder 会经过代理；目标内部 this.reserve 直接调用自身，通常不会再绕到代理。把 @Transactional 或切面注解放在 reserve 上，不保证内部调用生效。更清晰的修复是把独立业务边界放到另一个 Bean，或让事务覆盖真正的外部业务入口，而非默认引入代理自引用。

## 设计模式的关联

代理强调对目标访问的控制和增强，装饰器强调叠加功能，适配器转换接口；结构可能相似，但动机不同。AOP 与 AspectJ 编译/加载时织入也不同，不能把所有 AOP 的限制归结为 Spring 运行时代理限制。

模式是解决问题的语言，不是增加层数的指标。一个没有替换需求的小型纯函数没有必要变成接口、工厂和策略三件套。

## 参考资料

- [Spring 6.2 代理机制](https://docs.spring.io/spring-framework/reference/6.2/core/aop/proxying.html)
- [Boot 3.5 AOP 配置](https://docs.spring.io/spring-boot/3.5/reference/features/aop.html)

## 模拟面试问题

1. 【基础】Spring AOP 代理位于调用链的什么位置？
2. 【应用】环绕通知不调用 proceed 会发生什么？
3. 【应用】为什么同类 this 调用可能不触发事务？
4. 【深入】Framework 代理规则与 Boot 默认配置为何需要区分？
5. 【深入】代理、装饰器、适配器的设计动机有什么区别？

