---
id: algo-trie-prefixes
title: Trie：前缀、词频与内存模型
description: 将前缀计数、完整单词计数与字符集边界分开设计。
course: algorithms
order: 8
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# Trie：前缀、词频与内存模型

## 一条路径表示一个前缀

Trie 将字符串公共前缀合并成路径：边是字符，根到节点的字符序列是前缀。插入 ab 与 ac 时，只有一条根到 a 的边，a 下再分 b/c。原稿示意图让 ac 从根的 c 边开始，与文字不符，在此更正。

节点通常需要 child、pass 和 end：pass 为经过这个前缀的词次数，end 为恰好结束于此的词次数。插入 app 两次与 apple 一次，前缀 app 的 pass=3，完整 app 的 end=2。这些是次数，不是去重词数，想统计不同词要另行定义。

```python
class Trie:
    def __init__(self):
        self.root = {'children': {}, 'pass': 0, 'end': 0}
    def insert(self, word):
        node = self.root; node['pass'] += 1
        for ch in word:
            node = node['children'].setdefault(ch,
                {'children': {}, 'pass': 0, 'end': 0})
            node['pass'] += 1
        node['end'] += 1
    def prefix_count(self, prefix):
        node = self.root
        for ch in prefix:
            node = node['children'].get(ch)
            if node is None: return 0
        return node['pass']
```

示例定义允许空词；根 pass 因而表示所有插入次数，查询空前缀返回总次数。业务不允许空词时应入口拒绝，不要靠数据结构偶然行为决定。

## 时间与空间不能只看字符串数

固定数组孩子访问 O(1)，长度 L 的插入/查询 O(L)；用哈希孩子是平均 O(L)。节点数不超过总字符数加一，稠密数组空间 O(节点数×字符集大小)，动态字典只为存在边分配，但每个字典有对象开销。原稿 `3000005×65` 的 int 表，仅边表就约 780 MB（十进制），不是轻量结构。

原 C++ 清零循环第二维 65，却写到 122，构成越界；ASCII 编码值与映射后的孩子下标是两回事。映射 A-Z、a-z、0-9 时只允许 62 类输入，其他字符必须拒绝或另加映射，不能统一按数字分支处理。

## 实际开发：搜索建议

站内输入“并”希望列出“并发”“并查集”。先走到前缀节点，再遍历后代收集 end>0 的词。查到前缀本身 O(L)，返回所有建议还需访问结果子树；不能把“输出任意多条建议”也写成 O(L)。可保存每个节点的 Top K 热门建议，代价是更新与额外空间。

中文采用 Unicode 字符或 UTF-8 字节建边，语义、节点数不同。大小写、全半角与规范化应统一在索引和查询入口；不能仅对查询转小写。需要拼音、模糊匹配或相关性排序时，Trie 不是完整搜索引擎。

## 适度拓展

删除一个词要先验证 end>0，再沿路径递减 pass 和 end；否则会产生负频次。多组竞赛数据可只清理已使用节点，但应包含根并重置 idx。静态词典可考虑压缩前缀树；任意子串查找则是另一类任务，不能直接用前缀 Trie 替代。

## 参考资料

- [Princeton Algorithms 作者的 TrieST](https://github.com/kevin-wayne/algs4/blob/master/src/main/java/edu/princeton/cs/algs4/TrieST.java)
- [Unicode 标准规范化 UAX #15](https://unicode.org/reports/tr15/)

## 模拟面试问题

1. 基础理解：pass 与 end 统计什么，重复插入如何影响它们？
2. 实际应用：前缀匹配和返回全部建议的复杂度为什么不同？
3. 实际应用：如何发现原稿字符表清零循环的越界问题？
4. 深入追问：中文 Trie 按字符与按字节建边有何取舍？
5. 深入追问：怎样安全删除一次词频而不破坏共享前缀？
