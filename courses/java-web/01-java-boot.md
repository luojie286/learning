---
id: java-boot
title: "Spring Boot 3：启动、Starter 与自动配置"
description: "区分应用装配、服务器启动和运行时请求三个阶段。"
course: java-web
order: 1
date: 2026-10-23
tags: [Java, Spring]
---

# Spring Boot 3：启动、Starter 与自动配置

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 四个技术各司其职

Spring Framework 提供容器、事务等能力；Spring MVC 负责 Servlet 栈请求处理；Tomcat 是 HTTP 服务器和 Servlet 容器；Boot 简化依赖、配置和启动。Boot 可以运行非 Web 应用，也可以采用别的 Web 技术栈，不是所有 Boot 项目都有 Tomcat。

本课程锁定 Java 17+、Boot 3.x、Spring Framework 6.x，Servlet 与 Validation 使用 jakarta 包；JDBC 等 Java SE API 中的 javax.sql 不因此改名。不要全局替换所有 javax，也不要把 Boot 4 的 starter 命名混进 Boot 3 教程。

## 最小启动结构

```java
package com.example.shop;
@SpringBootApplication
public class ShopApplication {
    public static void main(String[] args) {
        SpringApplication.run(ShopApplication.class, args);
    }
}
```

注解组合 @SpringBootConfiguration、@EnableAutoConfiguration 与 @ComponentScan。启动类放根包，业务组件放其子包。扫描是注册候选定义的重要机制，不是先遍历所有对象再给它们贴 Bean 标记。

starter-web 组织相关依赖，典型 Servlet Web 项目默认引入嵌入式 Tomcat。它不是代码生成器；只加依赖也不会自动生成订单 Controller。

## 自动配置有条件

类路径存在某能力、属性满足、用户未提供某 Bean 时，自动配置可能生效。定义自有 DataSource 后相关默认配置会让步；手写配置并非总是覆盖所有自动配置，需看具体条件和顺序。

启动时读取环境配置、选择应用上下文、注册定义、刷新容器并启动适用服务器。真实启动不是严格三行伪代码，后置处理器与服务器生命周期相互配合；请求到来时通常使用已创建的共享组件，而非重新跑一次全部启动流程。

## 实际开发：配置与诊断

```yaml
server:
  port: 8081
spring:
  datasource:
    url: ${DB_URL}
    username: ${DB_USERNAME}
    password: ${DB_PASSWORD}
```

环境变量提供敏感值，不提交生产口令。修改配置需区分默认值、Profile、外部文件与命令行优先级。排查“为什么没有某 Bean”可使用 --debug 的条件报告，先看缺类、条件与扫描范围，再盲目添加注解。

循环依赖优先改职责；开启允许循环引用也无法解决所有构造器循环。Boot 管理兼容依赖版本，手工覆盖版本前先确认兼容与安全修复依据。

## 配置绑定与源码入口

`@Value("${server.port:8080}")` 提供占位符缺省值，但读取的是配置值；`server.port=0` 时实际监听端口需在服务器启动后通过 WebServer API 或事件获得。不能假设框架的内部默认端口必然已作为 Environment 属性存在。

成组业务参数可用 `@ConfigurationProperties(prefix = "shop.order")`，例如将订单超时绑定到 record；还需 `@ConfigurationPropertiesScan`、`@EnableConfigurationProperties` 或显式 Bean 注册，注解本身不保证任意类都自动可注入。参数建议用 `Duration` 等明确单位，避免把 30 到底是秒还是分钟留给调用方猜。

Boot 3 的自定义自动配置候选通常登记在 `META-INF/spring/org.springframework.boot.autoconfigure.AutoConfiguration.imports`。这与扫描应用的 `@ComponentScan` 是不同发现路径。追踪机制可从 `SpringApplication.run`、`AbstractApplicationContext.refresh`、配置类解析和 `AutoConfigurationImportSelector` 入手，先在自己的 Controller/Service 断点观察调用栈，再逐步进入框架。

Actuator 可帮助查看健康、映射与条件，但端点的可用、启用和 HTTP 暴露是不同配置。不要因本地能看到端点就把 beans、env、conditions 等未经认证暴露给公网。

## 参考资料

- [Boot 3.5 自动配置](https://docs.spring.io/spring-boot/3.5/reference/using/auto-configuration.html)
- [Boot 3.5 应用启动注解](https://docs.spring.io/spring-boot/3.5/reference/using/using-the-springbootapplication-annotation.html)
- [Boot 3.5 外部配置](https://docs.spring.io/spring-boot/3.5/reference/features/external-config.html)

## 模拟面试问题

1. 【基础】Boot、MVC、Tomcat 各解决什么问题？
2. 【应用】加了 @Service 却无法注入，应检查什么？
3. 【应用】如何查明某个自动配置为什么没有生效？
4. 【深入】为什么自定义 DataSource 能影响自动配置？
5. 【深入】Boot 3 升级中的 jakarta 迁移是否意味着替换全部 javax？

