# Tomcat 与 Spring Boot 请求处理：学习笔记

> 整理范围：本次聊天中关于 Tomcat 是什么、有什么作用、与 Spring Boot/Spring MVC/Nginx 的关系，以及 Java Web 请求处理链路的讲解。

## 1. Tomcat 是什么

**Apache Tomcat 是 Java Web 服务器，同时也是 Servlet 容器。**

它的核心用途是接收客户端 HTTP 请求、将请求交给 Java Web 应用处理，再将响应返回客户端。

要区分两个概念：

- **Web 服务器**：提供 HTTP 服务，接收请求、返回响应。
- **Servlet 容器**：提供 Servlet 运行环境，管理 Servlet 生命周期并调用 Servlet 处理请求。

Tomcat 两者兼具，但并不替你编写登录、下单、查询订单等业务逻辑。

## 2. 一个请求经过哪些部分

以用户查询订单为例：

```text
浏览器 / App
    │ GET /orders/1
    ▼
Tomcat（例如监听 8080 端口）
    │ 接收连接、解析 HTTP 请求
    ▼
Spring MVC：DispatcherServlet
    │ 根据映射关系找到 Controller
    ▼
Controller（接收并校验请求参数）
    ▼
Service（处理业务逻辑）
    ▼
Mapper / Repository（数据库访问）
    ▼
MySQL
    │ 查询结果逐层返回
    ▼
Controller → Spring MVC → Tomcat
    │ HTTP 响应（常见为 JSON）
    ▼
浏览器 / App
```

**分工要点**：Tomcat 负责 HTTP 通信以及 Servlet 运行环境；Spring MVC 负责 Web 请求分发、参数绑定、结果处理；业务代码决定请求要做什么。

> 补充理解：Tomcat 通常先把请求交给 Spring MVC 的 `DispatcherServlet`，后者再找到具体的 Controller 方法；并不是 Tomcat 自己根据 `@GetMapping` 直接找到 Controller。

## 3. Spring Boot、Spring MVC、Tomcat 的关系

| 组件 | 核心职责 | 典型表现 |
|---|---|---|
| Tomcat | 处理 HTTP 连接、提供 Servlet 运行环境 | 监听端口、接收请求、调用 Servlet |
| Spring MVC | 实现 MVC Web 请求处理框架 | 路由、参数绑定、Controller 调用、响应转换 |
| Spring Boot | 自动配置、整合依赖、简化启动和部署 | 自动配置并启动内嵌 Web 服务器 |

在常见的 **Spring Boot + Spring MVC（Servlet 栈）** 项目中，默认使用内嵌 Tomcat，因此不必另行下载、手工部署外置 Tomcat。

但要注意：Spring Boot 并不只能使用 Tomcat，也可配置其他受支持的服务器；WebFlux 等非 Servlet 技术栈的默认运行方式也有所不同。

## 4. 代码实例：为什么没写 Socket 也能响应请求？

```java
@RestController
public class HelloController {

    @GetMapping("/hello")
    public String hello() {
        return "Hello World";
    }
}
```

访问：

```text
http://localhost:8080/hello
```

通常会得到响应体：

```text
Hello World
```

逐行解释：

1. `@RestController`：声明这是一个处理 Web 请求的控制器，并使返回值通常直接写入 HTTP 响应体。
2. `@GetMapping("/hello")`：将 `GET /hello` 映射到下面的方法。
3. `hello()`：普通 Java 方法，返回字符串。
4. 浏览器访问 `localhost:8080`：请求被内嵌 Tomcat 接收，再通过 Spring MVC 找到 `hello()`。

**关键理解**：开发者没有手写 `ServerSocket`、HTTP 报文解析和请求分发，因为这些基础工作由 Tomcat 与 Spring MVC 等框架完成。

## 5. Tomcat 为什么需要端口？

一台机器可以运行多个网络服务。监听端口用于区分请求应该交给哪个服务。例如：

- `localhost`：本机地址。
- `8080`：示例中 Tomcat 所监听的端口。
- `http://localhost:8080/hello`：访问本机 8080 端口上的 HTTP 服务，请求路径是 `/hello`。

在 Spring Boot 中可以通过 `application.yml` 配置：

```yaml
server:
  port: 8080
```

其中 `server` 是 Spring Boot 服务器相关配置的前缀，`port` 指定监听端口。这不是“创建服务器”的代码，而是告诉应用启动时使用哪个端口。

## 6. Servlet 与 Servlet 容器

**Servlet**：Java Web 技术规范定义的服务器端组件接口，用于处理请求和生成响应。

**Servlet 容器**：负责创建、初始化、调用、销毁 Servlet，并提供必要的运行环境。Tomcat 就是常见 Servlet 容器。

在 Spring MVC 中，核心入口 `DispatcherServlet` 本身就是 Servlet。通常由它把请求进一步分发给 Controller。

可类比：

- Tomcat：负责接待和管理入口。
- `DispatcherServlet`：负责根据请求找到对应处理者。
- Controller/Service：真正执行应用功能。

这个比喻仅用于理解分工，不代表它们分别运行在独立进程中。内嵌 Tomcat 与 Spring 应用通常在同一个 JVM 进程内。

## 7. Tomcat 与 Nginx 区别

| 对比 | Tomcat | Nginx |
|---|---|---|
| 主要定位 | Java Web 服务器、Servlet 容器 | Web 服务器、反向代理服务器 |
| 直接运行 Java Servlet | 可以 | 不可以 |
| 处理静态文件 | 可以 | 通常更适合承担大量静态资源服务 |
| 常见部署位置 | Java 应用所在服务 | 应用前方统一接入层 |
| 必须同时使用吗 | 不必须 | 不必须 |

一种常见部署结构是：

```text
用户 → Nginx（80/443）→ Tomcat（8080）→ Spring MVC → 业务逻辑
```

**注意**：Nginx 是可选的，不是所有 Java Web 项目都必须增加的一层。学习和简单部署时可直接访问 Spring Boot 内嵌 Tomcat。

## 8. 常见误区

**误区一：Tomcat 就是 Spring Boot。**

不是。Tomcat 负责服务器与 Servlet 容器能力；Spring Boot 用于配置、组织和启动 Spring 应用。

**误区二：Tomcat 自己负责数据库查询。**

不是。Tomcat 转交请求，数据库访问通常由业务代码通过 Mapper、Repository、JDBC 等完成。

**误区三：加了 Nginx 才算有服务器。**

不是。单独运行的 Tomcat 也能提供 HTTP 服务。

**误区四：使用 Spring Boot 就完全不需要 Servlet。**

对于 Spring MVC Servlet 技术栈，`DispatcherServlet` 仍是核心基础设施，只是多数细节已由框架封装。

**误区五：Tomcat 等于一个独立的电脑或操作系统。**

不是。它是可以运行在计算机中的软件；内嵌模式下通常和 Spring Boot 应用一起运行在同一个 Java 进程中。

## 9. Java 后端学习优先级

当前阶段建议先掌握：

1. HTTP 请求与响应的基本构成：方法、URL、状态码、请求体、响应体。
2. Tomcat 端口监听及基本职责。
3. Servlet 与 Servlet 容器的关系。
4. `DispatcherServlet`、Controller 和 Service 的调用链。
5. Spring Boot 如何自动配置和启动内嵌 Tomcat。
6. Tomcat 请求处理线程池的基本作用。

目前不必优先深挖 Tomcat 源码、Catalina 内部架构及复杂调优。

## 10. 复习自测

**Q1：为什么没有手写 Socket，Spring Boot 项目仍可以处理浏览器请求？**

因为内嵌 Tomcat 实现了底层网络收发与 HTTP 处理，Spring MVC 实现 Web 请求路由等机制。

**Q2：Tomcat 接收到 `GET /hello` 后，是它直接调用 `hello()` 吗？**

不准确。一般先进入 Spring MVC 的 `DispatcherServlet`，再根据映射调用 Controller 方法。

**Q3：Servlet 容器中的“容器”指什么？**

指为 Servlet 提供生命周期管理和运行环境的基础设施，不是 Docker 那种容器。

**Q4：不用 Nginx，可以访问 Spring Boot 项目吗？**

可以。Tomcat 本身能够提供 HTTP 服务，前提是地址、端口、网络、防火墙等配置允许访问。

**Q5：Tomcat、Spring MVC、Spring Boot 怎么一句话区分？**

Tomcat 接收 HTTP 请求并运行 Servlet；Spring MVC 把请求路由到 Java 业务入口；Spring Boot 负责便捷配置、启动和整合。

## 11. 自检结论

- 已区分 **Tomcat / Servlet / Spring MVC / Spring Boot / Nginx** 的角色。
- 示例请求链路与注解用法符合常见的 Spring Boot + Spring MVC 场景。
- 已补充两处关键限定：Tomcat **通常先调用 `DispatcherServlet`**，而且 **Spring Boot 并非必须使用 Tomcat**。
- 本笔记集中整理当前聊天实际讨论的主题；不是对所有 Spring 底层原理的全面教程。
