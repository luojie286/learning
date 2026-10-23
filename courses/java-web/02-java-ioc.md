---
id: java-ioc
title: "IoC、DI、Bean 作用域与生命周期"
description: "从普通构造器注入到容器装配，理解共享对象的真实边界。"
course: java-web
order: 2
date: 2026-10-23
tags: [Java, Spring]
---

# IoC、DI、Bean 作用域与生命周期

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 控制反转与依赖注入

IoC 强调对象组装与生命周期控制转交容器，DI 通过构造器、setter 等把依赖交给对象。反射只是部分实现手段，DI 不定义为“反射改 private 字段”。Bean 是被容器管理的对象；BeanDefinition 是如何创建和管理它的元数据，不是同一个东西。

```java
@Service
public class OrderService {
    private final OrderRepository orders;
    public OrderService(OrderRepository orders) {
        this.orders = Objects.requireNonNull(orders);
    }
}
```

OrderRepository 是应用契约，需提供实现 Bean；单构造器通常无需 @Autowired。构造器参数先接收依赖引用，再用 Java 赋值初始化字段。final 不自动拒绝 null，这里 requireNonNull 明确表达前置条件。

## 注册与消歧

@Component 与 @Service 等参与组件扫描；第三方类可通过配置类中的 @Bean 工厂方法注册。@Bean 不要求任何场景都与 @Configuration 同时出现，但所在类必须被 Spring 配置机制处理。

同一类型多个候选时用 @Qualifier、@Primary 等消歧。按接口注入需要可赋值的实现或代理，并不是接口自己实例化。手工 new 带 @Service 的类不会自动把这个对象纳入已有容器。

## 实际开发：共享密码编码器

@Configuration 定义一个 PasswordEncoder Bean，UserService 和 AdminService 注入时可共享它。singleton 的边界是每容器、每 Bean 定义，既不保证同一类整个 JVM 只有一个对象，也不保证所有字段线程安全。

Service 不要保存 currentUserId 之类请求级可变字段；并发请求会互相覆盖。prototype 注入 singleton 时通常只在创建时注入一次，要每次获得新实例需要 ObjectProvider 或其他显式机制。request/session 作用域依赖 Web 环境。

## 生命周期不是一行 new

注册定义 → 实例化 → 属性填充与依赖解析 → 初始化回调与后置处理 → 提供对象，容器关闭执行适用销毁回调。@PostConstruct 通常由相关后处理器参与初始化前处理，AOP 代理也可能在后处理阶段出现，不能把某个细节说成所有 Bean 固定路径。

ApplicationContext 在 BeanFactory 基础上提供事件、资源等能力。业务代码优先依赖注入，避免到处 getBean 隐藏依赖。构造器循环依赖往往提示职责纠缠，应先拆分业务关系。

## 参考资料

- [Spring 6.2 依赖注入](https://docs.spring.io/spring-framework/reference/6.2/core/beans/dependencies/factory-collaborators.html)
- [Bean 作用域](https://docs.spring.io/spring-framework/reference/6.2/core/beans/factory-scopes.html)
- [容器扩展点](https://docs.spring.io/spring-framework/reference/6.2/core/beans/factory-extension.html)

## 模拟面试问题

1. 【基础】BeanDefinition 与 Bean 有何区别？
2. 【应用】构造器注入为什么方便单元测试？
3. 【应用】两个 Service 注入同一 singleton Bean 时共享什么？
4. 【深入】prototype 注入 singleton 后每次调用会自动产生新对象吗？
5. 【深入】为什么不应把当前用户保存到单例 Service 字段？

