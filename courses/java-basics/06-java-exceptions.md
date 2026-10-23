---
id: java-exceptions
title: "异常传播与资源清理"
description: "区分编译期处理义务、业务失败与外部资源生命周期。"
course: java-basics
order: 6
date: 2026-10-23
tags: [Java, 后端]
---

# 异常传播与资源清理

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 谁必须处理

Throwable 下的 RuntimeException 家族与 Error 家族属于非受检异常，编译器不强制捕获或声明；其余受检异常必须捕获或通过 throws 传播。Error 通常表示严重运行问题，不应常规吞掉，但“任何 Error 都无法捕获”并非语言规则。

throw 真正抛出对象；throws 只是方法声明，不主动触发异常。运行时异常无需层层声明也会沿调用栈传播。catch 要把子类型放在父类型前，否则后续分支不可达。

原稿包装 UnsupportedEncodingException 成 RuntimeException 后，调用方仍专门捕获已不可能抛出的受检异常，示例无法正确编译；新代码使用明确 Charset，避免制造这个无效异常路径。

## 读取订单导出文件

```java
static List<String> readOrders(Path file) throws IOException {
    try (BufferedReader reader =
             Files.newBufferedReader(file, StandardCharsets.UTF_8)) {
        List<String> lines = new ArrayList<>();
        String line;
        while ((line = reader.readLine()) != null) lines.add(line);
        return lines;
    }
}
```

文本需要编解码，图片、压缩文件等用字节流。Reader 的 read() 终止标记为 -1，readLine() 为 null。大文件应流式逐条处理而非全部收集到内存；读取网络数据还需要超时、大小上限和取消能力。

try-with-resources 管理 AutoCloseable，按资源声明的逆序关闭。正文异常和关闭异常同时出现时，关闭异常可记录为 suppressed，排障不能只看最外层消息。包装流通常负责关闭底层流，不要把标准输入或共享输出的所有权随意交给局部方法。

## finally 的边界

正常返回、抛异常时通常执行 finally；JVM 强制终止、进程崩溃或永不离开的循环都可能让它没有机会运行。因此不能宣称“只有 System.exit 这一个例外”。不要在 finally 中 return，它可能覆盖返回值或吞掉异常。

## 业务层如何处理

库存不足可以转换成明确业务异常；数据库断线应保留 cause 并按系统故障处理。每层只 printStackTrace 再返回 null，会丢掉失败语义，最终可能以 NPE 结束。选择能恢复、能补充语义或对外形成响应的边界处理一次，日志避免重复打印。

## 参考资料

- [JLS 17：异常](https://docs.oracle.com/javase/specs/jls/se17/html/jls-11.html)
- [AutoCloseable API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/AutoCloseable.html)
- [Files API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/nio/file/Files.html)

## 模拟面试问题

1. 【基础】throws 会主动创建并抛出异常吗？
2. 【应用】什么时候应该包装异常并保留 cause？
3. 【应用】大文件导出为何不能总用 readAllLines？
4. 【深入】try-with-resources 如何处理关闭时再次发生的异常？
5. 【深入】finally 中 return 有什么风险？

