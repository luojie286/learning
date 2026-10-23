---
id: java-transactions
title: "SQL、索引与事务中的业务不变量"
description: "用防超卖与转账理解约束、隔离和 Spring 事务边界。"
course: java-data
order: 4
date: 2026-10-23
tags: [Java, 后端]
---

# SQL、索引与事务中的业务不变量

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 数据建模先表达规则

用户、商品、订单、明细分别建模，主键标识身份，唯一约束保护不可重复规则，NOT NULL 表达必填。SQL NULL 表示缺失或未知，比较用 IS NULL；WHERE x=NULL 不会得到正常等号预期。LEFT JOIN 右表条件放 WHERE 可能把未匹配行过滤掉。

金额用 DECIMAL 与 BigDecimal 对齐，时间字段约定时区；AUTO_INCREMENT 不保证连续，不能用编号缺口判断订单丢失。

## 实际开发：最后一件库存

```sql
UPDATE product
SET stock = stock - 1
WHERE id = ? AND stock > 0;
```

检查受影响行数是否为 1，再在同一事务中插入订单。先查 stock>0 再无条件更新会竞态；Java 单机锁也拦不住其他实例。

```java
@Transactional
public long createOrder(long productId, long userId) {
    if (stockMapper.reserve(productId) != 1) {
        throw new IllegalStateException("库存不足");
    }
    return orderRepository.create(productId, userId);
}
```

这是业务方法片段，Mapper/Repository 需实现并共享事务管理的数据源；真实错误应使用清晰业务异常。事务本身不自动防重复支付，仍需唯一约束、幂等键和状态转换条件。

## ACID 与隔离

原子性保护事务整体提交或回滚；一致性还依赖应用正确表达规则，不能期待数据库替你发现转账 SQL 写反。隔离级别、锁与 MVCC 决定并发观察；持久性还受日志刷盘与存储配置约束。

InnoDB 的 undo 支持回滚与多版本，redo 支持崩溃恢复，binlog 是服务器层变更日志，三者不能混为“操作历史”。快照读并不意味着永远读到最新提交；锁定读与普通 SELECT 的语义需区分。

## Spring 事务生效条件

代理模式需从适用代理入口进入；内部 this 自调用可能绕过。默认通常 RuntimeException/Error 回滚，受检异常按 rollbackFor 等配置；吞掉异常可能让事务提交。readOnly 多为提示，不是通用写保护。

REQUIRED 加入现有事务，REQUIRES_NEW 使用独立事务并可能额外占连接，要谨慎评估池容量。本地数据库事务不能自动撤销已发出的短信、支付请求或远程服务操作；跨边界需补偿、消息或幂等设计。

## 索引与证据

为按用户倒序查询订单考虑联合索引 (user_id, created_at, id)，结合 EXPLAIN、行数和真实负载验证。索引占空间并增加写成本，不能为每列都建一个索引。复杂查询是否使用索引、是否回表要以具体执行计划为证据。

## 参考资料

- [MySQL 8.4 InnoDB 事务模型](https://dev.mysql.com/doc/refman/8.4/en/innodb-transaction-model.html)
- [MySQL 8.4 索引](https://dev.mysql.com/doc/refman/8.4/en/optimization-indexes.html)
- [Spring 6.2 @Transactional](https://docs.spring.io/spring-framework/reference/6.2/data-access/transaction/declarative/annotations.html)
- [Spring 6.2 回滚规则](https://docs.spring.io/spring-framework/reference/6.2/data-access/transaction/declarative/rolling-back.html)

## 模拟面试问题

1. 【基础】事务的 ACID 各是什么？
2. 【应用】最后一件商品如何避免两个请求同时扣减成功？
3. 【应用】捕获异常后不再抛出，可能怎样影响事务？
4. 【深入】本地事务为什么不能自动回滚第三方支付？
5. 【深入】如何为用户订单列表设计并验证联合索引？

