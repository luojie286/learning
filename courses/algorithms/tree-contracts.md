---
id: algo-tree-contracts
title: 树形 DFS：返回契约、路径与回溯
description: 区分向上传递的单支贡献、全局答案和当前路径状态。
course: algorithms
order: 5
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# 树形 DFS：返回契约、路径与回溯

## 先定义返回值，再选择遍历顺序

DFS 不是某一套代码，而是函数契约。高度依赖左右子树结果，用后序；BST 大小约束由祖先传下来，用参数；路径频次随当前调用栈变化，用回溯。每一层 return 只结束当前调用，需要把答案显式向上传递。

二叉树直径（按边数）可令空树高度为 0，当前高度为 `1+max(left,right)`，经过当前点的直径为 left+right。最大路径和也类似，但节点值可为负，返回给父亲只能带一条分支，全局候选可以接左右两条。

```python
def max_path_sum(root):
    if root is None:
        raise ValueError('路径要求至少一个节点')
    answer = float('-inf')
    def dfs(node):
        nonlocal answer
        if node is None: return 0
        left = max(0, dfs(node.left))
        right = max(0, dfs(node.right))
        answer = max(answer, node.val + left + right)
        return node.val + max(left, right)
    dfs(root)
    return answer
```

树 `-10(9,20(15,7))` 中答案为 15+20+7=42，但 20 子树向父亲只能返回 35。全负树必须保留至少一个负节点，answer 初始为负无穷。每点处理一次 O(n)，栈 O(h)。

## BST 是祖先约束，不是相邻比较

验证严格 BST 时传开区间 `(low,high)`，左子树上界缩成当前值，右子树下界变当前值。根 10 的右孩子 15，其左孩子 6 虽小于 15，却违背根的约束。中序严格递增是等价验证方式；允许重复键的树需要先定义重复放置规则。

迭代中序用栈保存尚待返回的祖先，pop 时才访问节点。查第 k 小可提前终止，O(h+k) 时间、O(h) 栈。平衡树才能保证 h=O(log n)。有序数组建平衡 BST，区间递归必须排除中点，且将返回的子树接到左右指针，构建时间 O(n)。

## 当前路径上的前缀和

向下路径和等于 target，有 `cur-prefix=target`，查的是 `cur-target`。频次表只包含当前根到节点路径上的历史前缀，先查再加入当前值，退出子树时撤销；空前缀初始化为 1。

```python
freq = {0: 1}
def count_paths(node, cur, target):
    if node is None: return 0
    cur += node.val
    count = freq.get(cur - target, 0)
    freq[cur] = freq.get(cur, 0) + 1
    count += count_paths(node.left, cur, target)
    count += count_paths(node.right, cur, target)
    freq[cur] -= 1
    if freq[cur] == 0:
        del freq[cur]
    return count
```

调用前为每棵新树重新创建 freq。删除零频键且用 get 查询，表大小才随当前路径控制在 O(h)；原稿 defaultdict 查询和只减不删可能累积 O(n) 个零值键。算法平均 O(n) 时间；哈希复杂度是平均假设。固定宽度语言前缀和用足够宽的整数，不能覆盖 node.val。

## 最近公共祖先与结构修改

LCA 简洁递归：命中 p/q 返回该节点，左右同时命中返回当前节点，否则返回非空一侧。它依赖两个目标确实存在；一般接口须额外验证存在性。比较节点身份，不能只比较可能重复的值。

展开树为先序链时，先保存原右子树入口，明确“已处理节点 left 为空，right 链符合先序”的不变量。开发中的组织树、目录树也可能很深；业务数据常是图或含环，不能直接把二叉树递归模板套到任意关联表。退化树用显式栈，避免只增大 Python 递归上限。

## 翻转与镜像不是同一个问题

翻转树可先递归处理两侧再交换 left/right；返回原节点作为新子树根，空节点才返回 None，不能把叶子直接丢弃。对称检查则成对传节点 a/b：值相等，且 a.left 与 b.right、a.right 与 b.left 分别镜像；两侧同时空为真，仅一侧空为假。根值相同或左右各自有序都不足以推出镜像。

## 参考资料

- [Python 语言参考：nonlocal](https://docs.python.org/3/reference/simple_stmts.html#the-nonlocal-statement)
- [Python sys：递归限制与风险](https://docs.python.org/3/library/sys.html#sys.setrecursionlimit)

## 模拟面试问题

1. 基础理解：最大路径和为什么返回单支、更新答案却可连接双支？
2. 实际应用：只比较 BST 的直接孩子会漏掉什么错误？
3. 实际应用：树路径前缀和为什么必须回溯并先查询后插入？
4. 深入追问：只减频次不删除零键时，空间复杂度如何变化？
5. 深入追问：目标节点不保证存在时怎样修改 LCA 接口？
