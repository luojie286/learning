# Java 网络编程学习笔记

> 主题：Java 网络编程入门（TCP / UDP / Socket / IO / 多线程）  
> 学习目标：能够写出一个 TCP 客户端和服务端，理解阻塞、消息边界，并逐步实现多人聊天室。

## 一、整体认识

**网络编程**：让不同进程（可能运行在不同计算机上）通过网络传输数据。

通信时需要关注三个关键要素：

| 要素 | 作用 | 示例 |
| --- | --- | --- |
| IP 地址 | 标识通信目标主机/网络接口 | `192.168.1.20` |
| 端口 | 标识目标主机上的网络服务端点 | `8888` |
| 协议 | 规定数据如何传输与解释 | TCP、UDP、HTTP |

示例：`192.168.1.20:8888` 表示访问该 IP 地址上使用 8888 端口的服务。

- `127.0.0.1`：IPv4 回环地址，指当前机器，适合本机联调。
- 一台计算机可以同时运行多个服务，通过不同端口进行区分，例如 MySQL 常见端口 `3306`、Web 服务常见端口 `8080`。
- **Socket（套接字）**：应用程序使用操作系统网络功能的接口/通信端点，不等于 TCP 协议本身。

## 二、TCP 与 UDP

| 对比项 | TCP | UDP |
| --- | --- | --- |
| 建立连接 | 需要 | 不需要像 TCP 那样建立连接 |
| 可靠性 | 提供可靠、有序的字节流传输（连接正常工作时） | 不保证送达、不保证顺序 |
| 数据形式 | 字节流，**不保留消息边界** | 数据报，保留数据报边界 |
| Java 常用 API | `Socket`、`ServerSocket` | `DatagramSocket`、`DatagramPacket` |
| 常见用途 | 普通 HTTP/1.1、数据库连接、传统聊天室 | 实时通信的部分场景、DNS 查询等 |

> 注意：TCP 的“可靠”不代表连接永远不会断，也不代表业务消息一定已被对方成功处理。应用仍需考虑超时、断线和错误处理。

现阶段 Java 后端入门，**优先掌握 TCP**。

## 三、Socket 与 ServerSocket

服务端通常先创建一个监听套接字：

```java
ServerSocket serverSocket = new ServerSocket(8888);
```

它监听本机的 8888 端口，等待客户端连接：

```java
Socket socket = serverSocket.accept();
```

`accept()` 会**阻塞**，直到有连接到来（或者发生错误、监听套接字被关闭等）。返回的 `Socket` 用于与这个客户端通信。

客户端通过目标 IP 和端口发起连接：

```java
Socket socket = new Socket("127.0.0.1", 8888);
```

**区分：**

- `ServerSocket`：监听并接受连接。
- `Socket`：代表一个已连接的 TCP 通信端点，负责读写数据。

```text
                 服务端
            ServerSocket:8888
                    |
                accept()
                    |
                  Socket  <---------->  Socket
                  服务端               客户端
```

## 四、Socket 与 Java IO

使用 `Socket` 获取输入/输出流：

```java
InputStream in = socket.getInputStream();
OutputStream out = socket.getOutputStream();
```

- `getInputStream()`：**读取对端发来的字节**。
- `getOutputStream()`：**向对端发送字节**。

每个 TCP `Socket` 都可以有两个方向：双方都能读，也都能写，所以 TCP 通信是全双工的。

处理文本时可以使用字符流包装：

```java
BufferedReader reader = new BufferedReader(
        new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8)
);

PrintWriter writer = new PrintWriter(
        new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8),
        true
);
```

需要导入 `java.io.*` 与 `java.nio.charset.StandardCharsets`。`PrintWriter` 的 `autoFlush=true` 适用于调用 `println` / `printf` / `format` 等操作时的自动刷新；不是所有 `write` 都自动刷新。

## 五、最简单的 TCP 通信实战

### 1. 服务端 Server.java

```java
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class Server {
    public static void main(String[] args) throws Exception {
        try (ServerSocket server = new ServerSocket(8888)) {
            System.out.println("服务端启动，等待连接……");

            try (Socket client = server.accept();
                 BufferedReader reader = new BufferedReader(
                         new InputStreamReader(
                                 client.getInputStream(), StandardCharsets.UTF_8))) {

                System.out.println("客户端连接成功");
                String message = reader.readLine();
                System.out.println("客户端：" + message);
            }
        }
    }
}
```

### 2. 客户端 Client.java

```java
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class Client {
    public static void main(String[] args) throws Exception {
        try (Socket socket = new Socket("127.0.0.1", 8888);
             PrintWriter writer = new PrintWriter(
                     new OutputStreamWriter(
                             socket.getOutputStream(), StandardCharsets.UTF_8), true)) {
            writer.println("hello server");
        }
    }
}
```

**运行顺序**：先启动 `Server`，再启动 `Client`。服务端预期打印：

```text
服务端启动，等待连接……
客户端连接成功
客户端：hello server
```

这里 `println` 会在文本后写入换行符，因此服务端的 `readLine()` 能读到一整行。`try-with-resources` 用于自动关闭连接和流。

## 六、为什么 readLine() 会“卡住”

`BufferedReader.readLine()` 在**尚未收到一行完整数据**时会等待。完整一行通常需要换行符，或在到达流结尾时返回剩余字符；若到达流结尾且没有剩余字符则返回 `null`。

```text
服务端：reader.readLine()
           ↓
         阻塞等待
           ↓
客户端：writer.println("hello")
           ↓
       接收换行与数据
           ↓
服务端继续执行
```

因此如果发送端只写了 `hello`、没写换行、也没有结束输出，接收端的 `readLine()` 可能会一直等待。

这一点与 `BlockingQueue.take()` 的阻塞思想类似，但两者处理的数据来源和语义不同。

## 七、多客户端与多线程

一个服务器通常需要服务多个客户端。经典的**阻塞 IO（BIO）+ 每连接一个处理线程**的思路：

```text
ServerSocket.accept()
         |
   接收新 Socket
         |
   创建/分配处理任务
         |
  +------+------+------+
  |             |      |
客户端 A       客户端 B  客户端 C
处理任务        处理任务  处理任务
```

伪代码：

```java
while (true) {
    Socket socket = serverSocket.accept();
    new Thread(() -> handleClient(socket)).start();
}
```

这段只展示思路，`handleClient` 需要自己实现。**生产环境不推荐无限制为每个连接创建新线程**，否则客户端过多会产生资源压力；后续可学习有界线程池、连接数控制和 NIO/Netty 等方案。

一个简易聊天室的基本模型：

1. 服务端使用 `accept()` 接收客户端连接。
2. 为每个客户端安排独立的读取任务。
3. 维护在线客户端连接集合。
4. 某个客户端发送消息后，服务端转发给其他在线客户端。
5. 客户端断开连接时清理资源与在线状态。

> 并发广播时，在线连接集合要考虑线程安全（例如适当使用并发集合），也要考虑写入失败与慢客户端问题。

## 八、TCP 的字节流特性与“粘包/拆包”

**TCP 只保证按序提供字节流，不保证一次发送对应一次接收。**

发送端调用：

```text
发送 "hello"
发送 "world"
```

接收端可能读到：

```text
"helloworld"
```

或者多次读取：

```text
"hel" → "lowo" → "rld"
```

因此应用必须自行定义**消息边界（framing）**。常见方式：

- **分隔符协议**：例如一条消息以 `\n` 结束，搭配 `readLine()`。
- **长度前缀协议**：先传消息长度，再读取指定长度的数据。
- **固定长度协议**：每条消息长度固定（适用面较窄）。

聊天室入门建议采用**文本行协议**：

```text
LOGIN alice\n
CHAT 你好\n
ONLINE\n
```

注意：如果消息本身允许换行，需要转义、编码或者改用长度前缀等协议。长度前缀必须约定长度字段大小、字节序、编码方式，并限制最大消息长度。

## 九、Socket、TCP、HTTP 与 Spring Boot 的关系

大致可以这样理解典型的 HTTP/1.1 服务链路：

```text
浏览器
  ↓ HTTP 请求
TCP 连接
  ↓
操作系统网络栈 / Socket
  ↓
Tomcat 等 Web 容器
  ↓
Spring MVC
  ↓
Controller
```

- **TCP**：传输层协议，负责可靠、有序的字节流传输。
- **Socket**：Java 程序调用网络功能的一种接口。
- **HTTP**：应用层协议，规定请求与响应语义。
- **Spring Boot**：开发应用的框架，可通过嵌入式 Web 容器处理 HTTP 请求。

补充：上述是典型 HTTP/1.1 或 HTTP/2 over TCP 路径；**HTTP/3 使用 QUIC（基于 UDP）**，所以不能笼统地说所有 HTTP 都运行在 TCP 上。

## 十、BIO、NIO 与 Netty

| 技术 | 核心思路 | 当前学习要求 |
| --- | --- | --- |
| BIO（传统阻塞 IO） | `accept()`、`read()` 等调用可阻塞当前线程 | **能独立写代码** |
| NIO | `Channel`、`Buffer`、`Selector`，支持非阻塞与多路复用 | 了解概念 |
| Netty | 建立在 Java NIO 等机制之上的高性能网络应用框架 | 后续项目按需学习 |

注意：BIO 不等于“只能一个客户端”；BIO 同样能配合多线程处理多个客户端。NIO 也不等于“一定只有一个线程”，它提供了更灵活的 IO 模型。

## 十一、学习路线与练习任务

建议顺序：

1. **单向通信**：客户端向服务端发一行文本，服务端打印。
2. **双向通信**：客户端发出 `PING`，服务端回复 `PONG`。
3. **持续会话**：通过循环读取多条消息；理解 `null`、断线、异常与资源释放。
4. **多客户端**：通过多线程让多个客户端同时连接。
5. **最小聊天室**：实现 `LOGIN`、`CHAT`、`ONLINE` 等文本协议及消息广播。
6. **健壮性**：添加消息长度上限、连接超时/读超时、异常处理、线程安全的在线用户管理。
7. **进阶**：了解 NIO 的 `Buffer`、`Channel`、`Selector`，再考虑 Netty。

**本阶段重点：Socket + IO + 多线程的实际协作，不要急着堆 NIO/Netty 框架。**

## 十二、自测问题

1. IP 和端口分别负责什么？`127.0.0.1` 表示什么？
2. `ServerSocket` 和 `Socket` 的职责有什么区别？
3. `accept()` 与 `readLine()` 各在什么情况下阻塞？
4. `getInputStream()` 与 `getOutputStream()` 的数据方向分别是什么？
5. 为什么 `writer.print("hello")` 后，对方的 `readLine()` 可能不返回？
6. 为什么 TCP 要单独设计消息边界？
7. BIO 能否服务多个客户端？为什么需要考虑多线程/线程池？
8. HTTP、TCP、Socket、Spring MVC 处于什么层次？

---

**一句话总结**：Java TCP 网络编程就是通过 `ServerSocket` 监听连接、通过 `Socket` 建立通信端点、通过 IO 流收发字节，再结合自定义协议和并发处理实现多客户端服务。
