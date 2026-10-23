---
id: java-reflection
title: "JavaBean、record 与反射"
description: "理解框架读取元数据的能力，同时识别模块访问限制。"
course: java-basics
order: 10
date: 2026-10-23
tags: [Java, 后端]
---

# JavaBean、record 与反射

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 约定与语言能力

JavaBean 是围绕属性访问等形成的组件约定，不是关键字，也不等同于 Spring Bean。getter/setter、无参构造是常见数据绑定方式，但 Spring 管理对象不要求每个类都按这一模板编写。Java 17 的 record 适合简洁的不可变数据载体，其访问器是 name()，不是 getName()；引用组件仍可能指向可变对象。

反射通过 Class、Constructor、Method、Field 在运行时查询和操作类型。Class.forName(name) 通常还会触发初始化，Type.class 与已存在对象的 getClass 有不同获取方式。

## 一个可运行的最小例子

```java
import java.lang.reflect.Method;
public class ReflectDemo {
    public record User(String name) {}
    public static void main(String[] args) throws Exception {
        User user = new User("Roger");
        Method accessor = User.class.getMethod("name");
        System.out.println(accessor.invoke(user));
    }
}
```

getMethods 返回公开方法并包含继承成员；getDeclaredMethods 返回本类声明方法，不自动包含父类，也不代表均可访问。Method.invoke 的目标方法异常包装在 InvocationTargetException 中，要查看 cause 区分业务异常与反射调用错误。

## 实际开发：JSON 转换与框架

序列化框架可以检查属性、注解、构造器，生成或缓存访问逻辑。编译期注解处理与运行时反射不同，Lombok 等生成代码不应统称为反射。Spring 也可能通过工厂方法、Supplier、预处理元数据创建对象，并非“只靠反射 new”。

setAccessible(true) 不是任意绕过封装的万能钥匙。Java 17 模块强封装可能拒绝深反射；trySetAccessible 可返回失败。不要把打开整个 JDK 模块作为日常业务默认配置。

## 性能与故障边界

原稿“反射固定慢 10—100 倍”缺少可复现实验，删除这个断言。开销依调用方式、缓存、JDK 和 JIT 而变化；先避免热路径反复查找成员，再按实际需求测量。

ClassNotFoundException 通常是显式按名加载未找到；NoClassDefFoundError 可能来自运行时依赖缺失，也可能与此前初始化失败相关，不能只定义为“类初始化失败”。

## 参考资料

- [java.lang.reflect API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/reflect/package-summary.html)
- [AccessibleObject API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/reflect/AccessibleObject.html)
- [JLS 17：record 类](https://docs.oracle.com/javase/specs/jls/se17/html/jls-8.html#jls-8.10)

## 模拟面试问题

1. 【基础】JavaBean 和 Spring Bean 是同一概念吗？
2. 【应用】getMethods 与 getDeclaredMethods 如何选择？
3. 【应用】反射调用业务方法失败后应检查哪一层异常？
4. 【深入】setAccessible 在 Java 17 为什么可能失败？
5. 【深入】record 为什么不是所有层次的深度不可变对象？

