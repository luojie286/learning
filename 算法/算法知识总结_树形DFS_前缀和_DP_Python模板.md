# 算法知识总结：树形 DFS、前缀和、计数 DP 与 Python 竞赛模板

> 来源：本次聊天记录。面向 LeetCode 刷题与蓝桥杯 Python 竞赛。重点记录**为什么这样设计**、**状态/递归返回值的含义**、**典型错误**与**可复用模板**。

## 0. 速查索引

| 题型 | 核心思想 | 时间复杂度 | 最容易错的地方 |
|---|---|---|---|
| 路径总和 III（LeetCode 437） | 前缀和 + 频次哈希 + DFS 回溯 | `O(n)` 平均 | 查 `curSum - targetSum`；左右子树不能共享历史路径状态 |
| 二叉树最近公共祖先（LeetCode 236） | 后序 DFS：递归返回命中的 `p/q/LCA` | `O(n)` | 当前节点是 `p/q` 时要立即返回 |
| 二叉树最大路径和（LeetCode 124） | 树形 DP：对父节点返回单链，更新答案可连接双链 | `O(n)` | 负贡献截断为 0；全局答案初始化 `-inf`；返回 `ans` |
| 平方数配对计数 | 固定总和 + 有界变量计数 + 平方和公式 | `O(1)`* | `x` 的上下界；分段阈值；整数平方根端点 |
| 二进制串全 1 子区间计数 | 末尾连续 1 长度 + 累计贡献的计数 DP | `O(NK²)` | 追加 1 新增 `j+1`；只保存累计值 `< K`；模数依题面 |
| Python 竞赛读写 | `solve()` + 标准输入输出 | — | LeetCode 的 `Solution` 与普通 OJ 的输入输出形式不同 |

\* `O(1)` 指固定次数的大整数算术操作；严格按位复杂度会受整数位数影响。

---

## 1. 二叉树递归的总方法论

写 DFS 之前，先明确三个问题：

1. **递归函数返回什么？** 是布尔值、某个目标节点、单边路径和，还是不返回、只维护全局状态？
2. **什么时候更新全局答案？** 是在访问当前节点时更新，还是处理完左右子树后更新？
3. **状态需不需要回溯？** 如果递归分支共用可变字典、数组或集合，离开某条分支时往往需要撤销修改。

**后序 DFS** 的典型顺序：先拿到左右子树结果，再判断当前节点。

```python

def dfs(node):
    if node is None:
        return base_value

    left = dfs(node.left)
    right = dfs(node.right)

    # 依据左右子树的信息处理当前节点
    return merge(left, right, node)
```

### 1.1 题目中两种不同的“路径”

- **向下路径**：沿父节点 → 子节点，不能由左子树经父节点再进入右子树；路径总和 III 属于此类。
- **无向树上简单路径（不重复节点）**：可以从左子树经过某个节点进入右子树，但不能分叉；二叉树最大路径和属于此类。

不要把两题的路径约束混淆。

---

## 2. 路径总和 III：双 DFS → 前缀和优化

**题意**：统计向下连续路径中，节点值之和等于 `targetSum` 的路径数量。路径不必从根开始，也不必在叶子结束。

### 2.1 暴力双 DFS：把每个节点当起点

- `start_from(node, cur)`：固定起点，从当前节点继续向下探索。
- `enumerate_starts(node)`：遍历每一个节点，让它成为路径起点。

```python
class Solution:
    def pathSum(self, root, targetSum):
        ans = 0

        def start_from(node, cur):
            nonlocal ans
            if node is None:
                return
            cur += node.val
            if cur == targetSum:
                ans += 1
            start_from(node.left, cur)
            start_from(node.right, cur)

        def enumerate_starts(node):
            if node is None:
                return
            start_from(node, 0)
            enumerate_starts(node.left)
            enumerate_starts(node.right)

        enumerate_starts(root)
        return ans
```

复杂度：最坏 `O(n²)` 时间、`O(h)` 递归空间，`h` 是树高。

### 2.2 优化：前缀和 + 哈希表

定义沿着**当前根到当前节点**路径的前缀和：

- 当前前缀和 `cur`
- 某个祖先之前的前缀和 `pre`
- 从这个祖先之后到当前节点的路径和：`cur - pre`

要求路径和等于 `targetSum`：

\[
cur-pre=targetSum \Longrightarrow \boxed{pre=cur-targetSum}
\]

**查找方向必须是 `cur - targetSum`，不是 `targetSum - cur`。**

用 `mp[x]` 记录**当前递归路径上**前缀和 `x` 出现的次数。之所以记录频次而不是是否出现，是因为多个不同起点可能产生同一个前缀和。

```python
from collections import defaultdict

class Solution:
    def pathSum(self, root, targetSum):
        freq = defaultdict(int)
        freq[0] = 1  # 空前缀：允许路径从根节点开始
        ans = 0

        def dfs(node, cur):
            nonlocal ans
            if node is None:
                return

            cur += node.val
            ans += freq[cur - targetSum]  # 查历史前缀和

            freq[cur] += 1                 # 当前前缀加入路径
            dfs(node.left, cur)
            dfs(node.right, cur)
            freq[cur] -= 1                 # 回溯：离开当前路径

        dfs(root, 0)
        return ans
```

**顺序是关键**：先查询，再把当前前缀和加入哈希表。否则当 `targetSum = 0` 时，当前节点对应的空长度区间可能被误计。

**为什么要回溯？** 左子树中的某个前缀和，不应该被右子树当作当前路径的祖先前缀和，否则会统计一条跨越两个分支、不符合“只能向下”的假路径。

**为什么初始化 `freq[0] = 1`？** 若根到某节点的和恰好为目标，路径开始前的空前缀和 0 应被算一次。

复杂度：平均 `O(n)` 时间；`O(h)` 额外空间（DFS 栈和当前路径前缀频次表；最坏 `O(n)`）。

> **代码质量注意**：不要写 `node.val = cur + node.val` 直接覆盖原节点。这样的写法在特定单次 DFS 中未必影响本次计数，但会破坏原树，影响重复调用或后续处理。用局部变量 `cur += node.val` 更稳妥。

---

## 3. 最近公共祖先 LCA：返回“本子树找到的目标”

**题意**：二叉树中已知两个节点 `p` 和 `q`，求它们的最近公共祖先（节点可以是自身的祖先）。

递归约定：`dfs(node)` 返回当前子树中找到的 `p`、`q` 或最终的 LCA；如果都没有，返回 `None`。

```python
class Solution:
    def lowestCommonAncestor(self, root, p, q):
        def dfs(node):
            if node is None or node is p or node is q:
                return node

            left = dfs(node.left)
            right = dfs(node.right)

            if left is not None and right is not None:
                return node
            return left if left is not None else right

        return dfs(root)
```

**为什么遇到 `p` 或 `q` 就立即返回？** 例如 `p` 本身是 `q` 的祖先，答案就是 `p`。在 `p` 处应立即返回 `p`；不需要再把下方的 `q` 向上传，覆盖掉它。

**三种合并情况**：

1. 左右子树都有返回值 → 当前节点是 LCA。
2. 只有一侧有返回值 → 原样向上返回。
3. 两侧都无返回值 → `None`。

时间 `O(n)`，递归栈 `O(h)`。这个简洁版本通常依赖题目保证 `p` 和 `q` 都在树中。

**Python 容易写错**：`cur1 ? cur1 : cur2` 是 C++ 三元表达式写法；Python 写作 `cur1 if cur1 else cur2`。`if node is node` 永远成立，应该比较 `node is p or node is q`。

---

## 4. 二叉树最大路径和：返回值与全局答案不是一回事

**题意**：路径可以从任意节点开始到任意节点结束，相邻节点相连且不重复。路径可以经过一个节点的左右两个孩子，但**不能分叉后继续向父节点延伸**。

### 核心设计

- `dfs(node)` **返回给父节点**：从 `node` 开始、只能往一侧延伸的最大路径和。
- `ans` **更新全局答案**：允许 `node` 连接左右两边，构成一条完整路径。
- 负收益分支不选：`left = max(0, dfs(node.left))`。

公式：

\[
\begin{aligned}
left &= \max(0,dfs(node.left)) \\
right &= \max(0,dfs(node.right)) \\
ans &= \max(ans,node.val+left+right) \\
dfs(node) &= node.val+\max(left,right)
\end{aligned}
\]

```python
class Solution:
    def maxPathSum(self, root):
        ans = float('-inf')

        def dfs(node):
            nonlocal ans
            if node is None:
                return 0

            left = max(0, dfs(node.left))
            right = max(0, dfs(node.right))

            ans = max(ans, node.val + left + right)
            return node.val + max(left, right)

        dfs(root)
        return ans
```

**高频易错点**：

- `ans = 0` 会让全负树出错，例如单节点 `-3` 的答案就是 `-3`。
- `return dfs(root)` 返回的是根节点的最大单边路径和，**不是**整棵树最大路径和；应返回 `ans`。
- 记得包含 `node.val`；不允许左右两边都返回给父节点。
- 既然左右负收益已经截断为 0，就不用额外判断 `if left > 0 and right > 0`。
- 更新写 `ans = max(ans, ...)`，不能误写 `max(0, ...)` 而丢失历史最大值。

时间 `O(n)`、额外递归栈空间 `O(h)`。

---

## 5. 数学计数：`x + y` 是完全平方数

**问题模型**：`0 ≤ x ≤ A`，`0 ≤ y ≤ B`，求多少有序整数对满足 `x + y = s²`；再对所有可能平方数求和。聊天中的常量：

```python
A = 20269876543210
B = 20260123456789
MOD = 998244353
```

### 5.1 固定和后把双变量变成一个变量

因为 `y = s² - x`，约束为：

\[
0\le x\le A,\quad 0\le s^2-x\le B
\]

移项：

\[
\max(0,s^2-B)\le x\le\min(A,s^2)
\]

因此方案数为：

\[
\boxed{cnt(s)=\min(A,s^2)-\max(0,s^2-B)+1}
\]

当 `0 ≤ s² ≤ A+B` 时才存在解；平方根上界为 `isqrt(A+B)`，遍历时要包含端点。

### 5.2 为什么简单循环可能超时

平方根上界约为几百万，Python 中每轮乘方、取 `min/max`、取模，仍可能卡严格时限。**数学规律比微优化循环更重要。**

### 5.3 分段 + 平方和，消掉循环

已知本题 `A ≥ B`，将 `t=s²` 分段：

| 范围 | 配对数 |
|---|---|
| `t ≤ B` | `t + 1` |
| `B < t ≤ A` | `B + 1` |
| `A < t ≤ A+B` | `A+B-t+1` |

使用平方和公式：

\[
S(m)=\sum_{i=0}^{m}i^2=\frac{m(m+1)(2m+1)}6
\]

```python
from math import isqrt

A = 20269876543210
B = 20260123456789
MOD = 998244353


def square_sum(m):
    return m * (m + 1) * (2 * m + 1) // 6


def solve():
    l = isqrt(B)
    r = isqrt(A)
    m = isqrt(A + B)

    # 0 <= s <= l
    ans = square_sum(l) + (l + 1)
    # l < s <= r
    ans += (r - l) * (B + 1)
    # r < s <= m
    ans += (m - r) * (A + B + 1) - (square_sum(m) - square_sum(r))
    print(ans % MOD)


if __name__ == '__main__':
    solve()
```

**技巧提炼**：先固定一个量 → 用边界求可行解数量 → 找分段 → 识别等差和/平方和等闭式。注意 `isqrt()` 是**整数下取整平方根**，不会受到浮点精度影响。

---

## 6. 二进制串计数 DP：连续全 1 子区间贡献

**题意**：长度为 `N` 的 01 串，每个全为 1 的连续子区间贡献 1，求总贡献至少 `K` 的串数。已知 `1 ≤ N ≤ 1000`、`0 ≤ K ≤ min(N(N+1)/2, 100)`。

### 6.1 从贡献增量入手

连续 `1` 段长度为 `L`，全 1 子区间个数为：

\[
f(L)=1+2+\cdots+L=\frac{L(L+1)}2
\]

如果当前末尾连续 `1` 长度为 `j`：

- 追加 `0`：新增贡献 `0`，连续段长度归零。
- 追加 `1`：**新增贡献 `j+1`**，连续段长度变成 `j+1`。

关键在于：不需要重新统计整个字符串，维护**末尾连续 1 的长度**就足以知道新增贡献。

### 6.2 正难则反：计数 `< K` 的方案

总 01 串数是 `2^N`，符合条件的答案是：

\[
\boxed{ans=2^N-bad}\pmod{MOD}
\]

其中 `bad` 为累计共鸣次数 `< K` 的方案数。`K ≤ 100`，因此只保存这部分 DP 状态，大幅减少状态空间。

### 6.3 DP 状态与转移

`dp[s][j]`：当前已经构造的前缀中，共鸣次数为 `s`，末尾连续 `1` 长度为 `j` 的方案数。

初始：`dp[0][0] = 1`。

转移：

- 末尾追加 `0`：`ndp[s][0] += dp[s][j]`
- 末尾追加 `1`：若 `s+j+1 < K`，则 `ndp[s+j+1][j+1] += dp[s][j]`

使用滚动数组，不用保存 `N` 这一维。

```python
import sys

input = sys.stdin.buffer.readline
MOD = 10**9 + 7  # 示例值；必须按原题指定的模数替换


def solve():
    N, K = map(int, input().split())
    total = pow(2, N, MOD)

    if K == 0:
        print(total)
        return

    dp = [[0] * K for _ in range(K)]
    dp[0][0] = 1

    for _ in range(N):
        ndp = [[0] * K for _ in range(K)]
        for s in range(K):
            # 追加 0：不同末尾长度合并
            ndp[s][0] = sum(dp[s]) % MOD

            # 追加 1：增量为 j+1
            for j in range(K):
                ways = dp[s][j]
                if ways == 0:
                    continue
                ns = s + j + 1
                if ns < K:
                    ndp[ns][j + 1] = (ndp[ns][j + 1] + ways) % MOD
        dp = ndp

    bad = sum(sum(row) for row in dp) % MOD
    print((total - bad) % MOD)


if __name__ == '__main__':
    solve()
```

复杂度：`O(NK²)` 时间、`O(K²)` 空间。当 `K = 0` 时不能初始化 `dp[0][0]`，需单独处理。

> **题面缺失提醒**：用户粘贴的原始题面中模数未显示，所以这里的 `10**9+7` 只是占位示例，并非已核实的该题模数。提交前必须确认原题。

### 6.4 如何判断 DP 状态设计是否合理

问自己：**在已处理前缀的基础上，决定下一个位置贡献，需要知道哪些历史信息？**

此题只需要：目前的累计贡献 `s` 与末尾连续 `1` 长度 `j`；不需要存储完整字符串。这就是状态压缩的根本依据。

---

## 7. Python 高频竞赛模板

### 7.1 通用入口与输入输出

```python
import sys
from math import isqrt, gcd
from collections import defaultdict, Counter, deque
from heapq import heappush, heappop
from bisect import bisect_left, bisect_right

input = sys.stdin.buffer.readline


def solve():
    n, k = map(int, input().split())
    a = list(map(int, input().split()))
    # 在这里编写算法
    print(0)


if __name__ == '__main__':
    solve()
```

`if __name__ == '__main__':` 表示**只有直接运行当前文件**时才执行 `solve()`；若被别的模块 `import`，则不会自动调用。竞赛不是强制要求，但建议保留。

常用输入：

```python
n = int(input())
a, b = map(int, input().split())
arr = list(map(int, input().split()))
s = input().decode().strip()      # 使用 buffer.readline 时获取 str
print(*arr)                        # 空格分隔输出

# 多组数据
t = int(input())
for _ in range(t):
    # 处理每一组测试
    pass
```

### 7.2 初始化数组与字典

```python
n, m, k = 5, 6, 7
arr = [0] * n
mat = [[0] * m for _ in range(n)]
dp = [[[0] * k for _ in range(m)] for _ in range(n)]

mp = {}
mp[3] = mp.get(3, 0) + 1

freq = defaultdict(int)
freq[3] += 1

cnt = Counter([1, 1, 2])           # {1: 2, 2: 1}
```

**千万别把二维数组写成** `[[0] * m] * n`：它复制的是**行的引用**，改一行会影响其他行。

**Python 与 C++ 的传参区别**：

- C++ `map<int,int> mp` 按值传递，会复制；`map<int,int>& mp` 引用传递，共享原对象。
- Python `dfs(mp)` 传入字典对象时，递归函数与调用者引用的是**同一个可变字典**；函数返回不会自动撤销修改。需要根据算法手动回溯，或者显式拷贝（通常代价较高）。

### 7.3 栈、队列、最小堆

```python
# 栈：LIFO
stack = []
stack.append(3)
x = stack.pop()

# 双端队列：适用于 BFS
q = deque([0])
q.append(1)
x = q.popleft()

# 最小堆
heap = []
heappush(heap, (5, 'A'))
heappush(heap, (2, 'B'))
priority, item = heappop(heap)     # (2, 'B')
```

### 7.4 二分查找 / 排序 / 数学

```python
arr = [1, 2, 2, 4, 8]
arr.sort()

l = bisect_left(arr, 2)             # 第一个 >= 2 的位置
r = bisect_right(arr, 2)            # 第一个 > 2 的位置
count_2 = r - l

root = isqrt(10**12 + 123)         # 精确整数平方根
```

### 7.5 BFS 模板（适用于无权图最短路）

```python
from collections import deque


def bfs(start, graph):
    dist = {start: 0}
    q = deque([start])
    while q:
        u = q.popleft()
        for v in graph[u]:
            if v not in dist:
                dist[v] = dist[u] + 1
                q.append(v)
    return dist
```

### 7.6 DFS + 回溯模板

```python

def dfs(u, path, visited):
    visited.add(u)
    path.append(u)

    # 使用当前路径做判断或计数
    for v in graph[u]:
        if v not in visited:
            dfs(v, path, visited)

    path.pop()          # 撤销当前路径修改
    visited.remove(u)   # 撤销访问标记（用于枚举简单路径）
```

注意：这一版本适合**枚举路径**；如果只是遍历一个图的所有可达点，通常不应回溯删除 `visited`，否则可能重复访问节点。

### 7.7 一维前缀和模板

```python
arr = [2, -1, 3, 5]
pre = [0]
for x in arr:
    pre.append(pre[-1] + x)

# [l, r] 的和（含两端）
l, r = 1, 3
segment_sum = pre[r + 1] - pre[l]
```

与二叉树前缀和题的思想相通：**区间和 = 终点前缀和 - 起点前一位置的前缀和**。

### 7.8 递归深度注意事项

```python
import sys
sys.setrecursionlimit(10**6)
```

它只调整 Python 递归层数限制，并不保证操作系统或解释器能承受极深递归。树退化成链或深图时，**显式栈**通常更稳妥。

---

## 8. 刷题与比赛中的检查清单

1. **明确统计对象**：是路径数量、节点数量、方案数量，还是最大和？不要混淆。
2. **写清楚递归函数含义**：返回单边值还是完整答案？`dfs` 返回值是否必须计入 `ans`？
3. **把公式推导出来再敲代码**：例如 `cur - pre = target`，查的是 `pre = cur - target`，绝不可直接凭印象反过来。
4. **手工测试极端样例**：空树、单节点、全负节点、`K=0`、`targetSum=0`、左右子树同时存在、链状树。
5. **区分共享状态与局部状态**：字典/集合/列表被原地修改后不会因函数退出而恢复；分支之间有隔离要求就回溯。
6. **不要不必要地改输入**：原地覆盖 `node.val` 可能不影响当前计算，却会影响复用与调试。
7. **重视数量级**：`sqrt(4×10^13)` 级别可能意味着几百万次循环；若限时紧，应考虑公式化简。
8. **整数边界一致**：`range(m+1)` 是否包含 `isqrt(n)`；分段区间是否重复或漏计端点。
9. **确认模数**：题面不完整时不可把示例 `MOD` 当作原题要求。
10. **比赛与 LeetCode 的接口不同**：LeetCode 多数只实现 `Solution` 方法，普通 OJ 需要自行读取标准输入并打印输出。

## 9. 最值得反复记忆的 5 个核心模式

```text
1. 枚举起点 + 向下 DFS    → 先获得 O(n²) 正确解，再尝试优化
2. 当前前缀 - 历史前缀 = 目标和
   → 哈希查当前前缀 - 目标；递归分支间要回溯
3. 递归返回“父节点能继续使用的信息”
   → LCA 返回目标节点；最大路径和返回最大单边链
4. 计数 DP = 状态足够描述下一步的增量
   → 二进制串题只需累计贡献 + 末尾连续 1 长度
5. 先压缩变量，再用公式消循环
   → 固定总和后的区间方案计数 + 平方和公式
```

**建议复习顺序**：路径总和 III → 最近公共祖先 → 最大路径和 → 二进制串计数 DP → 平方和计数。先能独立口述“状态含义/转移公式/回溯原因”，再在无提示下写出代码。
