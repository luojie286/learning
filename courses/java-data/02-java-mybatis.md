---
id: java-mybatis
title: "MyBatis 映射与动态 SQL"
description: "理解 Mapper 代理、绑定参数和结果映射，保留 SQL 控制力。"
course: java-data
order: 2
date: 2026-10-23
tags: [Java, 后端]
---

# MyBatis 映射与动态 SQL

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## Mapper 代理不等于接口自动理解业务

MyBatis 封装 JDBC 样板、参数处理和结果映射，SQL 仍需由开发者设计。Mapper 接口经 MyBatis-Spring 注册代理 Bean，调用方法按 statement 映射执行；只有接口方法名不会让任意复杂查询自动获得实现。

```java
@Mapper
interface OrderMapper {
    @Select("""
        SELECT id, user_id AS userId, amount
        FROM orders
        WHERE id = #{id} AND user_id = #{userId}
        """)
    OrderRow findOwned(@Param("id") long id, @Param("userId") long userId);
}
```

OrderRow 是应用映射类型，这里明确两个参数名。按当前用户查询能避免只传订单 ID 导致水平越权，但 userId 必须来自可信认证上下文，不能直接信任请求体。

## 参数与结果

#{} 通常转为 JDBC 占位符并绑定值；${} 直接替换文本，不要接收客户端 SQL 片段。SQL 参数化不代替业务授权。字段名不同可用别名、resultMap 或按配置启用下划线转驼峰；不能假定任何项目都默认开启。

XML 的 namespace 对应 Mapper 全限定名，statement id 对应方法映射。resultType 表示一行的目标类型，不是把 List 当作单行类型。复杂构造器、关联与嵌套对象应显式检查映射与查询次数。

## 动态 SQL 的业务边界

列表筛选可用 if、where、foreach，既避免字符串散落，也要处理空集合、缺条件与更新范围。动态更新不小心省掉 WHERE 会影响整表；set 与 where 标签解决格式，不证明业务正确。

不要把“复杂业务都不能在 SQL”绝对化。数据库擅长集合运算、条件更新和约束；Service 负责组织业务操作与事务边界，SQL 可以准确表达单次数据变更规则。

## 实际开发：列表 N+1

先查十条订单，再每条查用户和明细，可能形成几十次请求。按场景选择 JOIN 或批量 IN 查询，并注意连接导致的行重复、分页与聚合语义。一级缓存通常关联 SqlSession，二级缓存需明确配置和一致性边界，不能以“框架有缓存”为由忽略更新可见性。

MyBatis 版本与 Spring Boot Starter 需保持兼容；Boot 3 不直接照搬旧 javax 生态依赖。事务由集成层协调，避免同时手动提交 Spring 管理的 SqlSession。

## 参考资料

- [MyBatis Mapper XML](https://mybatis.org/mybatis-3/sqlmap-xml.html)
- [MyBatis 动态 SQL](https://mybatis.org/mybatis-3/dynamic-sql.html)
- [MyBatis Spring Boot Starter](https://mybatis.org/spring-boot-starter/mybatis-spring-boot-autoconfigure/)

## 模拟面试问题

1. 【基础】Mapper 接口没有实现类为什么能被调用？
2. 【应用】#{} 与 ${} 在输入安全上有什么区别？
3. 【应用】数据库 user_id 如何映射 Java userId？
4. 【深入】列表查询为什么会出现 N+1？
5. 【深入】为什么 Service 与 SQL 的职责不能简单按“复杂不复杂”划分？

