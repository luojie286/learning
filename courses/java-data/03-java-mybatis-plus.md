---
id: java-mybatis-plus
title: "MyBatis-Plus 条件构造器与分页"
description: "把方法引用、动态条件与分页的真实前提联系起来。"
course: java-data
order: 3
date: 2026-10-23
tags: [Java, 后端]
---

# MyBatis-Plus 条件构造器与分页

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 在 MyBatis 上减少 CRUD 样板

BaseMapper<T> 提供约定的通用 CRUD，@Mapper 或 @MapperScan 解决注册。@TableName、@TableId 等描述映射与主键策略，不会自动把任意现有数据库结构改好。

AUTO 通常依靠数据库自增，INPUT 由应用提供，ASSIGN_ID 由框架生成；选择取决于现有表与跨服务 ID 需求。实体到表的命名受全局配置影响，不能声称 UserInfo 映射 user_info 一定要显式 @TableName。

## 实际开发：菜品条件查询

```java
LambdaQueryWrapper<Dish> query = new LambdaQueryWrapper<Dish>()
    .eq(Dish::getEnabled, true)
    .like(keyword != null && !keyword.isBlank(), Dish::getName, keyword)
    .and(w -> w.eq(Dish::getCategory, "MAIN")
               .or().eq(Dish::getCategory, "SIDE"))
    .orderByDesc(Dish::getCreatedAt)
    .orderByDesc(Dish::getId);
```

Dish 是需提供对应 getter 的应用实体。方法引用描述属性，框架据元数据解析列名，不是当场从某个 Dish 读取值。like 通常形成两侧带 % 的绑定值；通配符本身会影响匹配语义，若业务要字面匹配还须处理转义。

and 嵌套明确括号，防止 enabled=true AND A OR B 让 B 绕过有效状态限制。条件参数 false 表示不添加，不等于自动替你决定 null 的业务含义。

## 分页有基础设施前提

selectPage 需要对应分页插件与兼容依赖，常用 MybatisPlusInterceptor 加 PaginationInnerInterceptor。3.5.9 起相关解析器依赖有拆分，按选用版本文档配置，不要无意识粘贴一个旧配置片段。

Page 的 total 是满足条件的总数，records 是本页数据，count 可能需要额外查询。页码和 pageSize 要校验并设置上限；稳定排序最好加入唯一 ID 消歧。即使排序稳定，偏移分页仍可能受并发插删影响。

## 大列表与权限

深 offset 可能扫描并丢弃大量行，可用键集分页：按 created_at 与 id 的复合边界取下一批，并用匹配索引。是否返回精确 total 是 API 产品取舍。

Wrapper 不能直接接受客户端列名、apply/last 的原始 SQL。所有用户和租户条件都需可信构造；逻辑删除、租户插件等会影响生成 SQL，调试时以实际语句为准，不保证永远等于某条固定 SELECT *。

## 参考资料

- [MyBatis-Plus 条件构造器](https://baomidou.com/guides/wrapper/)
- [分页插件](https://baomidou.com/en/plugins/pagination/)
- [注解配置](https://baomidou.com/reference/annotation/)

## 模拟面试问题

1. 【基础】User::getName 在 Lambda Wrapper 中是否立即调用 getter？
2. 【应用】如何正确表达 A AND (B OR C)？
3. 【应用】selectPage 为什么可能没有实际分页？
4. 【深入】稳定排序为什么不能完全消除偏移分页的并发问题？
5. 【深入】哪些 Wrapper 写法会重新引入原始 SQL 风险？

