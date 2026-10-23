---
id: algo-linked-list-lru
title: 链表的边界、原地恢复与 LRU
description: 通过指针角色理解反转、分组、回文和缓存淘汰。
course: algorithms
order: 4
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# 链表的边界、原地恢复与 LRU

## 每次改边之前先保留入口

单链表不能随机向前找前驱。头节点可能变化时使用 dummy；改 `cur.next` 前保存旧后继；按组操作先确认整组存在，再反转，最后接回前后两段。变量名 pre 不自带含义，必须说清它是“当前前驱”还是“已反转段的新头”。

```java
ListNode reverse(ListNode head) {
    ListNode prev = null, cur = head;
    while (cur != null) {
        ListNode next = cur.next;
        cur.next = prev;
        prev = cur;
        cur = next;
    }
    return prev;
}
```

不变量：prev 是已反转前缀，cur 是未处理后缀入口。空链表直接返回 null。时间 O(n)，额外空间 O(1)；递归反转不能声称 O(1) 空间。

## 删除、加法与分组

删除倒数 n 个节点，fast/slow 都从 dummy 开始，fast 先走 n 步，再同步移动直到 fast.next==null，slow 停在待删除节点前。此模板依赖 1<=n<=长度；一般业务方法应拒绝不合法参数。也可领先 n+1 步并让 fast==null 停止，步数与终止条件必须成对出现。

两数相加的链表若按个位优先存储，直接顺序加即可，循环条件包含两条链未结束或 carry 非零；短链空位视作 0。比如 9→9 与 1 相加得到 0→0→1，不要转换成固定宽度整数再运算。

K 组反转把待处理段写成 `pre → [K节点] → nextGroup`。保存旧头（即新尾），用 nextGroup 作为局部反转初始 prev；完成后 pre.next 接新头，pre 移到旧头。不足 K 个保留原样，K<=0 拒绝。两两交换就是 K=2 的特例。

## 回文判断需要恢复输入

快慢指针找到前半段末尾，反转后半段，比较，再将后半段反转回来接回。不应在首次不同处直接 return，必须统一恢复。奇数中点无需参与比较；比较次数以右半段长度为准。`1→2→3→2→1` 比较的是 1/1、2/2。

这是 O(n) 时间、O(1) 辅助空间，但会在函数执行期间暂时修改输入。若多个线程共享链表，恢复之后也不能抹去期间的竞态，需要互斥或使用复制方案。原稿“从中点直接反转也行”的方案须明确较长半段也会比较中心自身，不能与“比较较短后半段”的解释混用。

## LRU：把查找与顺序交给不同结构

后台商品详情缓存需要按键 O(1) 平均查找，也要快速移动“最近访问”位置。哈希表定位节点，双向链表维护顺序：get 命中移到头；put 已存在更新并移头，新增后超容量从尾淘汰，并同时从 map 删除。

用头尾哨兵统一插入删除，封装 `remove(node)` 和 `addFirst(node)`；删除必须同时修正前后两侧指针。容量为 0 时不能误删哨兵。哈希操作平均常数，节点移动常数；总体空间 O(capacity)。

进程内 LRU 不等于分布式缓存，未包含过期时间、内存字节上限、缓存击穿与并发一致性。Java 17 的 LinkedHashMap 可按访问顺序维护，但线程安全仍需另行设计。面试练习手写结构，工程优先评估成熟缓存实现。

## 参考资料

- [Java 17 LinkedHashMap：访问顺序和淘汰钩子](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/LinkedHashMap.html)
- [Python collections：deque 与有序容器](https://docs.python.org/3/library/collections.html)

## 模拟面试问题

1. 基础理解：反转链表时 prev、cur、next 各自保存什么？
2. 实际应用：K 组反转后 pre 为什么移到旧头而非新头？
3. 实际应用：回文判断提前失败时怎样保证输入恢复？
4. 实际应用：LRU 淘汰为什么必须同步修改哈希表与链表？
5. 深入追问：原地修改后恢复为何不能天然保证并发安全？
