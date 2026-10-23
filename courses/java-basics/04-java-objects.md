---
id: java-objects
title: "对象、继承与接口契约"
description: "从引用到动态分派，用支付策略理解抽象与边界。"
course: java-basics
order: 4
date: 2026-10-23
tags: [Java, 后端]
---

# 对象、继承与接口契约

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 变量类型与真实对象

声明 `Payment p` 没有创建对象，`new CardPayment()` 才是普通实例化。引用的编译期类型决定哪些成员可访问；被重写实例方法的实际执行取决于运行时类型。字段访问和静态方法隐藏按编译期类型解析，不能笼统说“所有成员都有多态”。

重载在编译期根据签名和参数类型选择；重写遵循继承契约。子类不能缩小可见性，不能扩大受检异常范围；引用返回类型可协变，基本类型返回值不能随意改变。private 方法不被继承重写，原笔记“private → public 是扩大重写权限”的口诀不成立。

## 实际开发：支付能力

```java
interface Payment {
    String pay(long orderId);
}
final class FakePayment implements Payment {
    @Override public String pay(long orderId) {
        return "test-" + orderId;
    }
}
final class Checkout {
    private final Payment payment;
    Checkout(Payment payment) {
        this.payment = Objects.requireNonNull(payment);
    }
    String checkout(long orderId) { return payment.pay(orderId); }
}
```

Checkout 依赖契约，测试可传 FakePayment，正式环境传真实网关实现。构造器注入只是把依赖从外部交进来，不要求必须使用 Spring。接口类型与实现类类型不是“两个对象”，赋值或转型也不创建对象。

## 抽象类与接口怎么选

抽象类可存实例状态、提供构造器和部分实现，适合有稳定共享骨架的类族。接口声明能力契约，允许 default、static、private 辅助方法；接口字段隐含 public static final。一个类可实现多个接口，只能直接继承一个类。

“是什么／能做什么”只是帮助记忆，不能替代对状态、演化和复用的设计判断。默认方法冲突必须按 Java 规则消歧。接口也不保证其所有实现都线程安全。

## 组合与生命周期

持有其他对象的字段至少说明关联。严格 UML 组合还强调生命周期归属，因此 Controller 引用单例 Service 不宜直接称为强组合。业务复用通常先考虑委托，避免为复用几个方法继承不合适的父类。

向下转型要有类型依据；Spring 注入的接口对象可能是代理，强转实现类可能失败。需要能力时把它放进合适契约，而不是到处 instanceof 补洞。

## 参考资料

- [JLS 17：类](https://docs.oracle.com/javase/specs/jls/se17/html/jls-8.html)
- [JLS 17：接口](https://docs.oracle.com/javase/specs/jls/se17/html/jls-9.html)

## 模拟面试问题

1. 【基础】重写与重载的选择发生在什么阶段？
2. 【基础】字段和静态方法是否按运行时对象多态分派？
3. 【应用】支付网关为什么适合通过接口注入？
4. 【深入】private 方法为什么不构成被重写方法？
5. 【深入】抽象类和默认接口方法如何选择？

