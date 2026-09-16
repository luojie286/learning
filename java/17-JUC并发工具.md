# 17 · JUC 并发工具

> 一句话概括：在 `14` 的线程与 `synchronized` 基础上，JUC 提供了一套更完整的并发工具箱——CAS 无锁原子操作、`ReentrantLock` 灵活锁、`ConcurrentHashMap` 并发容器、`BlockingQueue` 阻塞队列、`ThreadPoolExecutor` 线程池、`CompletableFuture` 异步编排。

---

## 一、知识主线：从 synchronized 到 JUC

```text
Thread / Runnable → 多线程运行 → 共享数据 → 线程安全问题
        ↓                                    ↓
   synchronized（悲观：先加锁）        CAS（乐观：先试，失败重试）
        ↓                                    ↓
     锁竞争                              Atomic 原子类（无锁并发）
                                             ↓
                              ReentrantLock（更灵活的锁控制）
                                             ↓
              wait / notify（底层协作思想）→ BlockingQueue（封装好的协作工具）
                                             ↓
              ThreadPoolExecutor（工作线程 + 任务队列）→ CompletableFuture（异步编排）
```

> 阅读顺序建议：`14-多线程与锁`（基础）→ 本篇 → `18-网络编程基础`（综合应用）。

---

## 二、CAS：Compare And Swap

**一句话理解**：修改数据前，先检查它还是不是我之前读到的那个值；是才改，不是就重试。

```java
// 逻辑近似（真实实现是 CPU 原子指令，不是这样的 if）
if (当前值 == 预期值) {
    当前值 = 新值;   // 成功
} else {
    // 失败：重新读取、重新尝试
}
```

典型过程：

```text
线程 A 读取 count = 10 ─┐
线程 B 读取 count = 10 ─┘
A：期望 10，当前还是 10 → 改成 11，成功
B：期望 10，发现已是 11 → 失败，重新读取再试
```

### CAS vs synchronized

| | `synchronized` | CAS |
|---|---|---|
| 思想 | **悲观**：先加锁，只放一个线程进来 | **乐观**：不加锁先改，失败再重试 |
| 阻塞 | 拿不到锁的线程阻塞等待 | 不阻塞，失败就自旋重试 |
| 适用 | 临界区较复杂、竞争激烈 | 简单原子操作（如计数） |

### ABA 问题

值经历了 `A → B → A`，CAS 只看到「之前是 A、现在还是 A」，误以为没变过。当前阶段**知道存在即可**；后续学习 `AtomicStampedReference`（版本号方案）。

---

## 三、Atomic 原子类

JUC 用原子类把常见 CAS 操作封装好，最常用 `AtomicInteger`：

```java
AtomicInteger count = new AtomicInteger(0);

count.incrementAndGet();              // 相当于 ++count（先加再取）
count.getAndIncrement();              // 相当于 count++（先取再加）
count.compareAndSet(10, 20);          // 当前值是 10 就改成 20，返回是否成功
```

常用方法速查：

| 方法 | 含义 |
|---|---|
| `get()` / `set(v)` | 读 / 写 |
| `incrementAndGet()` | `++count` |
| `getAndIncrement()` | `count++` |
| `decrementAndGet()` | `--count` |
| `addAndGet(5)` | 加 5 后取值 |
| `compareAndSet(exp, new)` | CAS 的直接暴露 |

> 亲测记忆法：方法名里 `get` 在前 = 先取旧值（后缀行为），`get` 在后 = 先完成操作（前缀行为）。

### 进阶坑：多个原子操作组合 ≠ 原子

```java
if (count.get() < 10) {          // ① 判断（原子）
    count.incrementAndGet();      // ② 自增（原子）
}
```

①②各自都原子，但**「判断 + 修改」整体不是**：`count = 9` 时 A、B 都判断通过 → 自增后变成 11。这是典型的 check-then-act 竞态。

> **原子类只保证单个方法调用原子，不保证你组合出来的逻辑原子。** 这种场景要么用 `synchronized` 包住整段判断 + 修改，要么用能一次完成「判断 + 更新」的方法（循环 CAS / `updateAndGet`）。

---

## 四、ReentrantLock 进阶：tryLock

基本用法（`lock()` + `finally` + `unlock()`）在 `14` 已讲过。JUC 阶段重点是它比 `synchronized` 多出来的能力：

```java
if (lock.tryLock()) {          // 能拿到就拿，拿不到立刻返回 false，不等
    try {
        // 操作共享数据
    } finally {
        lock.unlock();
    }
}
// 拿不到锁时的处理逻辑写在这里，而不是干等
```

| | `synchronized` | `ReentrantLock` |
|---|---|---|
| 释放锁 | JVM 自动（出临界区即释放） | 手动 `unlock()`，必须放 `finally` |
| 尝试拿锁 | 不支持 | `tryLock()` 拿不到不等待 |
| 可中断 / 超时 | 不支持 | `lockInterruptibly()`、`tryLock(time, unit)` |
| 公平锁 | 非公平 | 构造参数可选公平/非公平 |
| 日常选择 | 简单场景优先用它 | 需要上述高级能力时再用 |

---

## 五、ConcurrentHashMap

`HashMap` 本身**不是线程安全的**，多线程同时修改可能丢数据甚至死循环（JDK7 及以前）。JUC 提供线程安全的 `ConcurrentHashMap`：

```java
ConcurrentHashMap<String, Integer> map = new ConcurrentHashMap<>();
map.put("java", 1);
map.get("java");
```

选择原则：

```text
单线程 / 无并发 → HashMap
多线程共享 Map → ConcurrentHashMap
```

底层实现（CAS + synchronized 锁桶、源码演进）**后续学习**，当前会用即可。

---

## 六、生产者-消费者：为什么需要队列

```text
生产者线程 → [ 任务1 任务2 任务3 ] → 消费者线程
```

没有队列时：生产者必须等消费者处理完才能继续，两边强耦合。
有队列后：生产者只管往里放，消费者只管往外取，**队列起解耦和缓冲的作用**——两边速度不必匹配。

这个模型是理解 `BlockingQueue` 和线程池的钥匙。

---

## 七、BlockingQueue

**一句话理解**：线程安全的队列 + 队列**满时生产者自动阻塞等待空位** + 队列**空时消费者自动阻塞等新数据**。

| 方法 | 队列满/空时 | 说明 |
|---|---|---|
| `put(e)` | **阻塞等待**空位 | 生产 |
| `take()` | **阻塞等待**数据 | 消费 |
| `offer(e)` | 立刻返回 `false` | 非阻塞生产 |
| `poll()` | 立刻返回 `null` | 非阻塞消费 |

> 简记：**put/take 会等，offer/poll 不等**。另外 `offer`/`poll` 还有带超时的重载版本（等多久还失败就放弃）。

### 生产者-消费者示例

```java
BlockingQueue<Integer> queue = new ArrayBlockingQueue<>(5);

Thread producer = new Thread(() -> {
    try {
        for (int i = 1; i <= 10; i++) {
            queue.put(i);                    // 队列满则阻塞
            System.out.println("生产：" + i);
        }
    } catch (InterruptedException e) {
        Thread.currentThread().interrupt();  // 恢复中断状态
    }
});

Thread consumer = new Thread(() -> {
    try {
        for (int i = 1; i <= 10; i++) {
            int value = queue.take();        // 队列空则阻塞
            System.out.println("消费：" + value);
        }
    } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
    }
});
```

### 与 wait / notify 的关系

自己用 `synchronized` + `wait()` + `notify()` 实现生产者-消费者：消费者没数据就 `wait()`，生产者放入后 `notify()`（反向同理）。细节多、极易写错。

> **`wait/notify` 是底层协作思想（理解原理），`BlockingQueue` 是封装好的工具（要会用）。**

---

## 八、ThreadPoolExecutor

**为什么需要**：避免「每来一个任务就 `new Thread()`」的创建/销毁开销。基本思想：提前备好工作线程，任务先入队，线程循环取任务执行。

### 核心构造参数（7 个）

```java
new ThreadPoolExecutor(
    corePoolSize,      // 核心线程数：常驻
    maximumPoolSize,   // 最大线程数：核心 + 临时扩容
    keepAliveTime, unit, // 临时线程空闲多久回收
    workQueue,         // 任务队列（BlockingQueue<Runnable>）
    threadFactory,     // 线程工厂
    handler            // 拒绝策略
);
```

### 任务处理流程（必须记住的顺序）

以 `core=2, max=4, queue=3` 为例，任务持续到来：

```text
任务1、2 → 核心线程1、2
任务3、4、5 → 进队列
任务6、7 → 队列满了！创建临时线程3、4
任务8 → 到达 max，触发拒绝策略
```

> **核心线程 → 任务队列 → 临时线程 → 拒绝策略**。易错点：是**队列满了才创建临时线程**，不是先扩线程再入队。

### 拒绝策略

| 策略 | 行为 |
|---|---|
| `AbortPolicy`（默认） | 直接抛异常 |
| `CallerRunsPolicy` | 提交任务的线程自己执行（变相限流） |
| `DiscardPolicy` | 静默丢弃新任务 |
| `DiscardOldestPolicy` | 丢弃队列最老任务 |

### 与 BlockingQueue 的关系

线程池内部就是生产者-消费者：`execute(task)` 的调用者是**生产者**，工作线程是**消费者**，中间就是构造参数里的 `BlockingQueue<Runnable> workQueue`。

> `Executors.newFixedThreadPool()` 等工厂方法本质也是拼这些参数；实际开发推荐手动 `new ThreadPoolExecutor(...)`，参数自己可控。

---

## 九、CompletableFuture

用于异步任务的执行与结果编排：

```java
CompletableFuture
    .supplyAsync(() -> "java")            // 异步执行，产出结果
    .thenApply(s -> s.toUpperCase())      // 转换结果
    .thenAccept(System.out::println);     // 消费结果

String r = future.join();                 // 阻塞取结果（不抛受检异常）
```

执行链：**异步执行 → 得到结果 → 转换 → 消费**。当前阶段重点方法：`supplyAsync()`、`thenApply()`、`thenAccept()`、`allOf()`、`join()`。

> `join()` 和 `get()` 都能取结果，区别：`get()` 抛受检异常（要 try-catch），`join()` 抛非受检异常（写起来干净）。

---

## 十、学习优先级与手写清单

| 梯队 | 内容 |
|---|---|
| 必须掌握 | `synchronized`、`volatile`、CAS、`AtomicInteger`、`ReentrantLock`、`ConcurrentHashMap`、`BlockingQueue`、`ThreadPoolExecutor` |
| 会基本使用 | `CompletableFuture`、`CountDownLatch`、`Semaphore` |
| 暂不深入 | AQS / StampedLock / Phaser / ForkJoinPool 源码、`LongAdder` 源码、lock-free 算法、`ConcurrentHashMap` 源码细节 |

**必须手写**：

1. `AtomicInteger` 多线程计数（对比裸 `count++` 丢更新）
2. `ReentrantLock` 的 `lock / try / finally unlock` 模板
3. 一个生产者 + `ArrayBlockingQueue(1)` + 一个消费者（给消费者加 `sleep`，观察「消费慢 → 队列满 → 生产者阻塞」）
4. 线程池任务处理流程（core → queue → 临时线程 → 拒绝）
5. TCP 客户端/服务端（见 `18`）

---

## 📌 易错点

1. **`incrementAndGet()` 和 `getAndIncrement()` 记反**——前者 `++count`，后者 `count++`。
2. **以为线程池先扩线程再入队**——顺序是核心线程 → 队列 → **队列满了才**创建临时线程。
3. **生产者-消费者用 `offer`/`poll`**——它们不阻塞，起不到「等对方」的作用，应该用 `put`/`take`。
4. **`unlock()` 没放 `finally`**——临界区抛异常后锁永远不释放（同 `14`）。
5. **以为 CAS 万能**——高竞争下 CAS 会不停失败重试（自旋浪费 CPU），且有 ABA 问题；竞争激烈时 `synchronized`/`LongAdder` 反而更合适。

---

## 🧠 速记口诀

```text
CAS：先看变没变，没变才换；失败就重试，无锁靠乐观
Atomic 封装 CAS：get 在前是后加（count++），get 在后是先加（++count）
tryLock：拿得到就干，拿不到就走，不傻等
BlockingQueue：put/take 满空会等，offer/poll 立刻返回
线程池四步：核心 → 队列 → 临时线程 → 拒绝
CompletableFuture：supply 产、thenApply 转、thenAccept 用、join 取
```

---

## ✅ 自测问题

1. CAS 和 `synchronized` 分别代表什么并发思想？失败后的行为有什么不同？
2. ABA 问题是什么？为什么普通的 CAS 发现不了？
3. `getAndIncrement()` 和 `incrementAndGet()` 分别对应哪个表达式？
4. `tryLock()` 解决了 `synchronized` 的什么缺陷？返回 `false` 后程序该怎么继续？
5. `HashMap` 和 `ConcurrentHashMap` 的选用原则是什么？
6. `put/take` 与 `offer/poll` 六个方法在「队列满/空」时行为分别是什么？
7. 线程池收到第 8 个任务（core=2, queue=3, max=4）时会发生什么？说出完整四步。
8. `CallerRunsPolicy` 是谁在执行被拒绝的任务？这有什么副作用（提示：提交线程会被占住）？
9. 线程池内部哪个结构体现了生产者-消费者模型？

---

> 来源：原始 `java/20260915C.md` 第一～十三节、二十七～二十九节（JUC 部分）；网络编程部分已拆入 → `18-网络编程基础.md`
