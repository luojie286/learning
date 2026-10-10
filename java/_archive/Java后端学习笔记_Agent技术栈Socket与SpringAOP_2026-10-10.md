# Java 后端学习笔记：Agent 技术栈、Socket 网络编程与 Spring AOP

> 整理范围：本次聊天中的 Java 21、LangChain4j、MCP/JSON-RPC/stdio、Agent Harness、Maven、TCP Socket、bind/listen/accept、IO、HTTP 与 Spring AOP。
>
> 阅读建议：先看**知识结构总览**和**核心结论**，再结合代码复习。本文将之前讨论中容易混淆的 Java API 与操作系统底层 API 明确区分。

---

## 目录

1. [知识结构总览](#1-知识结构总览)
2. [Java 21、LangChain4j、MCP、JSON-RPC 与 stdio](#2-java-21langchain4jmcpjson-rpc-与-stdio)
3. [Agent 与 Harness](#3-agent-与-harness)
4. [Java/Maven 常用命令及对项目的影响](#4-javamaven-常用命令及对项目的影响)
5. [网络编程基础：进程、IP、端口、Socket](#5-网络编程基础进程ip端口socket)
6. [ServerSocket、Socket、bind、listen、accept](#6-serversocketsocketbindlistenaccept)
7. [Java TCP 服务端代码详解](#7-java-tcp-服务端代码详解)
8. [Java TCP 客户端代码详解](#8-java-tcp-客户端代码详解)
9. [输入流、输出流与字符编码](#9-输入流输出流与字符编码)
10. [HTTP 服务器与普通 TCP 服务器](#10-http-服务器与普通-tcp-服务器)
11. [Spring AOP](#11-spring-aop)
12. [常见误区速查](#12-常见误区速查)
13. [自测题与参考答案](#13-自测题与参考答案)
14. [学习优先级与自检](#14-学习优先级与自检)

---

## 1. 知识结构总览

```text
Java 后端 + Agent
├─ Java 21：编写并运行应用
├─ Maven：管理依赖、编译、测试、打包
├─ 网络编程
│  ├─ 进程/PID 与 IP/端口：不是同一概念
│  ├─ TCP：可靠、有序的双向字节流
│  ├─ ServerSocket：监听并接受新连接
│  ├─ Socket：已建立的连接，可双向收发
│  ├─ bind：指定 Socket 使用的本地地址
│  ├─ listen：进入 TCP 监听状态（底层概念）
│  ├─ accept：取得客户端连接
│  └─ InputStream / OutputStream：收/发字节
├─ HTTP：在传输之上规定请求/响应语义
├─ Spring / Spring Boot
│  └─ Spring AOP：公共逻辑与业务逻辑分离，通常通过代理实现
└─ AI Agent
   ├─ LangChain4j：Java LLM 应用开发框架
   ├─ MCP：工具/资源接入的标准化协议
   ├─ JSON-RPC：请求/响应消息格式
   ├─ stdio：进程间通信通道
   └─ Harness：模型调用、工具调度、上下文和执行循环等运行支撑
```

**贯穿全篇的核心原则：区分抽象层次。**

- Java `ServerSocket` 与操作系统底层 `socket()` 不是同一个 API。
- `bind()` 与 `listen()` 是不同概念，但 Java `ServerSocket` 构造方法和 `ServerSocket.bind()` 已封装监听设置。
- TCP 负责字节传输；业务程序负责解析字节、执行逻辑、生成响应。
- HTTP 是应用层协议，不是与 TCP 同级的替代方案。

---

## 2. Java 21、LangChain4j、MCP、JSON-RPC 与 stdio

| 名称 | 它是什么 | 在 Java Agent 中的作用 |
| --- | --- | --- |
| **Java 21** | Java 长期支持版本（LTS） | 编写 Agent、工具执行、文件操作、进程控制等 |
| **LangChain4j** | Java 大模型应用开发框架 | 模型调用、对话记忆、工具调用、RAG 等能力 |
| **MCP** | Model Context Protocol | 规范 AI 应用与外部工具、资源的接入方式 |
| **JSON-RPC 2.0** | JSON 格式的远程过程调用协议 | 表达方法调用、参数、请求 ID、执行结果及错误 |
| **stdio** | 标准输入/标准输出 | 让父子进程通过输入/输出流交换消息 |

### 2.1 Java 21

Java 21 提供虚拟线程等能力，也支持 `record`、更完善的模式匹配等现代语言特性。它是**开发和运行平台**，并非 Agent 专属技术。

### 2.2 LangChain4j

LangChain4j 封装了常见 LLM 应用开发能力。例如用 `@Tool` 将 Java 方法描述为模型可以调用的工具：

```java
class FileTools {
    @Tool("读取文件内容")
    String readFile(String path) {
        // 实际项目需要实现路径校验、权限控制、文件读取等
        return "...文件内容...";
    }
}
```

> 示例只展示概念，`@Tool` 需导入对应 LangChain4j 类型并配置工具注册/调用流程。模型负责**选择工具**，Java 程序负责**实际执行**。

### 2.3 MCP

MCP 将工具访问标准化。比如 Coding Agent 可以通过 MCP Client 与文件、Git、数据库等 MCP Server 交互。它与 LangChain4j 不是同一类事物：

- LangChain4j 是**开发框架**。
- MCP 是**交互协议**。
- 你可以用 LangChain4j 开发使用 MCP 的 Agent。

### 2.4 JSON-RPC 2.0

JSON-RPC 规定如何表达调用及结果。例如普通 JSON-RPC 请求：

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "add",
  "params": { "a": 10, "b": 20 }
}
```

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": 30
}
```

- `method`：调用哪个方法；`params`：参数。
- `id`：关联请求和响应；`result`：执行结果。
- **这只是通用 JSON-RPC 示例，并非完整 MCP 工具调用格式。**

### 2.5 stdio

MCP 的一种常见本地传输方式是 stdio：主程序启动 MCP Server 子进程，经标准输入向子进程写入协议消息，子进程经标准输出返回协议消息。

```text
Agent 主进程 ──写入子进程 stdin──> MCP Server 子进程
Agent 主进程 <──读取子进程 stdout── MCP Server 子进程
```

标准输出要留给协议消息，日志通常写到 `stderr`，否则会破坏消息解析。MCP 不限于 stdio，还支持网络传输方式（如 Streamable HTTP）。

---

## 3. Agent 与 Harness

**Agent**：围绕任务目标进行推理、决策、使用工具的智能体系统。

**Harness**：让 Agent 能稳定运行的执行支撑机制，常包括模型调用、Agent Loop、工具调度、上下文管理、权限与错误恢复。

示例：用户要求“修复 `UserService.java` 的 Bug”。

```text
用户给任务
   ↓
Agent 判断需要看哪些文件、做哪些修改
   ↓
Harness 组织 LLM 调用、执行工具、保存结果
   ↓
Agent 根据工具输出继续决定下一步
   ↓
Harness 执行测试、回传结果、控制是否继续
   ↓
给用户返回修复结果
```

**区别**：Agent 强调完成任务的智能行为；Harness 强调运行机制。两者并不总有严格的模块边界。**Agent Loop** 往往由 Harness 驱动，同时也属于 Agent 运行机制。

NovaCode 的一种可能的代码组织方式（架构示意，不代表项目实际目录）：

```text
NovaCode/
├── Agent/       # 任务策略、角色和目标
├── Harness/     # AgentLoop、上下文、执行控制、权限
├── LLM/         # LangChain4j 模型接入
└── Tools/       # 文件、Shell、MCP Client
```

---

## 4. Java/Maven 常用命令及对项目的影响

| 命令 | 作用 | 可能的项目影响 |
| --- | --- | --- |
| `java -version` | 查看 Java 版本 | 只读环境信息 |
| `mvn -version` | 查看 Maven 版本及其使用的 JDK | 只读环境信息 |
| `mvn clean verify` | 清理并执行 Maven 构建验证 | 删除/重建 `target`，执行编译测试；插件/测试也可能有其他副作用 |
| `java -jar target/novacode.jar --help` | 启动 JAR 并传入 `--help` | 常用于显示帮助，实际取决于程序实现 |
| `java -jar target/novacode.jar demo` | 启动 JAR 并传入 `demo` | 具体由 NovaCode 的演示命令决定，可能调用 API/读写文件 |

### 4.1 `mvn clean verify`

- `clean`：删除 Maven 默认构建目录 `target/`。
- `verify`：按 Maven 生命周期运行到验证阶段，通常包含编译、测试、打包等。
- 生成的 JAR **不一定**名为 `novacode.jar`；要看 `pom.xml` 打包配置。
- 普通构建不会主动改源代码，但**测试和插件本质上可以执行任意被授权的操作**，可能触及数据库、网络或文件。
- Maven 可能在本地 `.m2/repository` 下载依赖。

### 4.2 `java -jar`、`--help` 与 `demo`

```java
public static void main(String[] args) {
    if (args.length > 0 && "demo".equals(args[0])) {
        System.out.println("运行演示模式");
    }
}
```

- `-jar` 是 Java 启动器选项。
- `--help`、`demo` 在这里是**传给应用程序的参数**，Java 不会自动赋予 `demo` 特殊含义。
- `java -jar` 要求 JAR 配置可执行入口（例如清单的主类），不保证所有 Maven 打的 JAR 都能这样启动。

查看 Git 工作区变化：

```bash
git status
```

`git status` 看不到被忽略文件和外部系统的变化，因此不能证明命令“绝对没有副作用”。

---

## 5. 网络编程基础：进程、IP、端口、Socket

### 5.1 程序运行不需要“运行端口”

- **进程 PID**：操作系统标识运行中进程的编号。
- **IP 地址**：网络通信中的地址。
- **端口号**：TCP/UDP 通信端点的本地/远程地址组成部分。

计算器、普通离线工具等不需要监听 TCP/UDP 端口。一项服务可以监听多个端口；多个 TCP 连接也可以使用同一服务端端口。**进程与端口并非一一对应**。

### 5.2 一个 TCP 连接的双方都有端口

```text
客户端：192.168.1.10:52341  <==== TCP ====>  服务端：192.168.1.100:8888
             本地临时端口                      服务端监听端口
```

- 服务端通常选择固定端口，便于客户端找到它。
- 客户端通常由操作系统自动选择本地 IP 与临时端口，不必手动绑定。
- 服务器响应会沿已建立的 TCP 连接返回；不需要应用层再指定一次客户端端口。

TCP 连接通过四元组区分：**源 IP、源端口、目标 IP、目标端口**。

### 5.3 两个程序能否使用相同端口？

通常**不能在相同本地 IP + 相同传输协议 + 相同端口上都独占建立普通监听套接字**，否则可能抛 `BindException: Address already in use`。

但需区分：

1. 同一服务端监听 `8888`，可以同时服务多个客户端连接；这不是重复监听。
2. TCP 8888 和 UDP 8888 属于不同协议空间。
3. 不同本地 IP 绑定同一端口，或特定的端口复用机制，可能允许多个套接字并存；具体取决于操作系统和绑定设置。

Windows 检查谁在监听：

```powershell
netstat -ano | findstr :8888
tasklist /FI "PID eq 1234"  # 1234 换成实际 PID
```

---

## 6. ServerSocket、Socket、bind、listen、accept

### 6.1 Java 层面的职责

| 类型/方法 | 作用 |
| --- | --- |
| `ServerSocket` | 服务端绑定/监听端口，并接受新连接 |
| `server.accept()` | 等待/取得一条新建立的 TCP 连接，返回 `Socket` |
| `Socket` | 表示与对方通信的端点，可获取输入输出流 |
| `socket.getInputStream()` | 读取对方发来的字节 |
| `socket.getOutputStream()` | 向对方发送字节 |
| `socket.close()` | 关闭这条通信 Socket |
| `server.close()` | 停止这个监听 Socket 接受新连接 |

### 6.2 bind 为什么存在？（本次聊天重点澄清）

**服务端已经知道监听 8888 时，不需要再想一次端口。** `bind()` 的作用是向操作系统指定：“**这个监听套接字使用本机的某个 IP 地址和 8888 端口**。”

底层概念可以分成：

```text
socket() ─创建底层套接字─> bind() ─指定本地地址─> listen() ─进入监听状态─> accept()
```

注意：上面的 `socket()` 是**操作系统的 Socket API**，**不是 Java 的 `new Socket()`**。

在 **Java 层面**：

```java
ServerSocket server = new ServerSocket(8888);
```

这行已封装底层创建、绑定和监听。也可以拆开：

```java
ServerSocket server = new ServerSocket();
server.bind(new InetSocketAddress(8888));  // Java 方法：绑定并启动监听
Socket client = server.accept();
```

**没有必要再调用一个 `listen()` Java 方法**；Java `ServerSocket` API 不提供这样的独立调用。

### 6.3 为什么还要指定 IP？

IP 只是在指定**监听哪些本地网络接口**时才需要重点考虑：

```java
// 只监听 IPv4 本机回环地址
server.bind(new InetSocketAddress("127.0.0.1", 8888));
```

- `127.0.0.1:8888`：仅本机 IPv4 回环访问。
- `192.168.1.100:8888`：绑定对应本地局域网地址（前提是本机确有该地址）。
- `0.0.0.0:8888`：IPv4 通配地址，匹配本机所有 IPv4 接口。
- `new ServerSocket(8888)`：使用本地通配地址；IPv4/IPv6 细节依环境而定。

### 6.4 接收到一个连接后还会监听吗？

**会。** `accept()` 返回一个用于通信的 `Socket`，但并不自动关闭 `ServerSocket`。

```java
ServerSocket server = new ServerSocket(8888);
Socket a = server.accept(); // 第一个连接
Socket b = server.accept(); // 第二个连接
```

如果只写一次 `accept()`，程序并不会自动再次调用它。常见写法是：

```java
try (ServerSocket server = new ServerSocket(8888)) {
    while (true) {
        Socket client = server.accept();
        // 应交由线程池等机制处理；这里仅展示持续 accept 的思路
        handleClient(client);
    }
}
```

> 如果 `handleClient(client)` 在当前线程中阻塞执行，服务端就无法及时回到 `accept()`；需要并发处理才能更好地同时服务多个连接。`handleClient` 是示意方法，并非 Java 标准 API。

关闭 `ServerSocket` 不会自动关闭已经由 `accept()` 返回的连接 Socket。

### 6.5 客户端为何通常不需要显式 bind？

```java
Socket socket = new Socket("192.168.1.100", 8888);
```

- `connect` 的目标是**服务器地址和端口**。
- 客户端仍会使用**自己的本地地址和端口**，通常由系统隐式选择。
- 只有要指定本地 IP 或端口时才通常显式 `socket.bind(...)`。
- 隐式选择本地端口不代表实现中一定显式调用了一次底层 `bind()` 系统调用。

---

## 7. Java TCP 服务端代码详解

下面是本次聊天示例的简化版本，展示一次连接、欢迎消息、收到 `2048` 后结束：

```java
import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.util.Scanner;

public class Server {
    public static void main(String[] args) {
        try (ServerSocket server = new ServerSocket(8888);
             Socket socket = server.accept();
             InputStream is = socket.getInputStream();
             OutputStream os = socket.getOutputStream();
             Scanner scanner = new Scanner(is, StandardCharsets.UTF_8)) {

            PrintWriter writer = new PrintWriter(
                new OutputStreamWriter(os, StandardCharsets.UTF_8), true);

            writer.println("你好，客户端！");

            boolean done = false;
            while (!done && scanner.hasNextLine()) {
                String line = scanner.nextLine();
                System.out.println("Client: " + line);
                if ("2048".equals(line)) {
                    done = true;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
```

> 这个示例是**单连接阻塞式服务器**，不是完整生产级服务器。为了避免本次聊天原始代码 GBK 与默认编码不一致，这里双方示例统一为 UTF-8。Scanner 的 `hasNextLine()` 也可能阻塞等待数据。

### 7.1 语法拆分

1. `try (资源声明...) {}`：try-with-resources；声明实现 `AutoCloseable` 的资源，退出时按反顺序关闭。
2. `new ServerSocket(8888)`：绑定并监听 8888 端口。
3. `server.accept()`：阻塞等待客户端连接，返回通信 Socket。
4. `socket.getInputStream()`：输入字节流，方向为客户端 → 服务端。
5. `socket.getOutputStream()`：输出字节流，方向为服务端 → 客户端。
6. `new Scanner(is, UTF_8)`：将输入字节解析成文本，便于按行读取。
7. `new OutputStreamWriter(os, UTF_8)`：把字符编码为 UTF-8 字节写入 Socket。
8. `new PrintWriter(..., true)`：方便发送文本；`println()` 时自动刷新。
9. `!done && scanner.hasNextLine()`：短路与；有下一行且尚未结束才循环。
10. `"2048".equals(line)`：比较字符串**内容**，而不是用 `==` 比较引用。
11. `catch (IOException e)`：处理常见 Socket/IO 异常。

### 7.2 原始代码中的 `UnknownHostException`

原示例使用了：

```java
catch (UnknownHostException e) { ... }
catch (IOException e) { ... }
```

服务端这段逻辑不需要专门按“未知远程主机名”处理，可以统一捕获 `IOException`。`UnknownHostException` 是 `IOException` 子类，典型于远程主机名解析失败的情况。

### 7.3 阻塞意味着什么？

- `accept()`：没有新连接时可能一直等。
- `read()` / `readLine()` / `hasNextLine()`：没有足够数据或 EOF 时可能等待。
- 单线程里如果一直处理一个客户端，就难以及时处理其他客户端。

---

## 8. Java TCP 客户端代码详解

本次聊天中的客户端做四件事：**连接服务器 → 发送一行文本 → 读取服务器的一行回复 → 关闭连接**。

```java
import java.io.*;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class Client {
    public static void main(String[] args) throws IOException {
        String hostname = "localhost";
        int port = 12345;

        try (Socket socket = new Socket(hostname, port)) {
            System.out.println("Connected to the server");

            BufferedReader reader = new BufferedReader(
                new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8));
            PrintWriter writer = new PrintWriter(
                new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8), true);

            writer.println("Hello, server!");
            String response = reader.readLine();
            System.out.println("Server response: " + response);
        }
    }
}
```

### 8.1 核心语法

- `new Socket(hostname, port)`：主动连接目标服务器；**不负责监听新连接**。
- `InputStreamReader`：字节解码成字符。
- `BufferedReader`：给字符输入增加缓冲及 `readLine()`。
- `PrintWriter`：提供 `print/println/printf` 等文本输出。
- `writer.println(...)`：往网络 Socket 写数据。
- `System.out.println(...)`：打印到本地控制台；**不是网络发送**。
- `reader.readLine()`：读取一行；如果对端始终不发送换行，也不关闭输出，可能一直等待。
- `throws IOException`：声明可能抛出 IO 相关受检异常。

### 8.2 客户端为什么没有 ServerSocket？

**因为客户端是主动连接方，不需要等别人主动连接自己。** 连接建立后，双方使用 `Socket` 输入/输出流双向通信。

注意：示例的 `12345` 必须有一个正在监听的服务端，不能直接拿上面的 `8888` 服务端配对而不修改端口。

---

## 9. 输入流、输出流与字符编码

### 9.1 输入和输出相对于“当前程序”而言

```text
客户端 OutputStream  ─────→  服务端 InputStream
客户端 InputStream   ←─────  服务端 OutputStream
```

- `InputStream`：当前程序从网络读取字节。
- `OutputStream`：当前程序向网络写入字节。
- **接收到数据并不会自动产生业务回复**。是否回复、回复什么，由你写的代码决定。

例子：

```java
byte[] buffer = new byte[1024];
int len = is.read(buffer);
if (len != -1) {
    String request = new String(buffer, 0, len, StandardCharsets.UTF_8);
    String response = "收到：" + request; // 业务代码生成响应
    os.write(response.getBytes(StandardCharsets.UTF_8));
}
```

> TCP 是字节流，没有天然“消息边界”。一次 `read()` 不保证得到完整的一条业务消息。实际项目应设计协议：定长、分隔符或长度字段等。

### 9.2 为什么 `getOutputStream()` 不需要 `new`？

```java
OutputStream os = socket.getOutputStream();
```

因为它是**与已建立 Socket 连接关联的输出流**。Socket 的实现把 Java `write()` 调用与底层操作系统 TCP 发送路径连接起来。`OutputStream` 本身是抽象类，也不适合无关联地创建一个“网络输出流”。

`os.write(...)` 是提交字节进行发送，不代表对端应用已经读到这些字节。

### 9.3 字节流与字符流

```text
Socket InputStream (字节)
    ↓ InputStreamReader：按编码解码
Reader (字符)
    ↓ BufferedReader：方便按行读取
readLine()

String / 字符
    ↓ OutputStreamWriter：按编码编码
Socket OutputStream (字节)
```

字符编码要两端协商一致；新项目一般优先 UTF-8。聊天原始服务端用 GBK 发送、`Scanner` 用默认编码接收，可能乱码。

### 9.4 `PrintWriter(..., true)`

`true` 启用自动刷新；调用 `println()`、`printf()`、`format()` 可触发刷新。`PrintWriter` 的常规写入错误不会像很多 IO API 那样直接抛出 `IOException`，必要时用 `checkError()` 检查。

---

## 10. HTTP 服务器与普通 TCP 服务器

### 10.1 两者不是同一层次

```text
HTTP —— 应用层：请求方法、URL、头、状态码、请求体与响应体
TCP  —— 传输层：可靠、有序的双向字节流
IP   —— 网络层：寻址与分组转发
```

上图是 HTTP/1.1、HTTP/2 的常见协议关系；**HTTP/3 使用基于 UDP 的 QUIC**。

### 10.2 普通 TCP 服务器

它收发的是字节，应用自己决定数据表示什么，例如：

```text
客户端发：LOGIN alice 123
服务端回：OK
```

这里 `LOGIN/OK` 的语义属于你自定义的应用层协议，不是 TCP 自带规则。

### 10.3 HTTP 服务器

HTTP 对请求/响应结构有统一约定：

```http
GET /user/123 HTTP/1.1
Host: localhost:8080
```

```http
HTTP/1.1 200 OK
Content-Type: application/json

{"id":123,"name":"Tom"}
```

HTTP/1.1 的实际消息格式还有内容长度、连接管理等规则，不能只用以上极简示例替代完整实现。

### 10.4 Spring Boot Web 中的过程

```text
浏览器/客户端
    ↓ HTTP 请求
Tomcat（通常由 Spring Boot 启动并配置）
    ↓ 接收连接、解析 HTTP
Spring MVC
    ↓ URL 路由、参数绑定
Controller
    ↓ 调用 Service / 数据访问
返回结果
    ↓ Spring MVC 与 Servlet 容器构造 HTTP 响应
浏览器/客户端
```

例子：

```java
@RestController
@RequestMapping("/user")
public class UserController {
    @GetMapping("/{id}")
    public String getUser(@PathVariable Long id) {
        return "用户ID：" + id;
    }
}
```

对于常见的 Spring Boot + Tomcat 项目，程序员无需自己用 `ServerSocket` 手写 HTTP 解析器。

---

## 11. Spring AOP

**AOP（Aspect-Oriented Programming，面向切面编程）**：将日志、权限、事务等横跨业务方法的公共逻辑独立出来，在目标方法运行的相应时机执行。

### 11.1 为什么需要？

不使用 AOP 时，多个 Service 方法可能都重复包含：记录日志 → 执行业务 → 记录结束时间。公共逻辑重复、难维护。

### 11.2 典型代码

业务类：

```java
@Service
public class UserService {
    public void addUser() {
        System.out.println("添加用户");
    }
}
```

切面类：

```java
@Aspect
@Component
public class LogAspect {
    @Before("execution(* com.example.service.UserService.*(..))")
    public void logBefore() {
        System.out.println("调用前记录日志");
    }

    @After("execution(* com.example.service.UserService.*(..))")
    public void logAfter() {
        System.out.println("方法执行结束");
    }
}
```

> 假设项目已经添加并启用了 Spring AOP 相关依赖及配置，且包名、切点与实际业务代码一致。

### 11.3 注解与切点表达式

| 写法 | 意义 |
| --- | --- |
| `@Aspect` | 声明切面 |
| `@Component` | 注册为 Spring Bean |
| `@Before` | 匹配方法调用前执行 |
| `@After` | 方法执行结束时执行，即使方法抛出异常 |
| `@Around` | 环绕控制目标方法执行，可决定何时调用 `proceed()` |
| `execution(...)` | 声明匹配哪些方法 |

`execution(* com.example.service.UserService.*(..))`：第一个 `*` 表示任意返回类型，类名后 `*` 匹配任意方法名，`(..)` 表示任意参数。

### 11.4 AOP 原理：代理

```text
调用方 → Spring 代理对象 → AOP 通知 → 原始目标方法 → AOP 后续通知
```

Spring AOP 主要用动态代理实现，常见机制包括基于接口的 JDK 动态代理和基于子类的 CGLIB 代理。

### 11.5 AOP 和事务

```java
@Service
public class OrderService {
    @Transactional
    public void createOrder() {
        // 创建订单、扣减库存等
    }
}
```

在典型 Spring 代理式事务管理中，事务拦截器负责在目标方法调用前后执行事务处理。不过**事务是否回滚仍取决于异常类型、传播行为、配置等**。

### 11.6 重要陷阱：同类内部调用

```java
@Service
public class UserService {
    public void methodA() {
        methodB(); // 直接在本对象上调用，通常绕过 Spring 代理
    }

    @Transactional
    public void methodB() {
        // 数据库操作
    }
}
```

外部通过代理对象调用 `methodA()`，其中直接调用的 `methodB()` 通常不会获得它自己标注的代理式事务增强。原因是**内部调用没有再经过代理对象**。

---

## 12. 常见误区速查

| 易错说法 | 正确理解 |
| --- | --- |
| “程序运行必须有一个端口” | 不是；端口是 TCP/UDP 网络通信概念，进程通过 PID 管理 |
| “`ServerSocket` 和 `Socket` 都负责监听” | 不是；Java `ServerSocket` 监听/接受连接，`Socket` 负责通信 |
| “`accept()` 后 `ServerSocket` 不监听了” | 不会自动停止监听；但程序要再次调用 `accept()` 才能取得下一个连接 |
| “服务端知道 8888 了，bind 还要重新选端口” | 错；`bind` 就是将这个 Socket 绑定到指定本地 IP/端口，通常就是已选定的 8888 |
| “Java `new Socket()` 等于底层 `socket()`” | 不能混同；底层 `socket()` 是 OS Socket API，Java 两种类是更高层封装 |
| “监听 8888 后只能连接一个客户端” | 错；可以有多个不同 TCP 连接使用服务端端口 8888 |
| “收到数据，输出流自动产生回复” | 错；业务逻辑决定是否生成回复，输出流仅负责发送指定数据 |
| “一次 `read()` 就是一条完整消息” | 错；TCP 是字节流，应用需要定义消息边界 |
| “`System.out.println()` 会发给远程服务器” | 错；它输出到本地标准输出；网络发送需写 Socket 的输出流 |
| “HTTP 与 TCP 是同级协议” | 错；HTTP 是应用层协议，TCP 是传输层协议；HTTP/3 另用 QUIC |
| “Spring AOP 修改了目标方法源代码” | 通常是通过代理拦截调用，不直接修改源文件 |
| “同一个类里直接调用 `@Transactional` 方法一定生效” | 代理模式下通常会绕过目标方法的事务增强 |
| “`mvn clean verify` 完全不会影响文件或外部环境” | 默认主要重建 `target`，但插件与测试可能造成额外副作用 |

---

## 13. 自测题与参考答案

建议先独立口述，再展开答案。

<details>
<summary>1. Java 的 ServerSocket 和 Socket 分别负责什么？</summary>

`ServerSocket` 用于服务端监听/接受新 TCP 连接；`accept()` 返回 `Socket`；双方通过 `Socket` 收发数据。

</details>

<details>
<summary>2. 服务器监听 8888 后，还需要单独为每个客户端开一个新监听端口吗？</summary>

不需要。服务端的 8888 可对应多个已建立的 TCP 连接，由四元组区分。

</details>

<details>
<summary>3. bind、listen、accept 分别干什么？</summary>

`bind` 指定 Socket 的本地地址；`listen` 使 TCP Socket 进入监听状态；`accept` 获取已建立的客户端连接。Java `ServerSocket(8888)` 已封装绑定与监听。

</details>

<details>
<summary>4. 客户端已经知道服务器 IP 和端口，为什么还需要自己的本地端口？</summary>

TCP 连接两端都有地址，客户端本地端口通常自动选择，用于标识本机连接端点，配合 IP/其他端口构成 TCP 四元组。

</details>

<details>
<summary>5. `accept()` 返回后服务端是否还保持监听？</summary>

保持，直到监听 Socket 被关闭等情况；但需要再次调用 `accept()` 才会取出下一个连接。

</details>

<details>
<summary>6. 为什么 `reader.readLine()` 可能阻塞？</summary>

它读取一行；若对端未发送完整行、也没有结束流，就可能继续等待。

</details>

<details>
<summary>7. `System.out.println()` 和 `writer.println()` 有什么区别？</summary>

前者输出到本地标准输出；后者若包装 Socket 输出流，则发送文本到对端。

</details>

<details>
<summary>8. TCP 已经可靠传输，为什么还需要 HTTP？</summary>

可靠传输不等于定义业务含义。HTTP 规定请求方法、路径、头、状态码、消息语义等应用层规则。

</details>

<details>
<summary>9. Spring AOP 通过什么方式给方法加日志？</summary>

常见 Spring AOP 通过代理对象拦截匹配的方法调用，在适当时机执行通知逻辑。

</details>

<details>
<summary>10. 典型代理式事务为什么会遇到同类内部调用失效？</summary>

同类直接调用没有经过 Spring 代理，目标方法上的代理式事务拦截器就无法按预期介入。

</details>

<details>
<summary>11. `mvn clean verify` 与 `java -jar ... demo` 的安全性差异？</summary>

前者主要重建与验证项目，但测试和插件可能有副作用；后者直接运行应用的 demo 逻辑，可能执行 Agent 工具、读写文件或调用付费 API，需要查看实际代码。

</details>

<details>
<summary>12. Agent、Harness、MCP 分别是什么？</summary>

Agent 是面向任务的智能体系统；Harness 是运行支撑机制；MCP 是标准化的工具/资源交互协议。

</details>

---

## 14. 学习优先级与自检

**应记住的原理：**

1. 服务端 `ServerSocket` 监听/接受连接，连接双方用 `Socket` 通信。
2. IP + 端口描述网络通信地址；进程并不是通过端口号运行的。
3. `bind` 指定本地地址，`listen` 开始监听，`accept` 取得连接；Java 已做适当封装。
4. TCP 是双向字节流，业务程序需要自己规定消息格式、何时回复。
5. 字节流与字符流之间通过编码器/解码器转换。
6. HTTP 在传输层之上提供请求和响应语义。
7. Spring AOP 通过代理等机制把公共逻辑加入方法调用过程。

**查文档即可的细节：**各种 IO 包装类的完整构造参数、切点表达式全部语法、Maven 插件配置、少见 Socket 选项。做项目遇到再深入。

### 自检结论

- 已区分 **Java `ServerSocket`** 与 **操作系统底层 `socket()`**，消除原对话中的层次混淆。
- 已强调 Java `ServerSocket.bind()` 和 `ServerSocket(8888)` **封装了监听启动**；不需要手写 `listen()`。
- 已区分服务器监听端口、客户端本地临时端口、已建立连接的端口使用方式。
- 已补充 TCP 字节流的**消息边界**、`readLine()` 阻塞、编码一致性和 `PrintWriter` 自动刷新等容易遗漏的细节。
- 文中 NovaCode 模块划分、示例服务端/客户端均为**教学示意**，不代表对实际仓库源码的检查结果。
