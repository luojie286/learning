---
id: java-context
title: "ThreadLocal 与请求上下文的清理"
description: "理解线程复用、异步边界及认证信息传播。"
course: java-engineering
order: 3
date: 2026-10-23
tags: [Java, 后端]
---

# ThreadLocal 与请求上下文的清理

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## ThreadLocal 绑定线程

ThreadLocal 给每个线程关联独立值，不是在对象中自动创建全局当前用户，也不等于请求作用域。线程池复用线程，旧请求留下值会被下一个请求看到，因此“当前线程结束自然释放”不适用于长期存活的工作线程。

```java
try {
    currentUser.set(verifiedUserId);
    chain.doFilter(request, response);
} finally {
    currentUser.remove();
}
```

这是同步 Servlet Filter 内的生命周期片段，currentUser、verifiedUserId 和 chain 来自应用；必须在进入受保护处理前完成认证。finally 包围真正的下游执行，能覆盖业务异常，而不仅依赖某个后续 MVC 回调。

## 实际开发：用户串号

请求 A 在工作线程 T1 上设置用户 7，异常返回却忘 remove；请求 B 无认证分支复用 T1，若业务只读取旧值，就可能以用户 7 执行操作。解决不只是加一个 null 判断，要让认证、设置、消费和清理形成清晰边界。

ThreadLocalMap 的键弱引用不意味着值立即回收。线程长存活、条目残留或业务仍持有对象引用，都可能保留值。每次使用显式 remove 是生命周期管理，不应寄望某次 GC。

## 异步会切断假设

CompletableFuture、线程池和 Servlet 异步分派可能换线程，普通 ThreadLocal 不自动传播。InheritableThreadLocal 也不适合直接解决线程池复用传播：工作线程创建时继承的值不代表每个提交任务的上下文。

更明确的办法是把必要用户 ID 或不可变上下文作为参数传入任务；必须框架传播时使用其提供的受控机制，并在任务退出时恢复或清理，避免覆盖已有上下文。Spring Security 也有专门的安全上下文传播能力。

## 状态最小化

不要把完整请求、响应、数据库连接或大用户对象长期塞进 ThreadLocal。它们有独立生命周期和资源所有权。只传明确需要的信息，并在服务边界继续做授权，不能因为某处能读到用户 ID 就认为操作已被允许。

响应式 Web 栈的执行模型不同，不要把同步 Servlet 的 ThreadLocal 模式原样搬过去；需要遵循上下文随任务传播的框架契约。

## 参考资料

- [ThreadLocal API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/ThreadLocal.html)
- [InheritableThreadLocal API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/InheritableThreadLocal.html)
- [Spring Security 并发上下文](https://docs.spring.io/spring-security/reference/servlet/integrations/concurrency.html)

## 模拟面试问题

1. 【基础】ThreadLocal 的值与请求还是线程关联？
2. 【应用】线程池复用为何可能导致用户信息串号？
3. 【应用】为什么清理应覆盖异常路径？
4. 【深入】InheritableThreadLocal 为什么不能普遍解决线程池上下文传播？
5. 【深入】异步调用如何显式传递可信用户身份？

