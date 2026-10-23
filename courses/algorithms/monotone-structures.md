---
id: algo-monotone-structures
title: 单调结构与前后缀最值
description: 用支配关系解释弹出，用下标管理窗口与重复元素。
course: algorithms
order: 10
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# 单调结构与前后缀最值

## 栈里放尚未解决的候选

求右侧第一个严格更大值，栈中保存尚未遇到答案的下标，对应值从底到顶不增。新值更大时弹出，当前值就是被弹位置首次遇到的严格更大值。例 `[2,1,5,6,2,3]` 得 `[5,5,6,-1,3,-1]`。

```python
def next_greater(a):
    ans = [-1] * len(a); stack = []
    for i, value in enumerate(a):
        while stack and a[stack[-1]] < value:
            ans[stack.pop()] = value
        stack.append(i)
    return ans
```

每个下标只入栈一次、出栈一次，嵌套 while 仍是 O(n)。存下标还能计算距离，而只存值无法分清重复值位置。`<` 改成 `<=` 改变了严格性，不能凭记忆替换。

## 相等元素不是永远拥有相同答案

原稿“相等元素答案相同”应限定为当前单调栈中同组的候选。序列 `[2,1,2]` 的两个 2 的左右严格更小位置不同，不能把整个数组的相等值都合并。若一趟同时结算左右严格更小，需维护相等组，左答案取前一个严格更小组中最靠右的下标；也可用两趟分别求左右，降低实现复杂度。

## 窗口最值的支配关系

窗口最小值中，新元素 x 不大于旧队尾 y，且 x 更晚过期：只要 y 还在窗口，x 也在，y 永远不可能优于 x，可安全删除。队尾负责剔除劣候选，队首负责移除过期下标。

```python
from collections import deque

def sliding_min(a, k):
    if k <= 0 or k > len(a): raise ValueError('窗口范围无效')
    q = deque(); out = []
    for i, x in enumerate(a):
        while q and q[0] <= i-k: q.popleft()
        while q and a[q[-1]] >= x: q.pop()
        q.append(i)
        if i >= k-1: out.append(a[q[0]])
    return out
```

先过期再维护尾，或先维护尾后过期均可，只要取答案前不变量成立。不能把某一种操作顺序说成唯一正确。时间 O(n)，空间 O(k)。

## 从接雨水理解前后缀信息

每格水量 `min(leftMax[i],rightMax[i])-height[i]`，前后缀最大值都含自身。`[3,0,2,0,4]` 每格贡献 `[0,3,1,3,0]`，总数 7；两端不能存水。先用 O(n) 空间数组实现容易验证，再用左右指针和已知边界压到 O(1) 辅助空间。它是静态柱高模型，不要直接类比有渗透、流速与二维地形的实际洪水。

## 开发场景与拓展

每分钟统计过去十分钟的最高延迟，可以对等间隔样本维护窗口最大队列。时间戳不等间隔时，过期规则应按时间而非固定下标；分布式乱序数据需要事件时间与迟到处理，单一数组模板不能自动覆盖。柱状图最大矩形还需要在弹出时按左右边界算宽度，重复高度策略应与边界定义一起证明。

## 参考资料

- [Python collections.deque：两端操作](https://docs.python.org/3/library/collections.html#collections.deque)
- [Princeton Algorithms 作者的 Stack](https://github.com/kevin-wayne/algs4/blob/master/src/main/java/edu/princeton/cs/algs4/Stack.java)

## 模拟面试问题

1. 基础理解：嵌套弹栈循环为什么总时间仍为 O(n)？
2. 实际应用：相等值什么时候能分为一组，什么时候不能？
3. 实际应用：队尾淘汰与队首过期分别依赖什么证明？
4. 深入追问：按时间窗口统计与按 K 个样本统计有什么差别？
5. 深入追问：接雨水如何从前后缀数组压缩为双指针？
