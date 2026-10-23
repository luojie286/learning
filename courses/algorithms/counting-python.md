---
id: algo-counting-python
title: 数学计数与 Python 模板的成本
description: 固定总和压缩变量，用整数边界与小规模穷举验证闭式。
course: algorithms
order: 13
date: 2026-10-23
tags: [算法, 计算机基础, 面试]
---

# 数学计数与 Python 模板的成本

## 从两个变量到一个区间

求 0<=x<=A、0<=y<=B 且 x+y 是完全平方数的有序整数对。固定 t=s² 后 y=t-x，x 范围为 `[max(0,t-B),min(A,t)]`。若下界不超过上界，数量为上界减下界加一；s 只需 0 到 isqrt(A+B)，不能对正负 s 重复计数同一平方。

例如 A=2、B=1：平方和只能 0、1，分别产生 (0,0) 与 (0,1)/(1,0)，共 3 对。这里是有序对，交换两个坐标未必是同一个结果，且 A/B 范围可能不同。

## 分段消掉几百万次循环

可先交换 A/B 使 A>=B，计数总量不变。固定和 t 的方案数依次为：t<=B 时 t+1；B<t<=A 时 B+1；A<t<=A+B 时 A+B-t+1。定义 S(m)=m(m+1)(2m+1)/6，累加平方可用闭式。

```python
from math import isqrt

def count_square_pairs(a, b):
    if a < 0 or b < 0: raise ValueError('上界需非负')
    if a < b: a, b = b, a
    low, high, end = isqrt(b), isqrt(a), isqrt(a+b)
    def sqsum(m): return m*(m+1)*(2*m+1)//6
    return (sqsum(low) + low + 1
            + (high-low)*(b+1)
            + (end-high)*(a+b+1)
            - (sqsum(end)-sqsum(high)))
```

low/high/end 是三个分段的整数平方根端点；全都为 0 时也正确。Python isqrt 是精确整数根，浮点 sqrt 在大整数近平方边界可能舍入出错。闭式用固定次数算术操作，但严格位复杂度仍取决于整数位数，不能在任意大输入下称机器时间绝对常数。

## Python 模板不能掩盖语义

普通 OJ 自行读取输入输出，LeetCode 通常只实现指定方法。`sys.stdin.buffer.readline` 返回 bytes，decode 后才是 str；strip 会删除两端空白，若空格是有效字符就不可随意使用。

```python
import sys
from collections import deque, defaultdict, Counter
from heapq import heappush, heappop
from bisect import bisect_left, bisect_right

def solve():
    line = sys.stdin.buffer.readline()
    if not line: return
    a, b = map(int, line.split())
    print(count_square_pairs(a, b))

if __name__ == '__main__':
    solve()
```

字典、集合查找通常按平均 O(1) 分析；排序不保证 O(1) 辅助空间；list.pop(0) 线性，deque.popleft 常数。类方法递归用 self.method，嵌套函数重绑定外层变量用 nonlocal；修改列表内容不等于重绑定。统一 val/value 属性名，Python 动态加属性可能让拼写错误不立即报错。

## 把公式变成可信结果

先枚举 A/B 在 0..20 的所有小实例，暴力检查每对 x/y 的和是否满足 isqrt(t)^2==t，对照闭式。再测试 A=B、A=0、平方数恰落在 B/A/A+B 端点的情况。对模数题先做精确整数除法再取模，或者在适用前提下用模逆；不能直接把整数除 6 变成模意义普通除法。

这种“压缩变量—推导边界—分段—核对”的流程也适合优惠金额组合和库存分配计数，但现实业务可能有独立权重或额外条件，公式必须随模型重推。

## 参考资料

- [Python math.isqrt](https://docs.python.org/3/library/math.html#math.isqrt)
- [Python 标准类型与整数](https://docs.python.org/3/library/stdtypes.html)
- [Python __main__ 入口](https://docs.python.org/3/library/__main__.html)

## 模拟面试问题

1. 基础理解：固定总和为什么能把二维枚举变成区间计数？
2. 实际应用：平方根分段端点如何避免漏算或重复？
3. 实际应用：为什么浮点平方根不能可靠替代 isqrt？
4. 深入追问：固定次数大整数算术为何不代表严格常数位复杂度？
5. 深入追问：模运算下的除法为什么需要额外条件？
