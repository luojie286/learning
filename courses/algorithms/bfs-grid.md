---
id: algo-bfs-grid
title: 网格搜索与多源 BFS
description: 把格子建模为图，区分全局访问状态和路径枚举状态。
course: algorithms
order: 6
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# 网格搜索与多源 BFS

## 什么时候 BFS 能给出最短距离

无权图或所有边代价相同，先进先出队列使节点按距离层处理。首次发现时记录距离并入队，可保证每个节点至多入队一次。带权边长度不同，普通 BFS 不再保证最短；0/1 权可学双端队列，非负一般权转 Dijkstra。

```python
from collections import deque

def distances(starts, graph):
    dist = {}
    q = deque()
    for s in starts:
        if s not in dist:
            dist[s] = 0; q.append(s)
    while q:
        u = q.popleft()
        for v in graph[u]:
            if v not in dist:
                dist[v] = dist[u] + 1
                q.append(v)
    return dist
```

多个起点同时赋距离 0，就是多源 BFS，不是依次让每个源跑完全图。图有 V 点 E 边，邻接表实现 O(V+E) 时间、O(V) 状态；双向无向边计入两次仍同阶。

## 走一遍：多处火源的传播

走廊格子 `S . . . S` 两端起火：距离依次为 0、1、2、1、0。第一层处理两个 S，产生两个距离 1 的格；第二层才产生中间距离 2 的格。逐个源扩散并覆盖时间，会把“最近火源到达”算错。

腐烂橘子题同理，初始腐烂格为第 0 分钟；每次只让新鲜格入队，并立刻改状态。初始无新鲜格返回 0；队列耗尽仍有新鲜格说明不可达。分层循环开始先固定本层队列长度，循环中新入队的是下一层。

## 连通块不等于所有路径

数岛屿时扫描每个未访问陆地，答案加一，再 DFS/BFS 消耗整个连通块。四联通与八联通由题目决定；字符串 '1' 与数字 1 不能混用。空网格先返回，访问下标前判断边界；不能接受 Python 负下标意外绕到最后一行。

遍历可达节点的 visited 是全局、单调增加的，不应返回时撤销。枚举所有简单路径的 visited 表示“当前路径已出现”，必须回溯；混用会导致重复搜索或漏掉路径。

## 现实开发中的对应

“找最近服务站”可在无权道路抽象上使用多源 BFS；真实道路耗时不同，需按边权改算法。“列出某服务依赖的全部服务”是可达性搜索，要防依赖环；“列出所有依赖路径”可能指数爆炸，应设置规模限制并明确产品需求。

树层序遍历的右视图，正常先左后右时取每层最后一个；DFS 则可先右后左，首次到某深度记录。两种方法都正确，但不能混搭“最后一个”和“右边优先”的解释。

## 复杂度与实现边界

R×C 网格每格最多访问一次，O(RC) 时间，队列或栈最坏 O(RC)。DFS 不总比 BFS 省空间，蛇形长路径递归同样达到 RC。队列用 deque.popleft；list.pop(0) 会搬移元素。修改输入作为访问标记能省一个数组，却改变了接口语义，复用输入时需要显式 visited。

## 参考资料

- [Python collections.deque](https://docs.python.org/3/library/collections.html#collections.deque)
- [Princeton Algorithms 作者的 BreadthFirstPaths](https://github.com/kevin-wayne/algs4/blob/master/src/main/java/edu/princeton/cs/algs4/BreadthFirstPaths.java)

## 模拟面试问题

1. 基础理解：BFS 为什么在等权图中首次发现就能确定最短距离？
2. 实际应用：多源扩散为什么把所有起点放到第 0 层？
3. 实际应用：数连通块时为什么不回溯删除 visited？
4. 深入追问：真实道路导航为什么不能简单使用普通 BFS？
5. 深入追问：DFS 与 BFS 的最坏空间分别由什么决定？
