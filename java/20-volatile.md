# 20 · volatile

> 一句话概括：`volatile` 保证共享变量的**可见性**和一定的**有序性**，但**不保证复合操作的原子性**、也不提供互斥——典型用途是「一个线程写状态，其他线程读状态」。
> （底层原理与 happens-before 推导 → `19-JMM与内存可见性.md`）

---

## 一、volatile 是什么

> `volatile = 可见性 + 禁止部分重排序，但不保证复合操作原子性。`

它解决的是：一个线程改了共享变量，其他线程**看不到旧值卡住**的问题。

```java
class Task {
    private volatile boolean running = true;

    public void stop() { running = false; }

    public void run() {
        while (running) { /* 工作 */ }    // 能看到别的线程改的 false
    }
}
```

不加 `volatile` 时，循环线程理论上可能一直读到自己缓存的旧值 `true`，停不下来。

---

## 二、为什么不能保证原子性

`count++` 是**复合操作**：

```text
读 count → count + 1 → 写回 count
```

```java
volatile int count = 0;
count++;            // 仍然线程不安全！
```

交叉执行：A 读 0、B 读 0 → A 写 1、B 写 1 → 结果 1，不是 2。

> **volatile 只保证「看得见最新值」，不保证「读-改-写」这整段不被插入。**

### 判断方法

看操作是不是「读 → 改 → 写」：

| 操作 | 是否安全 |
|---|---|
| `value = 10;`（单次写） | ✅ volatile 语义足够 |
| `int x = value;`（单次读） | ✅ |
| `value++`、`value += 10` | ❌ 复合操作 |
| `if (value == 0) value = 1;` | ❌ 判断 + 修改 |

**要原子性，改用什么？** `AtomicInteger.incrementAndGet()` / `synchronized` / `Lock`。

> 进阶坑（见 `17`）：**多个原子操作组合起来也不原子**——
> `if (count.get() < 10) count.incrementAndGet();` 的 check-then-act 整体仍有竞态。

---

## 三、有序性

```java
int data = 100;
ready = true;        // volatile 写
```

volatile 写在它前面的普通写之间形成屏障语义：`data = 100` **不会被重排**到 `ready = true` 之后。因此别的线程看到 `ready == true` 时，必然也能看到 `data == 100`（完整推导见 `19` 第五节）。

---

## 四、volatile vs synchronized

| 特性 | `volatile` | `synchronized` |
|---|---|---|
| 可见性 | ✅ | ✅ |
| 有序性 | ✅ | ✅ |
| 复合操作原子性 | ❌ | ✅ |
| 互斥 | ❌ | ✅ |
| 等待锁 | ❌ | ✅ |
| 典型用途 | **状态标志** | 临界区、多步共享数据修改 |

---

## 五、怎么用：判断原则

```text
一个线程改状态 + 其他线程读状态        → volatile 合适
多个线程都要「读-改-写」               → Atomic / synchronized / Lock
```

适合：`volatile boolean shutdown;`、`volatile boolean running;`

不适合：`volatile int money; money -= 100;`（复合操作，得用锁或原子类）

---

## 📌 易错点

1. **以为 volatile 能代替锁** ✗——它不提供互斥。
2. **以为 `volatile int count; count++` 线程安全** ✗——经典陷阱，仍在丢更新。
3. **把可见性当成原子性**——可见性是「能不能看到最新值」，原子性是「能不能被插入」。
4. **把 volatile 理解成「每次直接读写物理内存」** ✗——教学模型而已，实际是 JMM 语义。
5. **以为「一个原子操作 + 一个原子操作」还是原子** ✗——组合后整体可能非原子。

---

## 🧠 速记口诀

```text
volatile：可见性 + 有序性，不互斥、不保复合原子
count++ 加 volatile 照样丢更新
一个线程写、其他线程读 → volatile；都要读改写 → Atomic/锁
单次读写 OK，读-改-写不行
```

---

## ✅ 自测问题

1. volatile 保证什么、不保证什么？各举一个代码例子。
2. `volatile int count; count++` 为什么仍然不安全？用「读-改-写」解释。
3. 怎么判断一个操作该用 volatile 还是 synchronized/Atomic？
4. volatile 与 synchronized 在互斥、可见性、原子性上有什么差异？
5. 为什么停止线程的标志位适合用 volatile？
6. `volatile` 的有序性保证在 `data/ready` 例子里起到了什么作用？

---

## 手写练习

**必写**：① `volatile boolean running` 停止标志，观察能否正常退出；② volatile 计数器错误案例，多线程 `count++` 观察结果偏小，再分别用 `AtomicInteger` / `synchronized` 改写对比。

---

> 来源：原始 `java/20260916B1.md`（volatile 学习笔记）、`20260916B3.md`（volatile 相关部分，已去重合并）
