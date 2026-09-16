# 19 · JMM 与内存可见性

> 一句话概括：**JMM（Java 内存模型）** 规定多线程如何读写共享变量；可见性、原子性、有序性三大问题的答案是 **happens-before 规则**，`volatile`/`synchronized`/`Atomic` 都是它的具体手段。
> （`volatile` 关键字单独一篇 → `20-volatile.md`）

---

## 一、JMM 是什么

> JMM 定义了**一个线程对共享变量的修改，在什么条件下必须被其他线程看到**。

抽象模型：

```text
            主内存 Main Memory
                  │
      ┌───────────┴───────────┐
线程 A 工作内存          线程 B 工作内存
（共享变量的副本）        （共享变量的副本）
```

线程改共享变量，可以抽象成「主内存 → 工作内存副本 → 计算 → 写回」。于是可能出现 A 看到 `count=1`、B 还看到 `count=0`——**可见性问题**。

> ⚠️ **「工作内存」只是 JMM 的抽象概念**，不能等同于 CPU L1/L2/L3 缓存或寄存器。JMM 描述的是**语言层面规则**，具体由 JVM 和硬件实现。

### JMM ≠ JVM 内存区域（高频混淆）

| | 回答什么问题 | 包含 |
|---|---|---|
| **JVM 内存结构** | 运行时数据**放在哪里** | 堆、虚拟机栈、程序计数器、方法区/元空间、本地方法栈 |
| **JMM** | 多线程**如何观察和修改**共享数据 | 可见性、原子性、有序性、happens-before |

---

## 二、三大特性

| 特性 | 含义 | 典型破坏原因 |
|---|---|---|
| **可见性** | 一个线程改了，别的线程能及时看到 | 各自的工作内存副本没同步 |
| **原子性** | 一组操作不可被其他线程插入（看不到中间态） | `count++` 是读-改-写三步 |
| **有序性** | 代码顺序 ≠ 实际执行顺序时，语义仍符合预期 | 编译器/JIT/CPU 的**指令重排序** |

---

## 三、重排序与 as-if-serial

**as-if-serial**：单线程下，编译器和 CPU 可以任意重排，只要最终**看起来像按代码顺序执行**。

```java
a = 1;
b = 2;
System.out.println(a + b);   // 单线程结果恒为 3，怎么重排都行
```

> **as-if-serial 只保证单线程语义，不足以保证多线程正确。**单线程里允许的重排，在另一线程看来可能就是错误顺序。

---

## 四、happens-before

> **A happens-before B** ⇒ JMM 保证 A 的结果对 B 可见，且顺序关系成立。

⚠️ 它**不等于「现实时间上 A 先跑完」**，是内存模型层面的可见性 + 顺序保证。

### 六条常用规则

| 规则 | 内容 |
|---|---|
| 程序次序 | 同一线程内，前面的操作 happens-before 后面的 |
| **volatile** | 对 volatile 变量的**写** happens-before 后续对同一变量的**读** |
| **锁** | 解锁 happens-before 后续对**同一把锁**的加锁 |
| 线程启动 | `start()` 之前的操作 happens-before 新线程的所有操作 |
| 线程终止 | 线程的所有操作 happens-before `join()` 成功返回后的代码 |
| **传递性** | A→B 且 B→C ⇒ A→C（最难也最有用的一条） |

---

## 五、经典推导：为什么 `data=100` 一定可见

```java
class Demo {
    int data = 0;
    volatile boolean ready = false;   // 关键：volatile
    void write() { data = 100; ready = true; }
    void read()  { if (ready) System.out.println(data); }
}
```

线程 A 写、线程 B 读。B 一旦看到 `ready == true`，**必然**能看到 `data == 100`。原因不是「源码里 data 写在前」——而是这条链：

```text
A：data = 100
     ↓ 程序次序规则
A：volatile 写 ready = true
     ↓ volatile 规则
B：volatile 读 ready == true
     ↓ 程序次序规则
B：读 data
     ↓ 传递性
A 的 data 写 happens-before B 的 data 读 ✅
```

同时 volatile 的内存语义**禁止**把 `data = 100` 重排到 volatile 写之后，否则整个保证就被破坏了。

> **如果 `ready` 不是 volatile**：没有跨线程 happens-before，看到 `ready==true` **推不出** `data==100`——这是最经典的误区。

---

## 六、synchronized 与 JMM

`synchronized` 不只是「加锁」，它同时提供：

```text
互斥（同一时刻一个线程进临界区）
+ 可见性（unlock happens-before 后续 lock）
+ 有序性
```

所以 A 在同步块里改的变量，B 进入**同一把锁**的同步块时一定能看到。

> 提醒：可见性成立的前提是**同一把锁**——用两把不同的锁，既不互斥也没有可见性保证（见 `14`）。

---

## 七、知识链

```text
              JMM
     ┌─────────┼─────────┐
  volatile  synchronized  Atomic/CAS
     │           │            │
  可见性      互斥+可见      原子更新
  有序性      有序性         （基于 JMM）
```

后续深入：AQS / JUC 容器 → 内存屏障 → 缓存一致性协议（MESI）。

---

## 📌 易错点

1. **「volatile 变量直接从物理主内存读」** ✗——那是教学简化；JMM 给的是**语义保证**，不机械等价于每次访问 RAM。
2. **以为源码顺序就能推出跨线程可见** ✗——必须靠 happens-before 链（要同步关系才成立）。
3. **happens-before 当成时间先后** ✗——它是可见性与顺序保证，不是墙上时钟。
4. **混淆 JMM 与 JVM 内存区域**——一个管「多线程怎么看数据」，一个管「数据放哪」。
5. **以为 as-if-serial 能保证多线程正确** ✗——它只管单线程。

---

## 🧠 速记口诀

```text
JMM 管三性：可见、原子、有序
JMM 是规则，不是缓存；JVM 内存区域才管数据放哪
happens-before 六条：程序次序 / volatile / 锁 / start / join / 传递性
经典链：data 写 → volatile 写 → volatile 读 → data 读（靠传递性）
as-if-serial 只保单线程，多线程得靠同步
```

---

## ✅ 自测问题

1. JMM 和 JVM 内存区域分别回答什么问题？各包含哪些内容？
2. 可见性、原子性、有序性各指什么？`count++` 破坏的是哪一个？
3. as-if-serial 保证什么？它对多线程足够吗？
4. 说出 happens-before 的六条规则，并指出哪一条让跨线程推导成立。
5. 若 `ready` 不是 volatile，B 看到 `ready==true` 时能否确定看到 `data==100`？为什么？
6. 用 happens-before 链推导一遍 `data/ready` 例子的完整过程。
7. `synchronized` 除了互斥还提供什么保证？前提是？

---

> 来源：原始 `java/20260916B3.md`（JMM 学习笔记）
