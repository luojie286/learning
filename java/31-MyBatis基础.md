# 31 · MyBatis 基础

> 一句话概括：MyBatis 是建立在 JDBC 之上的 SQL Mapper（半自动 ORM）框架——SQL 仍由你写，它替你完成连接管理、参数绑定、结果映射和资源释放。

## 一、为什么需要它

纯 JDBC 每次都要重复：`Connection → PreparedStatement → 绑定 → 执行 → ResultSet → 逐字段 set 进对象 → 关资源`。项目变大后全是模板代码。MyBatis 不取代 JDBC（底层仍走 JDBC 与驱动通信），而是把重复工作封装掉：

```java
// JDBC：十几行     vs     MyBatis：一行
User user = userMapper.selectById(1L);
```

不知道 JDBC 就看不懂连接池、事务、MyBatis 执行流程和 Mapper 底层原理——所以 JDBC 必须先手写一遍（见 `30`），但不用久留。

## 二、核心：Mapper 接口 + 注解

```java
@Mapper
public interface UserMapper {
    @Select("SELECT * FROM user WHERE id = #{id}")
    User selectById(Long id);

    @Insert("INSERT INTO user(username, age) VALUES(#{username}, #{age})")
    int insert(User user);

    @Update("UPDATE user SET username=#{username}, age=#{age} WHERE id=#{id}")
    int update(User user);

    @Delete("DELETE FROM user WHERE id=#{id}")
    int deleteById(Long id);
}
```

**接口没有实现类为什么能调用？** MyBatis（与 Spring 集成后）会为 Mapper 接口生成**代理对象**，由代理根据 SQL 映射去执行数据库操作。增删改返回的 `int` 是影响行数。

`@Insert("""...""")` 里的 `"""..."""` 是 Java 文本块（Text Block），本质还是 String，方便写多行 SQL。`#{username}` 是 MyBatis 参数占位符——`void insert(User user)` 时 `#{username}` 对应 `user.getUsername()`，底层转成 `?` 走 `PreparedStatement` 绑定（所以天然防注入）。

## 三、结果映射

数据库一行 → Java 对象，MyBatis 自动按列名/字段名对应。不一致时两条路：

- 开驼峰映射：`user_name → userName`；
- SQL 别名：`SELECT user_name AS userName FROM user`。

复杂 SQL 可写 **XML Mapper**（注解适合简单 SQL，复杂动态 SQL 仍常用 XML）：

```xml
<select id="selectById" resultType="User">
    SELECT * FROM user WHERE id = #{id}
</select>
```

`id` 对应接口方法名，调用 `userMapper.selectById(1L)` 即执行。

## 四、整体调用链与分层

```text
Controller → Service → Mapper → MyBatis → JDBC → MySQL Driver → MySQL
```

登录示例：前端提交账号密码 → `LoginController` → `UserService` 校验 → `UserMapper` 查询 → 返回 token。Controller 处理 HTTP、Service 写业务规则、Mapper 只做数据访问。

## 五、学习路线

```text
MySQL 基础 → JDBC 基础 → MyBatis → Spring + MyBatis → Spring Boot + MyBatis
```

重点掌握：`Mapper`、`@Select/@Insert/@Update/@Delete`、`#{}`、XML Mapper、参数传递、结果映射、动态 SQL。实际项目最常见组合：**Spring Boot + MyBatis + MySQL**（可再引入 MyBatis-Plus 减少样板 CRUD，但复杂查询与数据正确性仍离不开数据库知识）。

## 📌 易错点

- "学了 MyBatis 就不用会 SQL"——恰恰相反，SQL 是你自己写的。
- `#{ }` 是参数绑定（安全）；`${ }` 是字符串替换，**不可拼接不可信输入**。
- 接口没有实现类不是"框架魔法"，是动态代理。
- Mapper 只放数据访问，业务规则属于 Service——别在 SQL 里堆业务。

## 🧠 速记口诀

```text
MyBatis 半自动 ORM：SQL 你写，杂活它干
#{} 走 PreparedStatement 绑定，${} 是裸替换要当心
Mapper 接口靠代理，注解简单 XML 复杂
链路记牢：Controller → Service → Mapper → MyBatis → JDBC → MySQL
```

## ✅ 自测问题

1. MyBatis 和 JDBC 是什么关系？为什么说"不取代"？
2. Mapper 接口没有实现类，调用时背后发生了什么？
3. `#{id}` 和 `${id}` 有什么区别？哪个能防 SQL 注入？
4. 数据库字段 `user_name` 和 Java 字段 `userName` 不一致怎么办？
5. 注解写 SQL 和 XML 写 SQL 各自适合什么场景？

> 来源：原始《JDBC 与 MyBatis 学习笔记》(JDBC与MyBatis学习笔记.md §5-20)、《Java 后端基础学习笔记》(Java后端基础_本次聊天学习笔记.md §2)、《Java 后端学习笔记：Spring、Web 请求处理与数据库设计》(Java后端学习笔记_Spring与Web请求处理_2026-10-08.md §5)。
