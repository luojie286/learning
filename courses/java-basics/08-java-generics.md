---
id: java-generics
title: "泛型、通配符与类型擦除"
description: "用分页结果和批量处理理解类型安全及 PECS。"
course: java-basics
order: 8
date: 2026-10-23
tags: [Java, 后端]
---

# 泛型、通配符与类型擦除

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 泛型保护边界

`List<String>` 把元素类型约束提前到编译期，减少强转与运行时错误。List<Integer> 不是 List<Number> 的子类型，否则可向整数列表塞入 Double。数组协变是不同机制，可能在运行时抛 ArrayStoreException，不能套用到泛型。

```java
record PageResult<T>(long total, List<T> records) {}
static <T> void copy(List<? extends T> from, List<? super T> to) {
    for (T value : from) to.add(value);
}
```

生产者 extends：能安全读为上界类型，不能随意放入某个具体子类型；消费者 super：能写入 T，读出来通常只能视为 Object。PECS 是 API 设计提示，不是所有出现 extends 都“绝不能写任何东西”的绝对规则，例如 null 或合法删除操作另有契约。

## 实际开发：统一分页

PageResult<OrderVO> 和 PageResult<UserVO> 共享分页结构，仍保留各自 records 的编译期类型。不要用原始类型 PageResult 或 List 抹去检查，再靠调用方猜测。若一个方法同时读取并修改同一种元素，明确 List<T> 往往比通配符更好。

## 擦除的含义

大部分类型参数在字节码中擦除为上界或 Object，并伴随必要强转、桥接方法。不能写 new T()、new List<String>[10] 或 instanceof List<String>。但 class 文件可保留泛型签名元数据，反射能在部分声明位置读取 ParameterizedType；不能说运行时“一切泛型信息都完全消失”。

反射读取到 List<User> 字段声明，并不意味着任意 List 实例都能报告实际元素类型。JSON 框架处理 List<User> 时常需类型令牌，正因为只传 List.class 无法完整表达元素类型。

## 边界与误区

泛型类型实参不能直接是 int，要使用 Integer；可变参数叠加泛型可能产生堆污染。不要为消除警告大范围加 SuppressWarnings，先定位是哪一处突破类型边界。泛型本身也不提供线程安全或运行时内容校验。

## 参考资料

- [JLS 17：类型变量与参数化类型](https://docs.oracle.com/javase/specs/jls/se17/html/jls-4.html#jls-4.4)
- [Oracle：通配符](https://docs.oracle.com/javase/tutorial/java/generics/wildcards.html)

## 模拟面试问题

1. 【基础】为什么 List<Integer> 不能赋值给 List<Number>？
2. 【应用】批量复制 API 为什么适合使用 extends 和 super？
3. 【应用】分页对象如何保留元素类型约束？
4. 【深入】类型擦除与反射读取泛型声明信息矛盾吗？

