---
id: java-design-agent
title: "设计模式与 Java 工具型 Agent"
description: "从可替换策略到受控工具执行，区分框架、协议与模型决策。"
course: java-engineering
order: 5
date: 2026-10-23
tags: [Java, 后端]
---

# 设计模式与 Java 工具型 Agent

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 设计模式服务具体变化点

工厂集中创建策略，Builder 构建参数较多的对象，代理增强访问，装饰器叠加能力，适配器转换接口；策略封装可替换算法，观察者通知订阅者，模板固定骨架并开放步骤。使用模式前先说清“哪个变化导致现在代码难维护”。

单例的唯一性需限定类加载器、容器与实例定义；Spring singleton 不是 GoF 私有构造单例的同义词。JdbcTemplate 体现模板化执行与回调协作，不应把所有名字含 Template 的类机械说成严格继承式 Template Method。

## 实际开发：订单只读查询工具

模型提出 `lookupOrder(orderId)`，Java 工具层做参数校验、用户授权与数据库查询，再返回最小必要信息。模型不直接执行 SQL，也不能因为它传来 admin=true 就获得权限。工具只接受结构化 ID，不接受任意 SQL、Shell 或跨目录路径。

```text
用户目标 → 模型生成工具调用意图
         → 应用校验参数与权限
         → Java 工具执行并记录结果
         → 结果回到模型
         → 下一步或结束
```

执行循环需设置工具次数、总时间和上下文预算；失败区分可重试网络故障与不可重试的无权限，不能无限重试。对会写数据的工具，需要幂等与明确授权边界。

## Function Calling、MCP 与 Java 框架

Function Calling 表达模型要调用什么及参数，MCP 标准化应用与外部能力的发现和交互，两者可组合也可独立。MCP 角色包括 Host、Client、Server，Server 可以是本地子进程，不等于远端云主机。协议版本要固定核对，不能把某一版本的传输说成永久不变。

stdio 传输时 stdout 留给协议消息，日志放 stderr；Streamable HTTP 用于相应 HTTP 连接方式。服务端提供工具不代表每个模型或应用都自动授权执行。LangChain4j 是 Java 应用框架，MCP 是协议，Agent/Harness 是行为与运行机制概念，不能互相替代。

## 边界与验证

外部网页、工具结果和检索文档是数据，可能包含误导指令；不能自动提升为操作授权。最小权限工具、审计日志和可恢复状态比堆砌提示词更可靠。AGENTS.md 记录仓库约定，README 解释使用，架构文档记录决策；是否读取以及层级规则由具体工具决定。

这一章仅拓展原稿已有概念，不包含未经验证的“某框架当前最佳”或模型排行；完整生成式软件工程课程再展开评估、上下文与失效模式。

## 参考资料

- [MCP 架构](https://modelcontextprotocol.io/docs/learn/architecture)
- [MCP 传输规范](https://modelcontextprotocol.io/specification/2025-06-18/basic/transports)
- [LangChain4j 工具调用](https://docs.langchain4j.dev/tutorials/tools/)

## 模拟面试问题

1. 【基础】策略、工厂和适配器各解决什么变化？
2. 【应用】订单查询工具为什么不能直接信任模型传入的 userId？
3. 【应用】模型工具调用失败应如何决定重试？
4. 【深入】Function Calling 与 MCP 在系统中分别负责什么？
5. 【深入】stdio 协议为什么要求日志不写 stdout？

