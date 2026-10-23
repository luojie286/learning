---
id: java-lambda
title: "内部类、Lambda 与回调"
description: "把行为作为参数传递，并识别捕获变量与 this 的差异。"
course: java-basics
order: 5
date: 2026-10-23
tags: [Java, 后端]
---

# 内部类、Lambda 与回调

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 内部类与嵌套类

成员内部类关联外部实例，通过 outer.new Inner() 创建；静态嵌套类不隐含外部实例，用 new Outer.Inner() 创建。局部类声明在代码块内，匿名类在表达式中创建无名子类或接口实现。

原稿把所有局部类、匿名类都标成持有外部实例不严谨：是否存在外部实例取决于声明环境，例如静态上下文不能隐式取得外部 this。长期被缓存的回调若捕获服务对象，可能连带保留大量状态，应关注引用链而非仅看语法名字。

## 函数式接口是目标类型

Lambda 需要兼容的函数式接口，关键是一个符合规则的抽象方法，允许 default/static 等方法。@FunctionalInterface 可让编译器检查契约。

| 接口 | 行为 |
|---|---|
| Predicate<T> | T → boolean，判定 |
| Function<T,R> | T → R，转换 |
| Consumer<T> | T → void，消费 |
| Supplier<T> | 无参数 → T，延迟提供 |

```java
Predicate<Integer> valid = n -> n > 0;
Function<String, Integer> length = String::length;
Supplier<List<String>> create = ArrayList::new;
Comparator<Integer> compare = Integer::compare;
```

方法引用不立即执行方法；被调用方在需要时使用这段行为。比较任意整数用 Integer.compare，不能用 a-b 冒溢出风险。

## 实际开发：上传完成回调

```java
void upload(byte[] bytes, Consumer<Integer> onComplete) {
    // 此处只模拟同步处理；真实上传还需 IO 与错误处理
    onComplete.accept(bytes.length);
}
upload(new byte[64], size -> System.out.println("已处理字节：" + size));
```

回调只表示“把逻辑交给对方、由对方在约定时机调用”，不天然异步。要判断执行线程，必须看 upload 实现或框架契约。

## 捕获限制不等于不可变

被捕获局部变量须 final 或 effectively final，表示引用或值不再重新赋值；其指向集合的内部内容仍可能改变，线程安全也不会自动获得。Lambda 的 this 沿用词法环境中的 this，匿名类的 this 指匿名对象。

不要依赖 Lambda 恰好生成某种 class 文件或总比匿名类快。实现细节和性能要用具体 JDK 与真实调用方式验证。

## 参考资料

- [JLS 17：Lambda 表达式](https://docs.oracle.com/javase/specs/jls/se17/html/jls-15.html#jls-15.27)
- [JLS 17：内部类](https://docs.oracle.com/javase/specs/jls/se17/html/jls-8.html#jls-8.1.3)

## 模拟面试问题

1. 【基础】函数式接口能有多个 default 方法吗？
2. 【应用】方法引用与立即调用 getter 有什么区别？
3. 【应用】回调什么时候是同步的，什么时候是异步的？
4. 【深入】effectively final 为什么不保证集合不可变？
5. 【深入】Lambda 与匿名类中的 this 分别指向谁？

