---
id: java-collections
title: "集合选择与相等性"
description: "围绕订单列表、去重和索引理解集合契约与复杂度。"
course: java-basics
order: 7
date: 2026-10-23
tags: [Java, 后端]
---

# 集合选择与相等性

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 根据操作选择结构

List 表达有次序的元素序列且可重复；Set 表达不重复；Map 存键值映射，并不继承 Collection。Queue 并非都先进先出，PriorityQueue 按优先级出队，其迭代顺序也不保证全局排序；双端队列 ArrayDeque 可承担普通队列或栈。

ArrayList 随机访问 O(1)，尾部追加均摊 O(1)，中间插删一般 O(n)。LinkedList 找到指定下标要 O(n)，只有已定位节点附近操作才可能 O(1)。不能仅因“频繁插入”就直接推荐链表，还要看定位成本与内存局部性。

集合底层不止数组、链表和红黑树：还有哈希表、堆、位向量等。API 契约与 JDK 具体扩容倍率分开学习，扩容细节不是 Java 语言承诺。

## 实际开发：批量查商品

```java
Set<Long> ids = new LinkedHashSet<>(requestedIds);
Map<Long, Product> byId = new HashMap<>();
for (Product p : products) byId.put(p.id(), p);
List<Product> ordered = new ArrayList<>();
for (Long id : ids) {
    Product p = byId.get(id);
    if (p != null) ordered.add(p);
}
```

用 LinkedHashSet 保留首次请求顺序并去重，Map 提供查找。HashMap 通常查询近似 O(1)，但依赖哈希分布；碰撞、比较和扩容有成本。集合规模小时，可读性通常比预先微调容量更重要。

## equals 与 hashCode

哈希集合要求 equals 相等的对象具有相同 hashCode，反过来不成立。作为 key 的业务字段若加入后变化，可能导致“看起来存在却找不到”。使用不可变 key 或专门 ID，避免可变 Entity 直接充当键。

TreeMap / TreeSet 以比较结果判断排序位置和重复性；比较器与 equals 不一致时要了解其后果。比较数值别用减法造成溢出。

## 修改与只读

遍历删除可用 Iterator.remove、removeIf 或正确设计的下标循环，并非只能用迭代器。fail-fast 是尽力检测错误，不能当同步机制。

Arrays.asList 是固定长度、背靠原数组的列表，能 set 但不能增删；List.of 不可修改且拒绝 null；不可修改容器不等于内部元素深度不可变。跨线程共享时还需安全发布与并发设计，普通 HashMap 不是共享写入方案。

## 参考资料

- [Collections Framework](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/package-summary.html)
- [HashMap API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/HashMap.html)
- [PriorityQueue API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/util/PriorityQueue.html)

## 模拟面试问题

1. 【基础】Map 为什么不属于 Collection？
2. 【应用】批量去重并保持请求顺序选择什么集合？
3. 【应用】为何修改 HashMap key 的字段可能导致查找失败？
4. 【深入】PriorityQueue 的遍历结果为什么不一定有序？
5. 【深入】固定长度、不可修改与深度不可变有什么区别？

