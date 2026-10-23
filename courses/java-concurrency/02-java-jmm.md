---
id: java-jmm
title: "JMM、happens-before 与 volatile"
description: "用配置快照发布推导可见性，避免把 JMM 当作硬件缓存图。"
course: java-concurrency
order: 2
date: 2026-10-23
tags: [Java, 后端]
---

# JMM、happens-before 与 volatile

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## JMM 规定什么

JMM 描述共享内存操作允许观察到的行为。堆、栈、方法区属于运行时数据区域，回答数据放哪里；JMM 回答线程间如何合法观察读写。“工作内存”是教学抽象，不能直接等同 CPU 缓存，更不能断言 volatile 每次都访问物理 RAM。

原子性保护不可分割操作，可见性限定读写观察，有序性约束可观察行为。单线程语义允许编译器和硬件优化，但源码先后和现实时间先后都不足以单独建立跨线程同步。

## 推导发布关系

```java
final class ConfigHolder {
    private Map<String, String> config = Map.of();
    private volatile boolean ready;
    void publish() {
        config = Map.of("checkout", "enabled");
        ready = true;
    }
    String read() {
        return ready ? config.get("checkout") : "loading";
    }
}
```

限于一次发布且发布后不再修改 config 的场景：写 config → volatile 写 ready → 读取到 true → 读 config，借助程序顺序、volatile 规则和传递性建立 happens-before。ready 为普通字段则缺少这条保证。

常用同步边还包括同一监视器解锁到后续加锁、start 前动作到新线程、线程结束到成功 join 返回。工具类如 Future、并发队列也有各自文档规定的内存一致性效果。

## volatile 适用边界

volatile 保证单次访问的语义与安全发布，count++ 仍不是原子操作，if-check-then-set 也不是。原稿“一写多读才可用”只是常见情形，不是语言限制；多写线程能否使用取决于是否存在复合业务不变量。

更清晰的热更新模式是 `volatile Map<String,String> snapshot = Map.of()`，一次构建不可修改快照再替换引用。只给 List 引用加 volatile，不会让后续 list.add 自动线程安全。

## final 与双重检查

正确构造且未在构造期间逃逸的 final 字段具有特殊可见性保证，但不等于其引用的任意后续变化都可见。双重检查懒加载通常需要 volatile 保证发布，初学者可优先选择静态初始化或已有容器生命周期，减少手工并发状态机。

## 参考资料

- [JLS 17：内存模型与 final 字段](https://docs.oracle.com/javase/specs/jls/se17/html/jls-17.html)
- [java.util.concurrent 内存一致性](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/concurrent/package-summary.html)

## 模拟面试问题

1. 【基础】JMM 与 JVM 内存区域有什么区别？
2. 【应用】请完整推导 ready 为 true 后配置可见的关系。
3. 【应用】volatile List 为什么不能保护 add？
4. 【深入】happens-before 是墙上时钟的先后吗？
5. 【深入】构造期间 this 逃逸为何会破坏安全发布？

