---
id: algo-disjoint-sets
title: 并查集与动态连通性
description: 根代表集合，只有真正合并两个集合时才能减少连通块数。
course: algorithms
order: 7
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# 并查集与动态连通性

## 操作语义先于实现

并查集维护集合划分：find 返回代表元，union 合并两个集合。树边只是内部父链接，不是原图边，因此不能用 parent 路径还原原图实际道路。初始 parent[x]=x，每个有效节点自成一组。

```python
class DSU:
    def __init__(self, n):
        self.parent = list(range(n)); self.size = [1] * n
    def find(self, x):
        while x != self.parent[x]:
            self.parent[x] = self.parent[self.parent[x]]
            x = self.parent[x]
        return x
    def union(self, a, b):
        ra, rb = self.find(a), self.find(b)
        if ra == rb: return False
        if self.size[ra] < self.size[rb]: ra, rb = rb, ra
        self.parent[rb] = ra
        self.size[ra] += self.size[rb]
        return True
```

路径折半压缩查找路径，按大小合并防止小树挂出深链。二者组合的操作序列具有 O(α(n)) 均摊界，不是每次绝对 O(1)；单次查找仍可能走多条边。没有按秩或大小合并时，不应直接套用同一复杂度结论。

## 推演：增量岛屿

2×3 网格依次加入 `(0,0),(0,1),(1,2),(1,1)`。每新增一个陆地先使 components 加一，检查上下左右已存在陆地，每次 union 成功再减一。数量变化为 1、1、2、1。最后一格同时连接两组，必须合并两次；若邻居本来同组则不减少。

二维坐标映射 `id=r*cols+c`。水格即使预分配了 parent 也不能当陆地计数。重复加入同一格应直接返回旧数量。静态岛屿一次性统计 DFS/BFS 通常更直观，增量连接时并查集更方便。

## 开发例子与不能回答的问题

账号合并可把共享已验证邮箱的用户节点 union，最后按代表元分组；共享标识必须是业务认可的等价关系，不能仅凭相似昵称误合并。分组结果要保留原 ID，代表元可能随合并变化，不适合作为永久业务主键。

普通并查集擅长加边，不支持任意删边后自动拆分。好友关系也未必具有传递性，不能因为 A 认识 B、B 认识 C 就推断 A 与 C 是好友。它只能回答所定义无向关系下的连通性，不能直接判断有向依赖环。

## 适度拓展与边界

Kruskal 中发现两端已同根，加入该无向边就会成环。带权并查集和回滚并查集解决不同扩展需求；回滚通常避开破坏历史的路径压缩。节点越界应在外层校验；递归 find 在深链首次压缩前可能爆栈，迭代可消除此类调用栈风险。

## 参考资料

- [Princeton Algorithms 作者的 UF：按秩合并与路径压缩](https://github.com/kevin-wayne/algs4/blob/master/src/main/java/edu/princeton/cs/algs4/UF.java)
- [加权合并的实现对照](https://github.com/kevin-wayne/algs4/blob/master/src/main/java/edu/princeton/cs/algs4/WeightedQuickUnionUF.java)

## 模拟面试问题

1. 基础理解：union 为什么连接根而不是直接令 parent[a]=b？
2. 实际应用：动态岛屿为什么只在合并成功时减少计数？
3. 实际应用：并查集代表元为什么不宜当永久业务主键？
4. 深入追问：路径压缩的均摊界和单次最坏界有何区别？
5. 深入追问：普通并查集为什么无法直接处理删边？
