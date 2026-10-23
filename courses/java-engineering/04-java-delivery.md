---
id: java-delivery
title: "Maven、测试、Git 与证据式调试"
description: "把构建、验证和故障定位组织成可重复的开发流程。"
course: java-engineering
order: 4
date: 2026-10-23
tags: [Java, 后端]
---

# Maven、测试、Git 与证据式调试

> 课程基线：Java 17＋／Spring Boot 3.x（涉及 Spring 时采用 6.x、Jakarta 命名空间）。代码中标注的业务片段需要应用依赖与上下文，不等同于完整生产系统。

## 从能启动到能重复构建

Maven 用 groupId、artifactId、version 标识依赖，POM 描述构建。生命周期阶段与插件 goal 不同：test 执行此前相关阶段，package 生成包，install 放本地仓库，deploy 发布仓库。执行目标失败要看哪个插件、哪一阶段，而不是只看最后 BUILD FAILURE。

dependencyManagement 管版本约束，不等于直接引入所有依赖；Boot parent/BOM 帮助统一兼容版本。冲突通过 dependency:tree、effective-pom 定位，不要随机删 jar。构建使用的 JDK、测试 JDK 与部署 JRE 要一致兼容。

## 有意义的测试

```java
@Test
void rejectsNegativeQuantity() {
    assertThrows(IllegalArgumentException.class,
        () -> validateQuantity(-1));
}
```

validateQuantity 是被测业务方法。测试应覆盖正常、边界和失败：零库存、重复用户名、恰好到期、无权限订单。单元测试用 Fake 隔离慢外部依赖；集成测试验证真实映射、SQL 与事务，不能用全 Mock 证明数据库正确。

@BeforeEach 每个测试前，@BeforeAll 按测试类级运行；默认生命周期常要求 static，但 PER_CLASS 等配置可例外，原稿“一律必须 static”过于绝对。测试不要依赖固定执行顺序、真实时间流逝或外网稳定性。

## 实际开发：定位更新菜品失败

先记录请求、状态码和关联日志，确认是否进 Controller，再看 DTO、Service、SQL、影响行数及事务提交。断点 Step Over 看当前流程，Step Into 看实现，调用栈定位来路；快捷键随 IDE 可配置，不是语言规范。

调试过程中先提出可证伪假设，例如“数据库未改因为 DTO id 丢失”，检查对象与 SQL，而不是同时修改五个注解碰运气。AI 生成代码也需解释依赖、错误分支与资源生命周期。

## Git 与小范围 Spike

add 暂存，commit 创建本地历史，push 才更新远端；嵌套仓库、子模块各有独立状态。操作前确认根目录、status 和 diff，避免把另一个仓库的变更误当普通目录。本次网站改造禁止提交推送，与学习命令示例无关。

Spike 用时间盒验证一个高不确定问题，例如“分页插件在指定版本是否产生 LIMIT”。记录假设、最小实验、证据与结论，不把探索代码当可直接上线的系统。学习路线按能力验收，而非把“12—17 天”当精通框架的标准。

## 参考资料

- [Maven 生命周期](https://maven.apache.org/guides/introduction/introduction-to-the-lifecycle.html)
- [Maven 依赖机制](https://maven.apache.org/guides/introduction/introduction-to-dependency-mechanism.html)
- [JUnit 5 用户指南](https://docs.junit.org/5.12.2/user-guide/)
- [Git 文档](https://git-scm.com/docs)

## 模拟面试问题

1. 【基础】Maven phase 与 plugin goal 有何不同？
2. 【应用】如何定位传递依赖版本冲突？
3. 【应用】同一业务操作需要哪些边界与失败测试？
4. 【深入】全 Mock 的测试为什么不能证明 SQL 映射正确？
5. 【深入】如何把一次猜测式排障改为可证伪实验？

