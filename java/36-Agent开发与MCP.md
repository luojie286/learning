# 36 · Agent 开发与 MCP

> 一句话概括：Agent = LLM 决策 + 工具执行 + 执行循环；MCP 是 AI 应用接入外部工具的标准协议（JSON-RPC 消息 + stdio/HTTP 传输）；LangChain4j 是 Java 侧的开发框架——三者各管一层，可组合使用。

## 一、Agent 是什么

**AI Agent 是利用 LLM 进行决策、通过工具执行步骤来完成任务的应用系统**，简化组成：

- **LLM**：理解目标，选择下一步动作；
- **Tools**：查库、搜索、读写文件、调 HTTP API 等；
- **状态/上下文管理**：维护消息历史与任务状态；
- **执行循环**：调用模型 → 判断是否用工具 → 执行工具（由**后端程序**执行，不是 LLM）→ 结果回灌上下文 → 继续，直到模型直接回答。

与常规问答的区别：Agent 根据中间结果**动态决定下一步**。RAG 可以作为 Agent 的一个工具能力，但 Agent ≠ RAG。实际系统还需要参数校验、权限控制、超时、错误处理与工具调用次数限制。

## 二、Function Calling vs MCP

| | Function Calling | MCP |
|---|---|---|
| 关注点 | 模型如何表达"调哪个工具、传什么参数" | AI 应用如何统一连接、发现、调用外部能力 |
| 关系 | 不依赖 MCP 也能用 | 可为模型工具调用提供标准化工具接入 |

二者可同时使用，不冲突。

## 三、MCP：模型上下文协议

```text
AI 应用（MCP Host）→ MCP Client ⇅ MCP 通信 ⇅ MCP Server
                                                  ├─ Tool（执行操作）
                                                  ├─ Resource（提供数据）
                                                  └─ Prompt（提示模板）
```

- 消息格式是 **JSON-RPC 2.0**（`method`/`params`/`id`/`result`）；传输常见 **stdio** 和 Streamable HTTP。
- stdio 模式：主进程启动 MCP Server 子进程，写它的 stdin、读它的 stdout 交换消息——**stdout 要留给协议消息，日志写 stderr**，否则破坏解析。
- MCP Server 可以是**本地进程**，不一定是云服务器；现有 Spring Boot 业务逻辑可以包装成 MCP 工具供 Agent 调用，不必重写。

## 四、Java 技术栈

| 技术 | 是什么 |
|---|---|
| Java 21（LTS） | 运行平台：虚拟线程、record、模式匹配 |
| LangChain4j | Java LLM 应用框架：模型调用、对话记忆、工具调用、RAG |

```java
class FileTools {
    @Tool("读取文件内容")
    String readFile(String path) { /* 路径校验、权限控制…… */ return "..."; }
}
```

LangChain4j 是**开发框架**，MCP 是**交互协议**——可以用 LangChain4j 开发使用 MCP 的 Agent。模型负责"选工具"，Java 程序负责"真执行"。

## 五、Agent 与 Harness

**Harness** 是让 Agent 稳定运行的支撑机制：模型调用、Agent Loop、工具调度、上下文管理、权限与错误恢复。Agent 强调"完成任务的智能行为"，Harness 强调"运行机制"，两者没有严格模块边界。一个可能的代码组织：`Agent/`（策略）+ `Harness/`（循环、上下文、权限）+ `LLM/`（模型接入）+ `Tools/`（文件、Shell、MCP Client）。

## 六、AGENTS.md：给编码 Agent 的仓库约定

`AGENTS.md` 是部分 coding Agent 会读取的仓库约定文件，写明产品前提、技术栈、架构决策、操作边界——**是否读取取决于具体工具**。与其他文档分工：`README.md` 教新人跑项目，`PRD.md` 定产品范围，`ARCHITECTURE.md` 讲架构决策，`NEXT_TASKS.md` 记当前任务，`AGENTS.md` 记"Agent 在本仓库工作的稳定约定"。内容可用中文。写法原则：不写"先读代码再测试"这类流水账，但要把**架构边界和高影响操作**（换媒体架构、加依赖、改 schema、commit/push 需授权）写清楚——Agent 越自主，边界越要写清。

## 📌 易错点

- "MCP = Function Calling"——一个是接入协议，一个是模型表达机制。
- "MCP Server 一定是远程服务器"——可以是本地子进程（stdio）。
- "日志往 stdout 打没关系"——stdio 模式下会污染协议消息，日志走 stderr。
- "工具是 LLM 执行的"——LLM 只决定调什么，执行在你的后端。
- "Agent 不需要 AGENTS.md"——低价值流程约束可以省，高风险边界必须写。

## 🧠 速记口诀

```text
Agent 三件套：模型决策、工具执行、循环推进
Function Calling 说意图，MCP 管接入，JSON-RPC 定消息，stdio 走管道
LangChain4j 是框架，MCP 是协议，Java 21 是地基
Harness 托底运行，AGENTS.md 画边界
stdout 传协议，stderr 记日志
```

## ✅ 自测问题

1. Agent 为什么会多次调用 LLM？工具由谁执行？
2. MCP Client 与 Server 各承担什么？Server 一定是远程的吗？
3. MCP 与 Function Calling 的区别与关系？
4. stdio 传输下为什么日志要写 stderr？
5. AGENTS.md 和 README.md / NEXT_TASKS.md 的分工是什么？

> 来源：原始《Agent 与计算机网络基础学习笔记》(Agent与计算机网络基础学习笔记_2026-10-10.md §1-2)、《Java 后端学习笔记：Agent 技术栈、Socket 网络编程与 Spring AOP》(Java后端学习笔记_Agent技术栈Socket与SpringAOP_2026-10-10.md §2-3)、《计算机网络、Git 与 Agent 工程实践》(AGENTS.md 部分)；文中 NovaCode 结构为教学示意。
