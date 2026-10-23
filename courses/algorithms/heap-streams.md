---
id: algo-heap-streams
title: 堆、Top K 与多路有序流
description: 区分建堆、排序方向与只保留当前候选的流式归并。
course: algorithms
order: 3
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# 堆、Top K 与多路有序流

## 堆提供局部顺序

小顶堆保证父节点不大于孩子，因而堆顶最小，但兄弟和其他位置不必有序。零基数组中孩子是 `2*i+1`、`2*i+2`，父下标仅在 i>0 时为 `(i-1)//2`。新节点从尾部上浮；弹出堆顶先用尾元素替代，再下沉，与较小孩子交换。

逐个插入 O(n log n)，自底向上下沉建堆 O(n)：高度很高的节点少，所有节点可下沉高度之和是线性的。单次 push/pop O(log n)，peek O(1)。遍历 PriorityQueue 得不到完整排序，要重复 poll 才能按优先级消费。

## 两种升序输出为什么使用相反的堆

把堆顶逐次弹到新数组，升序用小顶堆，最小的先输出。原地堆排序把堆顶换到有效区域尾部，升序应使用大顶堆，最大的先放到最终位置。后者 O(n log n) 时间、迭代下沉 O(1) 辅助空间，通常不稳定；用递归下沉则还需计入调用栈。

原稿保留的 C++ 修复有价值：自由模板函数应在调用可见的位置声明；原地升序不能把选择大顶堆的布尔参数取反。描述修复时区分语言查找规则和排序逻辑，不能单凭输出方向猜编译是否成功。

## 开发例子：合并多机日志

三台服务器分别产生有序时间流 `[1,8]`、`[2,4]`、`[3,6]`。先把三个头 1、2、3 入堆；弹出 1 后只补入该流后继 8。之后依次得到 2、3、4、6、8。任何流未消费部分的最小值总在它自己的头部，因此堆最多放 K 个候选即可。

```python
from heapq import heappush, heappop

def merge_streams(streams):
    heap = []
    for sid, stream in enumerate(streams):
        it = iter(stream)
        first = next(it, None)
        if first is not None:
            heappush(heap, (first, sid, it))
    while heap:
        value, sid, it = heappop(heap)
        yield value
        nxt = next(it, None)
        if nxt is not None:
            heappush(heap, (nxt, sid, it))
```

示例流只含数值，None 用作结束标记。sid 用来在值相等时比较，避免比较不可排序的迭代器。总 N 个元素，O(N log(K+1)) 时间、O(K) 状态；调用方若收集所有输出仍需 O(N) 输出空间。

## Top K 与优先级更新

保留最大的 K 项，用容量 K 的小顶堆：堆顶是当前入选者中最弱的，新的更大项才替换它。全部取出后仍要排序才是降序列表。K 为 0 时直接返回，不读堆顶。

修改已经入堆对象的优先级不会自动修复堆；可移除后重入，或使用版本号和懒删除，读取时跳过过期项。Java 比较器使用 `Integer.compare`，直接相减有溢出风险。Python 3.13 可对数值取负模拟大顶堆；较新版本的最大堆 API 不能直接假设本机已有。

## 参考资料

- [Python heapq：堆不变量与优先队列注意事项](https://docs.python.org/3/library/heapq.html)
- [Java 17 PriorityQueue](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/PriorityQueue.html)

## 模拟面试问题

1. 基础理解：为什么堆顶有序不代表遍历有序？
2. 基础理解：自底向上建堆为什么是线性时间？
3. 实际应用：合并 K 个日志流为什么只需保留 K 个头？
4. 实际应用：原地升序与弹出到新数组的升序分别使用什么堆？
5. 深入追问：如何处理入堆后优先级变化及过期元素？
