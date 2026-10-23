---
id: java-strings
title: "字符串、Unicode 与控制流"
description: "准确理解不可变字符串、编码边界与 Java 17 switch。"
course: java-basics
order: 3
date: 2026-10-23
tags: [Java, 后端]
---

# 字符串、Unicode 与控制流

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 不可变与相等

String 的值不可改变，replace、strip、substring 返回新字符串或符合契约的结果，不会修改原变量指向的内容。`==` 比较引用是否同一对象，equals 比较内容；常量池可能让部分引用相同，但不能依赖它做业务判断。可空输入可用 Objects.equals 或先判断 null。

StringBuilder 用于一个线程内逐步拼接，例如导出多行订单 CSV；不要为了两个字符串相加就一律替换成 Builder。StringBuffer 对方法做同步也不意味着多个调用组合成的整个流程自动原子。

## 代码单元不等于看到的字符

```java
String face = "😀";
System.out.println(face.length()); // 2 个 UTF-16 代码单元
System.out.println(face.codePointCount(0, face.length())); // 1 码点
System.out.println(face.getBytes(StandardCharsets.UTF_8).length); // 4 字节
```

码点数仍不必等于用户感知字符数，例如组合音标与复合 Emoji。用户名限制要先确定按字节、代码单元还是感知字符计数；数据库字段长度、前端字数和后端 @Size 不能假定完全一致。网络和文件读写显式使用 UTF-8，避免 Java 17 的平台默认字符集差异。

isEmpty 判断零长度，isBlank 判断空串或空白字符组成；调用它们前仍须处理 null。trim 与 strip 的空白规则不同。split 接收正则，按点号切分要转义；Java 字符串和正则分别解释一次反斜杠。

## 用控制流表达完整规则

```java
String label = switch (status) {
    case "NEW" -> "待支付";
    case "PAID" -> "已支付";
    default -> throw new IllegalArgumentException("未知状态");
};
```

Java 17 支持 switch 表达式与箭头分支；多语句分支返回值用 yield，箭头分支不发生传统冒号分支的贯穿。Java 17 中不能把后续版本的模式 switch 当作无需启用预览就可用的语法，null 也要显式在进入前处理。

条件校验采用提前返回可减少嵌套，但不要散落重复的业务规则。块注释不能嵌套；注释描述设计意图，不能代替对状态转换的约束与测试。

## 参考资料

- [String API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/String.html)
- [JLS 17：switch 语句](https://docs.oracle.com/javase/specs/jls/se17/html/jls-14.html#jls-14.11)

## 模拟面试问题

1. 【基础】String 的不可变和 final 引用有什么区别？
2. 【应用】为什么 Emoji 的 length 不一定是 1？
3. 【应用】导出十万行文本时如何选择拼接方式和编码？
4. 【深入】Java 17 的 switch 表达式怎样保证结果完整？

