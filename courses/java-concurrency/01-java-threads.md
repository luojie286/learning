---
id: java-threads
title: "线程、锁与协作取消"
description: "从库存扣减理解临界区、互斥和中断协议。"
course: java-concurrency
order: 1
date: 2026-10-23
tags: [Java, 后端]
---

# 线程、锁与协作取消

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 并发问题来自共享状态

进程提供资源和隔离边界，线程是进程内执行路径。调用 run 是普通方法调用，start 才请求启动新线程；同一 Thread 对象不能重复 start。并发不一定并行，多线程也不保证更快。

count++ 包含读、计算、写回，两个线程可读到同一旧值，产生丢更新。锁的保护对象是业务不变量，而不是“某一行看起来危险的代码”。

```java
final class Stock {
    private int remaining = 1;
    synchronized boolean reserve() {
        if (remaining <= 0) return false;
        remaining--;
        return true;
    }
}
```

检查与扣减在同一临界区，且所有相关访问遵守同一把锁。不同 Stock 实例的实例锁不互斥；静态同步方法锁住对应 Class 对象。JVM 锁不能阻止另一台服务器修改数据库库存，真实下单需数据库条件更新与事务。

## synchronized 与显式锁

synchronized 在正常或异常离开时释放监视器，支持重入。ReentrantLock 提供尝试获取、超时和可中断获取，必须在成功获取后用 finally unlock。需要等待锁时选择 lockInterruptibly，并合理传播中断。

固定获取多把锁的顺序可以减少循环等待；避免持锁调用耗时远程服务。排障看线程转储中的持有者和等待关系，不是简单把“线程很多”当成死锁。

## 等待与通知

wait 必须持有对象监视器，并会释放该监视器；sleep 不释放已持有锁。等待条件必须用 while 检查，防止虚假唤醒和唤醒后条件已被其他线程改变。notify 不是立刻转交锁，等待线程仍需重新竞争。

实际生产消费优先 BlockingQueue，而非手写 wait/notify。学习底层目的是理解协议和故障，不是重复实现成熟容器。

## 取消是协作协议

interrupt 不是强制杀线程。许多阻塞方法收到中断抛 InterruptedException 并清除状态；不能处理时通常向上传播，或恢复中断状态后退出任务。仅恢复状态却继续无限循环也不能完成取消。对阻塞 IO 还需遵循对应 API 的关闭、超时和中断契约。

## 参考资料

- [Thread API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/Thread.html)
- [Lock API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/locks/Lock.html)
- [Object.wait API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/Object.html#wait())

## 模拟面试问题

1. 【基础】start 与 run 有什么区别？
2. 【应用】为什么检查库存与扣减必须保护为一个整体？
3. 【应用】wait 和 sleep 对锁的处理有什么不同？
4. 【深入】为什么捕获中断后只打印异常可能导致停机失败？
5. 【深入】如何分析互相等待的两条线程？

