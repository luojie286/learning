---
id: java-packages
title: "包、导入与项目边界"
description: "理解全限定名、源根目录和编译依赖，避免把 import 当作加载。"
course: java-basics
order: 2
date: 2026-10-23
tags: [Java, 后端]
---

# 包、导入与项目边界

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 名字、目录与源根

包用于组织命名空间。`com.example.order.OrderService` 是全限定名，常规 Maven 项目把它放在 `src/main/java/com/example/order/OrderService.java`。源根是 src/main/java，而不是包目录本身。包与目录匹配是工具链的重要约定，不能把 Java 包简单定义成文件夹。

普通编译单元的顺序是 package（如有）、import、顶级声明。字段、构造器和方法没有必须按某个顺序出现的语法要求。通常 javac 要求 public 顶级类名与文件名匹配，一个文件至多一个 public 顶级类型；其他包级顶级类型可共存。

## import 不复制代码

```java
package com.example.order;
import java.util.List;
import static java.util.Objects.requireNonNull;

public class OrderIds {
    private final List<Long> ids;
    public OrderIds(List<Long> ids) {
        this.ids = List.copyOf(requireNonNull(ids));
    }
}
```

import 允许用简单名引用可访问类型。通配导入不递归导入子包，也不会让运行时把包内所有类都加载。静态导入只缩短静态成员名字；过多通配导入会让同名类型或方法含混。

java.lang 和同包类型通常无需导入，但不表示任何库都在类路径里。要使用第三方类型，需要先让构建工具解析依赖，再导入其名字。import 本身不会下载 jar。

## 实际开发：模块怎么分

小型订单项目可以按功能组织 order/controller、order/service、order/mapper，公共能力再单独抽取。包级可见性有助于限制内部实现；public 是对外 API 承诺，不能为了消除编译错误就把所有成员开放。

看到“找不到符号”先检查拼写、包、导入和依赖；运行时 ClassNotFoundException 再查打包与类路径。不要通过移动单个 class 文件来掩盖 Maven 源目录配置错误。

## 参考资料

- [JLS 17：包和模块](https://docs.oracle.com/javase/specs/jls/se17/html/jls-7.html)

## 模拟面试问题

1. 【基础】全限定名与源根目录分别是什么？
2. 【基础】import java.util.* 会导入子包吗？
3. 【应用】引入第三方 jar 和写 import 的职责有什么区别？
4. 【深入】包级可见性如何帮助保护内部实现？

