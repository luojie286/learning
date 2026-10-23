---
id: java-sockets
title: "Socket、协议边界与 NIO"
description: "从阻塞流到 Selector，理解连接资源与消息分帧。"
course: java-concurrency
order: 6
date: 2026-10-23
tags: [Java, 后端]
---

# Socket、协议边界与 NIO

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 端点和连接

IP、端口和传输协议定位通信端点；TCP 连接常用源 IP/端口、目的 IP/端口四元组区分，讨论时还需考虑协议与网络命名空间。服务端一个监听端口可对应大量连接。原稿“一端口只能一个程序”要限定地址、协议、绑定选项和操作系统，不能当绝对规则。

ServerSocket 负责监听，accept 返回与具体客户端通信的 Socket。底层 socket/bind/listen/accept 有助于理解，但 Java 构造器封装不要求开发者逐一调用这些系统调用。UDP 无需 TCP 握手，但 API 中也有 connect 这种设置默认对端的行为。

## TCP 没有消息边界

一次 write 不对应一次 read，可能分段也可能合并。应用需要分隔符、固定长度或长度前缀，并对大小设上限。以长度前缀为例：

```java
DataInputStream in = new DataInputStream(socket.getInputStream());
int size = in.readInt();
if (size < 0 || size > 1024 * 1024) throw new IOException("非法帧大小");
byte[] payload = new byte[size];
in.readFully(payload);
```

这是片段：调用方需管理 socket 生命周期、读取超时、异常和协议版本。读前先验证大小，防止对方让服务分配巨型数组。read 返回的长度可能小于请求长度，不得假定数据总是一次读齐。

## 实际开发：按行聊天

双方明确 UTF-8 与换行分隔；发送端只 print 不换行且不关闭输出，readLine 可能一直等。PrintWriter 自动刷新只针对特定方法，不代表每次 write 都发出；它还可能把 IO 错误记录为内部状态，协议代码要检查错误。

用 Socket.connect 指定连接超时，用 setSoTimeout 设置阻塞读取超时；发送成功仅说明数据交给相应层，并不证明订单业务已提交。

## BIO 与 NIO

阻塞模型可用线程池服务多连接，但长时间占用线程和无界连接排队会耗尽资源。NIO 的 Channel、Buffer、Selector 允许少量线程管理多个可就绪通道；read/write 仍可能部分完成，需维护每个连接的帧解析与待发送状态。

Selector 不替你处理业务慢调用。Netty 等框架封装事件循环和编解码，业务阻塞应按框架规则隔离。TCP 的可靠有序是字节传输语义，不等于业务恰好一次；UDP 也不必然更快，选型需看拥塞控制、时延、可靠性和应用协议。

## 参考资料

- [Socket API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/net/Socket.html)
- [DataInputStream.readFully](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/io/DataInputStream.html)
- [Selector API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/nio/channels/Selector.html)

## 模拟面试问题

1. 【基础】监听 Socket 与 accept 返回的 Socket 有何区别？
2. 【应用】为什么不能按每次 read 结果当成完整消息？
3. 【应用】长度前缀协议要防范哪些输入？
4. 【深入】Selector 为什么不自动解决慢数据库调用？
5. 【深入】TCP 可靠传输为什么仍可能出现重复下单？

