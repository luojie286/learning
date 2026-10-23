---
id: algo-graphs-dependencies-mst
title: 依赖排序与最小生成树
description: 区分有向先后关系、无向连通性以及总成本最小化。
course: algorithms
order: 9
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# 依赖排序与最小生成树

## 拓扑序回答先后关系

有向边 A→B 表示 A 必须先完成。Kahn 算法统计入度，将所有零入度节点入队；完成一个节点，削减其出边目标的入度，刚降到零才入队。输出数不足 V 说明存在有向环。残余节点也可能只是被环阻塞，不代表每个残余节点自身都在环上。

课程依赖 `基础→数据库`、`基础→Web`、`数据库→项目`、`Web→项目`，合法顺序可为基础、数据库、Web、项目，也可交换中间两项。拓扑序通常不唯一，要求字典序最小时使用小顶堆。普通队列 O(V+E)，小顶堆版本额外考虑堆操作。

```python
from collections import deque

def topological_order(graph):
    indegree = {u: 0 for u in graph}
    for u in graph:
        for v in graph[u]: indegree[v] += 1
    q = deque(u for u, d in indegree.items() if d == 0)
    order = []
    while q:
        u = q.popleft(); order.append(u)
        for v in graph[u]:
            indegree[v] -= 1
            if indegree[v] == 0: q.append(v)
    if len(order) != len(graph): raise ValueError('存在有向依赖环')
    return order
```

例子约定所有目标也在 graph 键中，孤立任务也必须登记。Kahn 可以使用队列，但关键是依赖消除，不能把它和按距离分层 BFS 当成完全相同的问题。任务并发执行时，只有实际完成后才算依赖解除。

## 最小生成树回答总成本

连通带权无向图的生成树有 V-1 条边，MST 使总边权最小。Kruskal 全局按边权升序，用并查集跳过已连通两端，加入有效边。Prim 从一个已选点集扩张，每次挑跨割的最低代价边；它的 key 是连接到树的最小边权，不是从起点走来的累计距离。

三角形 A-B=2、B-C=3、A-C=9，Kruskal 选 2 与 3，总成本 5；Prim 从 A 开始也先选 B，再由 B 以 3 连接 C。若只按“离起点 A 最近的直连边”不更新 C，就会错误选择 9。

Kruskal O(E log E) 加并查集成本；Prim 邻接表配可重复候选的二叉堆通常 O(E log E)，常简写 O(E log V) 须说明简单图等假设；稠密图可用 O(V²) 数组版。求和使用足够宽的整数，允许负边权也能求 MST。

## 工程例子与常见误区

给机房布线，需要连接全部交换站且总铺设成本最小，是 MST 抽象；为某个站到其他站的访问延迟最小，是最短路，两者目标不同。MST 不保证路径最短，也不保证冗余可靠性，实际网络设计常需额外备用链路约束。

Kruskal 最终不足 V-1 条表示原图不连通，不是“出现环”。不连通时仍可得到最小生成森林；页面应明确任务是返回森林还是报无生成树。边权相同可能存在多棵 MST，不能仅凭结果边集合不一样就判错，可比较总权及树性质。

村庄 ID 稀疏时可用哈希表离散化成连续下标再进并查集，保留反向映射以输出原 ID。孤立村庄也必须从完整顶点清单登记，只扫描边端点会漏掉它们，进而错误地宣称图连通。

## 参考资料

- [Python graphlib：拓扑排序与完成通知](https://docs.python.org/3/library/graphlib.html)
- [Princeton Algorithms 作者的 KruskalMST](https://github.com/kevin-wayne/algs4/blob/master/src/main/java/edu/princeton/cs/algs4/KruskalMST.java)
- [Princeton Algorithms 作者的 LazyPrimMST](https://github.com/kevin-wayne/algs4/blob/master/src/main/java/edu/princeton/cs/algs4/LazyPrimMST.java)

## 模拟面试问题

1. 基础理解：Kahn 残余节点为什么未必都在环上？
2. 实际应用：并发任务调度应在开始还是完成时释放后继依赖？
3. 实际应用：Prim 与 Dijkstra 的状态分别代表什么？
4. 深入追问：MST 为什么不等于最短路树或高可用网络？
5. 深入追问：多个相同权值时怎样验证一个候选 MST？
