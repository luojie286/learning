# 23 · JVM 字节码与栈帧

> 一句话概括：`javac` 把 `.java` 编译成**栈式字节码**，JVM 每次方法调用创建一个**栈帧**（局部变量表 + 操作数栈 + 动态链接 + 返回信息），线程私有。
> （类加载 → `22-类加载机制.md`；原子性与 CAS → `17-JUC并发工具.md`）

---

## 一、从源码到机器指令

```text
.java 源代码
   ↓ javac
.class 字节码
   ↓ JVM 解释 / JIT 编译
机器指令
```

`javap` 是 JDK 自带的反汇编工具：

```bash
javap Test        # 类结构
javap -c Test     # 字节码
javap -v Test     # 详细信息：常量池、版本、stack/locals、LineNumberTable
```

### 加法字节码示例

```java
int a = 1; int b = 2; int c = a + b;
```

```text
iconst_1   → 常量 1 入栈
istore_1   → 存入局部变量表 slot 1
iconst_2   → 常量 2 入栈
istore_2   → 存入 slot 2
iload_1    → a 入操作数栈
iload_2    → b 入操作数栈
iadd       → 弹出两个 int 相加，结果入栈
istore_3   → 存入 c
```

常见指令速记：`iconst/bipush` 常量、`iload/aload` 加载局部变量、`istore/astore` 保存、`iadd/isub/imul` 运算、`getfield/putfield` 对象字段、`getstatic/putstatic` 静态字段、`invokevirtual`（实例方法）/ `invokestatic`（静态）/ `invokespecial`（构造、private、super）/ `invokeinterface`（接口）、`new`、`ireturn/areturn/return`。

> `new User()` 典型字节码：`new` → `dup` → `invokespecial User."<init>"`。

---

## 二、栈式虚拟机 vs 寄存器式虚拟机

| | 操作数怎么指定 | 例子 | 特点 |
|---|---|---|---|
| **栈式**（JVM） | 栈顶**隐式** | `iadd` | 指令简单紧凑、易跨平台；同逻辑指令更多 |
| **寄存器式**（Dalvik/ART） | 寄存器编号**显式** | `add-int v2, v0, v1` | 指令更少、更接近真机；编码和编译器更复杂 |

> ⚠️ **JVM 是栈式 ≠ CPU 也栈式执行**：JIT 编译后的机器码照样大量使用 CPU 寄存器。

---

## 三、栈帧

每次方法调用创建一个**栈帧**，方法结束出栈销毁——**栈帧生命周期 ≈ 一次方法调用**。

```text
┌──────────────┐
│ add() 栈帧    │  ← 栈顶
├──────────────┤
│ test() 栈帧   │
├──────────────┤
│ main() 栈帧   │
└──────────────┘
```

### 局部变量表（放变量）

- 按 `slot` 编号访问；**实例方法 slot 0 是 `this`**，static 方法没有 `this`
- `int/float/reference` 占 1 个 slot，**`long/double` 占 2 个连续 slot**

```java
static void test(int a, long b, int c) { }
// slot 0 → a，slot 1+2 → b，slot 3 → c
```

### 操作数栈（做计算）

```text
[] → iload a → [a] → iload b → [a, b] → iadd → [a+b] → istore c → []
```

> **局部变量表 = 放变量；操作数栈 = 做计算。**

### 参数与返回值传递

```text
调用者把参数压入自己的操作数栈 → 创建被调用者栈帧 → 参数进入其局部变量表
返回值通过 ireturn/areturn 回到调用者操作数栈 → 调用者再存到自己局部变量表
```

### 动态链接

帮助方法把运行时常量池里的**符号引用**定位到实际的类/字段/方法（对应类加载的解析阶段概念）。

---

## 四、`javap -v` 里的 stack / locals

```java
static int add(int a, int b) { int c = a + b; return c; }
// stack=2, locals=3
```

- `locals=3`：slot 0=a、1=b、2=c
- `stack=2`：算 `a+b` 时操作数栈最多同时放两个值

---

## 五、栈帧是线程私有的

每个线程有自己的 JVM 栈：

```text
局部变量 → 在自己栈帧里，不共享（两个线程调同一方法，各自的 count 互不影响）
对象字段 → 在堆里，同一对象被多线程共享时才有竞争
```

> 所以「多线程同时调用同一个方法」本身不会让局部变量共享——**共享的是对象**。

---

## 六、StackOverflowError

```java
static void test() { test(); }   // 无限递归
```

每递归一次压一个栈帧，栈空间耗尽 → `java.lang.StackOverflowError`。本质：**同一线程里未退出的方法栈帧太多**。

---

## 七、字节码条数 ≠ 原子性

```java
count++;   // → getstatic、iconst_1、iadd、putstatic（显然不是一步）
```

反过来，**一条字节码也不能断定是原子的**——它最终仍可能被解释/JIT 成多条机器指令。

> 判断原子性要依据 **JMM 与具体同步语义**，不是数字节码有几条（同理见 `20-volatile.md`）。

---

## 📌 易错点

1. **以为 JVM 栈式 ⇒ CPU 也栈式执行** ✗——JIT 后仍大量用寄存器。
2. **以为一条字节码 = 一个原子操作** ✗——判断原子性靠 JMM 语义。
3. **忘了实例方法 slot 0 是 `this`**（static 方法没有）——这也是 `this` 在 static 中不可用的原因之一。
4. **忘了 `long`/`double` 占两个 slot**。
5. **以为局部变量也会被多线程共享** ✗——局部变量在私有栈帧，共享的是堆里的对象。
6. **把 `<init>` 和 `<clinit>` 搞混**——前者对象初始化，后者类初始化（见 `22`）。

---

## 🧠 速记口诀

```text
javap -c 看字节码，-v 看常量池与 stack/locals
栈式靠栈顶隐式取数（iadd），寄存器式显式给编号（add-int v2,v0,v1）
栈帧 = 局部变量表（放变量）+ 操作数栈（做计算）+ 动态链接 + 返回信息
实例方法 slot0 = this；long/double 占两 slot
栈帧线程私有：局部变量不共享，对象字段才共享
字节码条数不能判原子性
```

---

## ✅ 自测问题

1. `.java → .class → 机器指令` 分别由谁完成？
2. 栈式与寄存器式虚拟机在「操作数怎么指定」上的本质区别是什么？各有什么优缺点？
3. 栈帧包含哪些部分？局部变量表和操作数栈分别负责什么？
4. 实例方法和 static 方法的局部变量表 slot 0 各是什么？`long` 占几个 slot？
5. 方法调用时参数如何从调用者传到被调用者？返回值怎么回来？
6. 为什么局部变量不会因多线程调用同一方法而共享？
7. `StackOverflowError` 的本质原因是什么？
8. 为什么说「一条字节码 ≠ 原子操作」？

---

## 八、JVM 知识对后端排障的映射

学 JVM 不是纯理论——后端常见问题都能在 JVM 里找到对应解释：

| 后端现象 | 对应 JVM 知识 |
|---|---|
| 递归太深、`StackOverflowError` | 虚拟机栈、栈帧（本篇） |
| 服务内存增长、`OutOfMemoryError` | 堆、对象引用、GC、内存分析 |
| 接口偶发延迟升高 | GC 停顿、内存分配、性能分析 |
| 多线程读写不一致 | JMM、`volatile`、`synchronized`、happens-before（见 `19`、`20`） |
| 服务 CPU 飙高、线程阻塞 | 线程状态、线程转储（thread dump） |
| Spring 运行时对象和代理行为 | 类加载、反射、动态代理、对象生命周期（见 `22`、`25`） |

典型例子：不断 `list.add(new byte[1MB])` 且 `list` 一直存活 → 数组无法被 GC 回收 → 堆占用增长 → 最终 `OutOfMemoryError`。能做出这个推断，JVM 就算学到位了。

### 主线与辅线的学法

```text
主线（约 70%）：Spring / Spring Boot → MySQL → Redis → 后端项目
辅线（约 30%）：JVM 运行原理 → GC / JMM → 基础排障
```

比例可随项目阶段调整。学习方式：**先用 Spring 写出能运行的功能 → 遇到对应机制时用 JVM/JMM 解释 → 做小实验验证**。不必等 JVM 全学完才开始 Spring。

---

## 手写练习

`javac Test.java && javap -c -v Test`，观察 `iload/istore/iadd` 与 `stack=`、`locals=` 的值，并对照本文解释为什么是这两个数。

---

> 来源：原始 `java/20260916B2.md` 第 4～16 节（javap、字节码、栈/寄存器虚拟机、栈帧、局部变量表、操作数栈、StackOverflowError）；《Java 后端学习笔记：JVM 栈帧、Spring 并行学习与 VS Code 环境》(`java/_archive/20261010_Java后端学习笔记_JVM栈帧Spring与VSCode.md` §一-三，JVM 排障映射与主辅线学法）
