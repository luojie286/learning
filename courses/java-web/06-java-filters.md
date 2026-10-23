---
id: java-filters
title: "Filter、Interceptor 与部署接入"
description: "理解请求生命周期和 Nginx 边界，正确放置日志与清理。"
course: java-web
order: 6
date: 2026-10-23
tags: [Java, Spring]
---

# Filter、Interceptor 与部署接入

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 不同扩展点面向不同对象

Servlet Filter 包围 Servlet 链，可处理通用请求与安全；MVC Interceptor 围绕已选择的 Handler，可获取处理器信息；AOP 面向代理方法。Filter 可以通过合适注册方式使用 Spring Bean，不能说“Filter 访问不到容器”。

Interceptor 的 preHandle 返回 false 会中断后续 Handler，但不会自动形成 401。postHandle 面向正常 Handler 完成后的 MVC 模型，@ResponseBody 响应可能早已写出；要统一修改正文应考虑 ResponseBodyAdvice 等机制。

## 注册与清理契约

@Component 只是注册 Bean，不代表拦截全部请求；WebMvcConfigurer 需将实例加到指定映射中。多个拦截器时，afterCompletion 只对成功通过自身 preHandle 的相应拦截器按契约回调，不能假设任何失败路径都必然进入自己的清理方法。

```java
registry.addInterceptor(traceInterceptor)
    .addPathPatterns("/api/**")
    .excludePathPatterns("/api/health");
```

这是配置方法内部片段。真正的认证层优先使用 Spring Security FilterChain，MVC 路径匹配差异可能让手写拦截器留下安全空洞。教学 JWT 拦截器只用来理解流程，不作为完整安全实现。

## 实际开发：代理后的真实请求

用户可先到 Nginx，再转发 Tomcat；Nginx 可做 TLS 接入、静态资源、反向代理和负载均衡，不必与 Java 同机器。独立 Nginx 一般是原生独立进程，原稿“常与 Java 同一 JVM”有误。

Tomcat 已可直接提供 HTTP 服务，Nginx 是按部署需求加入，不是 Spring Boot 必需组件。代理终止 TLS 后，后端可能看到 HTTP 和代理地址；只有来自可信代理的转发头才能参与构造真实协议、客户端地址与重定向，不能盲信用户传的 X-Forwarded-*。

## Servlet 生命周期与并发

Servlet 通常初始化一次并服务多次请求，容器可能并发调用。Spring Controller、Interceptor 和 Filter 也常共享，不能把当前请求数据写进普通实例字段。异步分派、错误分派和线程切换会改变回调行为；日志与 ThreadLocal 清理必须明确在哪条执行路径生效。

排查 502 先看代理连接上游是否成功，排查应用 500 再看业务日志。代理超时不等于后端操作已经取消，涉及重复提交时仍需幂等设计。

## 参考资料

- [Spring 6.2 拦截器](https://docs.spring.io/spring-framework/reference/6.2/web/webmvc/mvc-config/interceptors.html)
- [HandlerInterceptor 契约](https://docs.spring.io/spring-framework/docs/6.2.x/javadoc-api/org/springframework/web/servlet/HandlerInterceptor.html)
- [Nginx 反向代理模块](https://nginx.org/en/docs/http/ngx_http_proxy_module.html)
- [Jakarta Servlet 6.0](https://jakarta.ee/specifications/servlet/6.0/)

## 模拟面试问题

1. 【基础】Filter、Interceptor、AOP 各位于哪一层？
2. 【应用】preHandle 返回 false 后如何形成真正的错误响应？
3. 【应用】为什么不能靠 postHandle 一律修改 JSON？
4. 【深入】代理超时与后端任务取消为什么是两件事？
5. 【深入】为什么客户端可伪造的转发头不能直接信任？

