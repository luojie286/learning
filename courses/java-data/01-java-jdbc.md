---
id: java-jdbc
title: "JDBC、参数绑定与连接生命周期"
description: "从一条查询理解 JDBC 模板工作、注入边界和资源释放。"
course: java-data
order: 1
date: 2026-10-23
tags: [Java, 后端]
---

# JDBC、参数绑定与连接生命周期

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## API、驱动与数据库

JDBC 是 Java 数据库访问 API，驱动把这些调用转换成数据库协议。Connection 表示连接，PreparedStatement 表示参数化语句，ResultSet 是结果游标。现代兼容驱动通常自动注册，不必固定手写 Class.forName。

```java
static List<Long> orderIds(DataSource ds, long userId) throws SQLException {
    String sql = "SELECT id FROM orders WHERE user_id = ? ORDER BY id DESC";
    try (Connection c = ds.getConnection();
         PreparedStatement ps = c.prepareStatement(sql)) {
        ps.setLong(1, userId);
        try (ResultSet rs = ps.executeQuery()) {
            List<Long> result = new ArrayList<>();
            while (rs.next()) result.add(rs.getLong("id"));
            return result;
        }
    }
}
```

DataSource 来自应用配置，片段需对应表。参数从 1 编号，游标初始在首行之前，先 next 再读。executeQuery 取得结果集；executeUpdate 通常返回影响行数，自动主键需专门请求并读取 generated keys。

## 为什么参数化能防注入

输入以数据值绑定，不作为 SQL 结构解释。不要把用户名拼进引号形成 SQL，也不要误以为用了 PreparedStatement 就可以安全拼其他不可信片段。动态列名、表名、排序方向不能用普通参数替代标识符，应用白名单映射。

ORDER BY ? 可以在某些数据库作为值表达式合法出现，但不能因此把输入字符串当列名排序。关键在语义而非“能否通过语法解析”。驱动可能客户端模拟或服务端预处理，安全性应按驱动与参数边界理解。

## 连接池与事务状态

池中 close 通常归还连接，仍必须调用，否则借用槽位耗尽。归还之前事务应完成，状态需由池或管理层正确复位。不要长期持有连接等待慢远程调用，也不要跨线程共享普通 Connection 而忽略契约。

手工事务先 setAutoCommit(false)，成功 commit，失败 rollback，再释放资源；回滚本身也可能失败，不能掩盖原始异常。Spring 管理事务时，避免绕开它另建连接造成两套互不关联的事务。

## 实际开发的观测

接口变慢可能是等待池中连接，不是 SQL 本身慢；分别测排队、执行、结果传输。查询超时、网络超时、池借用超时各自配置。大结果集还需考虑驱动流式策略与 fetchSize，不能假定设置一个数字就总能逐行从网络读取。

## 参考资料

- [java.sql API](https://docs.oracle.com/en/java/javase/17/docs/api/java.sql/java/sql/package-summary.html)
- [PreparedStatement API](https://docs.oracle.com/en/java/javase/17/docs/api/java.sql/java/sql/PreparedStatement.html)
- [DataSource API](https://docs.oracle.com/en/java/javase/17/docs/api/java.sql/javax/sql/DataSource.html)

## 模拟面试问题

1. 【基础】Connection、PreparedStatement、ResultSet 各做什么？
2. 【应用】executeUpdate 的返回值与自增主键有什么区别？
3. 【应用】动态排序列怎样避免 SQL 注入？
4. 【深入】连接池环境为什么仍需要 close？
5. 【深入】如何区分 SQL 慢与等待数据库连接慢？

