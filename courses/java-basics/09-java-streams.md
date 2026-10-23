---
id: java-streams
title: "Stream 与展示模型转换"
description: "将筛选、映射和收集组成清晰的数据管道。"
course: java-basics
order: 9
date: 2026-10-23
tags: [Java, 后端]
---

# Stream 与展示模型转换

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## Stream 是计算描述

Stream 不存储数据，不是可重复消费的集合；中间操作形成管道，终止操作触发求值。filter 筛选，map 转换，flatMap 展开多层，sorted 排序。forEach 是消费动作，不能把它当作生成新元素的 map。

```java
record Order(long id, boolean paid, BigDecimal amount) {}
record OrderVO(long id, String amount) {}
List<OrderVO> result = orders.stream()
    .filter(Order::paid)
    .sorted(Comparator.comparingLong(Order::id).reversed())
    .map(o -> new OrderVO(o.id(), o.amount().toPlainString()))
    .toList();
```

这里把数据库模型转换成响应模型，只暴露 id 和金额文本。不是“Stream 自动防止泄漏”，而是 map 中显式选择了字段。大量筛选和分页应尽量交给数据库，不要先把全表读到 JVM 再 filter。

## 结果集合的契约

Java 16 起 Stream.toList 返回不可修改 List；Collectors.toList 不保证实现和可修改性。明确需要可修改列表时使用 toCollection(ArrayList::new)。List.of 从 Java 9 提供，也不可修改，不能机械把 Java 8 教程代码替换而忽略调用方后续 add。

中间操作惰性不意味着每个 Lambda 保证执行一次：优化和短路会影响调用次数。因此不要在 map、peek 中依赖发送短信等必须发生的副作用。

## 实际开发的陷阱

不要在过滤中修改原始集合，也不要把普通 ArrayList 作为 parallelStream 的共享累加器。并行流不是“免费加速”：拆分、合并、线程争用与阻塞 IO 可能让它更慢，还可能影响公共 ForkJoinPool 的其他任务。

toMap 遇重复键要明确合并规则，否则可能抛异常。订单行按商品归并时，保留第一条、求和或报错是业务决定，不能随意用 (a,b)->a 掩盖重复数据。

## 可读性选择

三步以内的筛选转换通常清晰；多重异常、提前退出和复杂状态更新用普通循环可能更容易维护。学习目标是解释每一步数据如何变化，而非把所有循环改写成链式调用。

## 参考资料

- [Java 17 Stream API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/stream/Stream.html)
- [Collectors API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/stream/Collectors.html)

## 模拟面试问题

1. 【基础】中间操作和终止操作有什么区别？
2. 【应用】把 Entity 转成 VO 时如何控制暴露字段？
3. 【应用】toMap 遇到重复键应该怎样决定行为？
4. 【深入】为什么不能在 peek 中依赖必须执行的业务副作用？
5. 【深入】parallelStream 处理阻塞接口可能有哪些问题？

