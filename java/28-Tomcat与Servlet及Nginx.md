# 28 · Tomcat、Servlet 与 Nginx

> 一句话概括：Tomcat 是"HTTP 服务器 + Servlet 容器"二合一的 Java Web 运行环境，Servlet 是规范、Tomcat 是实现；Nginx 则是放在它前面的可选代理层。

## 一、Tomcat 是什么

**Apache Tomcat = Web 服务器 + Servlet 容器**：

- Web 服务器：提供 HTTP 服务，接收请求、返回响应。
- Servlet 容器：管理 Servlet 生命周期并调用它处理请求。

Tomcat **不替你写业务**（登录、下单都是你的代码）。为什么没写 Socket 也能响应请求？因为内嵌 Tomcat 完成了底层网络收发与 HTTP 解析，Spring MVC 完成路由分发——你写的只是业务方法。

**端口**：一台机器跑多个网络服务，端口区分请求交给谁。`http://localhost:8080/hello` = 本机 8080 端口的 HTTP 服务、路径 `/hello`。Spring Boot 里 `server.port` 只是改监听端口，不是"创建服务器"。

## 二、Servlet 与 Servlet 容器

Servlet 是 Java Web **规范及 API**；开发者实现规定的行为，容器按规则管理调用：

```java
@WebServlet("/hello")
public class HelloServlet extends HttpServlet {
    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws IOException {
        resp.setContentType("text/plain;charset=UTF-8");
        resp.getWriter().write("Hello World");
    }
}
```

生命周期：加载/创建 → `init()` → 多次 `service()` → `destroy()`；**通常不是每次请求新建一个 Servlet**。有了 Servlet 为什么还要 Spring MVC？原生 Servlet 免去了手写 HTTP 服务器，但路径匹配、参数转换、JSON 响应仍繁琐——Spring MVC 在其上再封装一层（`DispatcherServlet` 本身就是个 Servlet）。

## 三、三个"容器"不是一回事

| 容器 | 管理对象 | 提供服务 |
|---|---|---|
| Servlet 容器（Tomcat） | Servlet、Filter | 生命周期、请求调度、Web 运行环境 |
| Spring IoC 容器 | Bean（Controller/Service） | 创建、注入、生命周期 |
| Docker 容器 | 应用进程 | OS 级隔离 |

"服务器"一词多义：硬件主机 / 服务器程序（Tomcat、Nginx、MySQL）/ 服务端应用。"Nginx + Spring Boot 是不是两个服务器"——从**服务进程**角度算两个，但常跑在同一台机器甚至同一台机器的同一 JVM 里。

## 四、Tomcat 与 Nginx

| 对比 | Tomcat | Nginx |
|---|---|---|
| 定位 | Java Web 服务器 + Servlet 容器 | Web 服务器 / 反向代理 |
| 跑 Java Servlet | 可以 | 不可以 |
| 静态资源 | 可以 | 更适合大量静态资源 |
| 常见位置 | Java 应用侧 | 应用前方的统一接入层 |

```text
用户 → Nginx（80/443）┬─ 静态文件直接返回
                      └─ /api/... → Tomcat（8080）→ Spring MVC
```

Nginx 四大用途：**反向代理**（用户只见到 Nginx，由它转发后端）、**负载均衡**（多实例分流量）、**静态资源服务**、**HTTPS 入口**（443 监听 + SSL 证书）。**Nginx 可选**：学习阶段直接访问内嵌 Tomcat 的 `localhost:8080` 即可；它也不限于代理 Java，可代理 Python/Go/Node。

## 📌 易错点

- "Tomcat = Spring Boot"——Tomcat 管服务器和容器，Boot 管配置组织启动。
- "Tomcat 自己查数据库"——它只转交请求，数据访问是业务代码的事。
- "加了 Nginx 才算有服务器"——单独 Tomcat 就能提供 HTTP 服务。
- "用 Spring Boot 就不需要 Servlet"——Servlet 栈里 `DispatcherServlet` 仍是核心，只是被封装。
- "Servlet = Tomcat"——Servlet 是规范，Tomcat 是其实现之一。

## 🧠 速记口诀

```text
Servlet 是规范，Tomcat 是容器也是 HTTP 服务器
生命周期 init→service→destroy，一个 Servlet 服务多次请求
三种容器三个概念，别拿 Docker 套 Servlet
Nginx 站门口：反代、均衡、静态、HTTPS——可选，不是必需
```

## ✅ 自测问题

1. 为什么没有手写 Socket，Spring Boot 项目仍能处理浏览器请求？
2. Tomcat 收到 `GET /hello` 后直接调用 `hello()` 吗？
3. Servlet 容器、IoC 容器、Docker 容器为什么不能混为一谈？
4. 不用 Nginx 能访问 Spring Boot 项目吗？Nginx 解决什么问题？
5. Servlet 生命周期是什么？"每次请求新建一个 Servlet"对吗？

> 来源：原始《Tomcat 与 Spring Boot 请求处理：学习笔记》(Tomcat与SpringBoot请求处理_学习笔记.md)、《Java 后端学习笔记：从 HTTP 请求到 Spring Boot》(Java后端学习笔记_从HTTP请求到SpringBoot.md §1-5)、《本次聊天学习笔记：Nginx 与 PDF 解密/扁平化》(本次聊天学习笔记_Nginx与PDF处理.md §1，PDF 部分为项目排障留痕，见根 README)。
