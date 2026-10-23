---
id: java-executors
title: "线程池、Future 与异步编排"
description: "明确线程、队列和超时预算，避免异步把负载藏起来。"
course: java-concurrency
order: 4
date: 2026-10-23
tags: [Java, 后端]
---

# 线程池、Future 与异步编排

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 线程池是一种容量管理

ThreadPoolExecutor 管理工作线程、任务队列和拒绝策略。通常在工作线程少于 core 时尝试创建线程，否则入队；队列无法接收时尝试扩到 max，再无法执行则拒绝。关闭状态等并发情况也会影响接收，四步口诀不是精确穷尽源码。

```java
ThreadPoolExecutor pool = new ThreadPoolExecutor(
    4, 8, 30, TimeUnit.SECONDS,
    new ArrayBlockingQueue<>(64),
    Executors.defaultThreadFactory(),
    new ThreadPoolExecutor.AbortPolicy());
```

core 默认也按需创建，不是构造线程池就预先起齐；可配置预启动与核心超时。无界队列往往使 max 不起预期扩容作用。线程数应结合 CPU、阻塞时间、数据库连接池和下游配额，不能固定套“核数×2”。

## 拒绝是业务可见的失败

AbortPolicy 抛拒绝异常；CallerRunsPolicy 可能占用请求或事件线程，并且关闭时不执行，不能宣称提交总是立即返回。丢弃策略可能让等待 Future 的逻辑无法完成，生产路径不能随意静默丢任务。服务应观测队列长度、等待时间、活跃线程、完成数和拒绝数。

submit 返回 Future，不代表任务成功；异常一般需 get 或完成回调观察。get 可阻塞并抛受检异常；Future.cancel(true) 只是尽力协作取消，任务仍需支持中断。

## 实际开发：并行聚合商品详情

```java
CompletableFuture<Product> product =
    CompletableFuture.supplyAsync(() -> loadProduct(id), ioPool);
CompletableFuture<Stock> stock =
    CompletableFuture.supplyAsync(() -> loadStock(id), ioPool);
CompletableFuture<Detail> detail =
    product.thenCombine(stock, Detail::new)
           .orTimeout(800, TimeUnit.MILLISECONDS);
```

loadProduct/loadStock、Detail 与 ioPool 是应用自定义依赖，此片段展示编排而非完整工程。thenApply 转换值，thenCompose 展平下一个异步阶段，thenCombine 合并独立结果；不带 Async 的延续可能在完成线程或当前线程执行，不能当作“自动开新线程”。

## 超时与收尾

orTimeout 让 Future 超时完成，不保证底层 HTTP 或 JDBC 调用真正停止；下游仍需连接、读取和查询超时。避免同一有限线程池内任务互相 get 等待造成饥饿。显式选择业务执行器，不把阻塞请求塞进公共池。

Java 17 的 ExecutorService 不按后续版本的 try-with-resources 用法处理；生命周期中 shutdown，按预算 awaitTermination，必要时 shutdownNow 并响应中断。

## 参考资料

- [ThreadPoolExecutor API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/ThreadPoolExecutor.html)
- [CompletableFuture API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/CompletableFuture.html)
- [Future API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/Future.html)

## 模拟面试问题

1. 【基础】任务入队和扩到 maximumPoolSize 的顺序是什么？
2. 【应用】为什么无界队列可能导致延迟和内存不断增长？
3. 【应用】thenCompose 与 thenCombine 如何选择？
4. 【深入】Future 超时为什么不等于底层调用已经停止？
5. 【深入】线程池内互相等待如何造成饥饿？

