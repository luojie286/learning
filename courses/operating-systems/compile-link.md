---
id: os-compile-link
title: 从源码到运行：编译、链接与诊断
description: 把语法、符号解析、动态库加载和性能优化分层判断。
course: operating-systems
order: 2
date: 2026-10-23
tags: [操作系统, 后端, 面试]
---

# 从源码到运行：编译、链接与诊断

## 四个阶段与观察方法

典型 C 工具链为预处理、编译、汇编、链接。预处理展开 include/宏与条件分支，编译将语言转换为目标汇编，汇编产生可重定位目标文件，链接解析符号并组织可执行文件。GCC 是驱动程序，具体工具与中间文件是否写盘随参数变化，不必真有四个可见子进程。

```bash
gcc -E hello.c -o hello.i
gcc -S hello.c -o hello.s
gcc -c hello.c -o hello.o
gcc hello.o -o hello
```

-E 停在预处理，-S 停在汇编文本，-c 产生目标文件而不链接；-o 后是输出名。目标文件并非单纯一串待定地址的机器码，还包含节、符号、重定位等信息。文件后缀只是常见约定，不能仅以名字判断格式。

## 链接器究竟找什么

调用了声明但没有实现，或没有把对应目标文件/库传给链接器，可出现 undefined reference。静态库通常只将被需要的目标成员提取进入结果，并非无条件复制全库；动态链接保留相关依赖与动态符号处理，装载器在运行时加载与解析，可能存在延迟绑定。

原稿“printf 的实现一定被合并进可执行文件”仅对应某种静态链接情况。常见动态链接 C 程序运行时依赖共享 libc。运行时缺 `.so` 与链接期找不到符号属于不同阶段，排查也不同。

头文件放普通外部函数定义，多个翻译单元可能产生 multiple definition；但 static、inline 和 C/C++ 语义存在细节，不能说所有头文件函数体都违法。C/C++ 混编还需一致的调用约定与符号链接，例如合适的 extern "C"。

## 开发例子：JNI 库在开发机能跑、服务器失败

Java 源码通常先成为字节码，由 JVM 解释/JIT/AOT 等执行，不能完整套用 C 的四阶段产物。JNI 原生库则再次引入本机架构和动态链接依赖：开发机 x86_64 Linux 编译的 so 不能默认在不同架构或不同 libc 环境加载。

排查顺序是确认目标系统与架构、库实际路径和依赖、符号导出、JVM/原生签名，再看业务逻辑。Java UnsatisfiedLinkError 可能来自找不到文件、依赖缺失或方法符号不匹配，不等于 Java 编译器语法错误。

## 优化不改变程序的合法语义

-O0、-Og、-O2、-O3 是一组优化策略，不保证级别越高每个程序越快或更小；某些变换增加代码体积或改变调试可见性。调试可用 -g 配合适当优化，-Og 并非绝对禁止优化。性能比较用相同输入和构建条件，先看算法与热点再调选项。

编译器可利用语言规则优化，C/C++ 未定义行为可能让错误在优化后暴露；不能用“不开优化就正常”证明代码正确。原稿“本质基于概率”和“优化性能、避免”缺上下文，不补成确定的编译原理结论，仅在修订记录标为未证实碎片。

## 适度拓展

链接也会受库顺序、LTO、符号可见性与构建系统影响。先用最小两个源文件复现缺实现或重复定义，再回到大型工程，错误范围更清楚。构建成功只证明生成产物，不证明功能、性能或安全要求全部达标。

## 参考资料

- [GCC Overall Options](https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html)
- [GCC 优化选项](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GNU ld：库与符号链接](https://sourceware.org/binutils/docs/ld/)
- [Java 17 UnsatisfiedLinkError](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/UnsatisfiedLinkError.html)

## 模拟面试问题

1. 基础理解：-E、-S、-c 分别在哪里停止？
2. 实际应用：undefined reference 和运行时缺共享库为什么要分开排查？
3. 实际应用：JNI 开发机正常、服务器加载失败应检查哪些环境条件？
4. 深入追问：静态链接是否把整座静态库完整复制进程序？
5. 深入追问：为什么优化级别更高并不保证程序更快？
