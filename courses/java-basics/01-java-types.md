---
id: java-types
title: "运行平台、类型与数值边界"
description: "从字节码到订单金额，分清基本类型、引用和值传递。"
course: java-basics
order: 1
date: 2026-10-23
tags: [Java, 后端]
---

# 运行平台、类型与数值边界

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 从源码到运行

Java 17 源码经 javac 生成 class 字节码，JVM 可以解释执行，也可以把热点代码即时编译成本机指令。跨平台还要求 class 版本、依赖和本机能力兼容；含 Windows 路径或本地动态库的程序不会仅靠 JVM 自动兼容 Linux。

JDK 提供编译器、调试工具和运行环境。为 Boot 3.x 示例安装兼容 JDK，并检查 `java -version`、`javac -version` 和 Maven 实际使用的 JAVA_HOME，避免编辑器能编译、命令行却报 class 版本过高。

## 类型、范围与默认值

| 类型 | 语言规定 | 实际用途 |
|---|---|---|
| byte / short | 8 / 16 位有符号整数 | 原始字节、协议字段 |
| int | 32 位，−2³¹ 到 2³¹−1 | 数量、页码 |
| long | 64 位有符号整数 | 数据库 ID、毫秒时间 |
| float / double | 32 / 64 位浮点 | 容忍近似的测量 |
| char | 16 位 UTF-16 代码单元 | 字符串底层语义 |
| boolean | true / false | 判断；不能转为 0/1 |

原稿把 int 范围写成 ±2¹⁵，现修正为 32 位范围。boolean 的语言语义不能推出所有 JVM 对象布局中的固定字节数。对象字段有零值或 null；局部变量读取前必须确定赋值。包装类允许 null，自动拆箱可能触发 NPE。

## 实际开发：数量与金额

```java
long bytes = 3L * 1024 * 1024 * 1024;
int total = Math.multiplyExact(quantity, unitCount);
BigDecimal price = new BigDecimal("19.90");
BigDecimal amount = price.multiply(BigDecimal.valueOf(quantity));
```

第一行先用 long 参与乘法，避免 int 已溢出后才转 long。整数溢出通常不会自动报错；需要拒绝溢出时使用 exact 系列方法。金额通常用 BigDecimal 或明确最小单位的整数；舍入位数与模式由业务规则确定。

`new BigDecimal(0.1)` 会保留二进制近似值；字符串构造更明确。BigDecimal 不可变，运算结果要接住；`1.0` 和 `1.00` 数值比较用 compareTo，相等性还需考虑 scale。

## 引用也是按值传递

`User b = a` 复制引用，不复制对象。方法参数接收引用值的副本：修改对象字段可能被调用方观察到，给形参重新赋 null 不会改变调用方变量。final 引用不能重新赋值，但不自动保证对象不可变、非空或线程安全。

## 参考资料

- [Java 17 JLS：类型、值与变量](https://docs.oracle.com/javase/specs/jls/se17/html/jls-4.html)
- [BigDecimal API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/math/BigDecimal.html)

## 模拟面试问题

1. 【基础】int 的范围是什么？为什么 long 接收 int 乘法结果仍可能溢出？
2. 【基础】成员字段和局部变量的默认值规则有什么区别？
3. 【应用】订单金额为什么通常不用 double？
4. 【应用】方法内重新赋值引用参数，调用方的变量会变化吗？
5. 【深入】BigDecimal 的 equals 与 compareTo 为什么可能得出不同结论？

