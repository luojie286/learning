---
id: java-juc
title: "原子类、并发容器与限流协作"
description: "正确组合 CAS、队列和信号量，不把线程安全方法误当业务原子性。"
course: java-concurrency
order: 3
date: 2026-10-23
tags: [Java, 后端]
---

# 原子类、并发容器与限流协作

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## CAS 保护一次状态变更

CAS 比较当前值与期望值，相等才原子替换。AtomicInteger 的 getAndIncrement 返回旧值，incrementAndGet 返回新值。多个原子方法组合仍可能竞态：get()<10 后 increment 不保证上限 10。

```java
static boolean acquire(AtomicInteger inFlight, int limit) {
    for (;;) {
        int old = inFlight.get();
        if (old >= limit) return false;
        if (inFlight.compareAndSet(old, old + 1)) return true;
    }
}
```

成功后业务结束必须对应释放；这只是单进程并发计数示例，不是跨服务器限流。复杂资源许可通常直接使用 Semaphore，获得许可后 finally release。公平与吞吐也需要按业务权衡。

CAS 看见 A→B→A 时可能漏掉历史变化；需辨认版本时考虑带 stamp 的引用。高竞争重试有 CPU 成本，不能把所有原子 API 都简化成“永远无锁且更快”。LongAdder 适合热点统计，但 sum 不提供原子快照，不适合严格库存余额。

## ConcurrentHashMap 的边界

```java
counts.computeIfAbsent(route, k -> new LongAdder()).increment();
```

并发 Map 保证各方法契约，不保证先 containsKey 再 put 的整个流程原子。compute 回调保持短小，避免阻塞远程调用或递归更新同一 map。它不允许 null 键和值；迭代通常弱一致，不是整个容器固定快照。

## 有界队列形成背压

ArrayBlockingQueue 用固定容量限制缓冲。put/take 等待空间或数据，offer/poll 立即失败返回，也有带超时版本。原稿把 offer/poll 一律描述为“不该用于生产消费”不成立：超时或立即拒绝正是许多服务的过载策略。

例如图片压缩生产快于消费者时，无界队列只是把延迟变成内存堆积；有界队列配超时、拒绝和指标，才能明确服务容量。持锁调用 queue.put 可能引发额外死锁风险。

## 同步工具怎么选

CountDownLatch 等待一批前置任务完成，倒计数到零不能重置；Semaphore 限制同时进入某区域的许可数。两者都不能替代所有共享状态保护，也不负责持久化故障恢复。

## 参考资料

- [AtomicInteger](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/atomic/AtomicInteger.html)
- [ConcurrentHashMap](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/ConcurrentHashMap.html)
- [BlockingQueue](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/BlockingQueue.html)
- [Semaphore](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/Semaphore.html)

## 模拟面试问题

1. 【基础】getAndIncrement 和 incrementAndGet 返回什么？
2. 【应用】为什么多个原子方法组合仍可能超限？
3. 【应用】有界队列满时有哪些合理的处理方式？
4. 【深入】LongAdder 为什么不适合严格余额？
5. 【深入】CountDownLatch 和 Semaphore 分别解决什么问题？

