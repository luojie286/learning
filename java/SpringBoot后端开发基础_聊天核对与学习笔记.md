# Spring Boot 后端开发基础：聊天核对与学习笔记

> 整理日期：2026-10-23  
> 范围：本轮聊天中的 API、Swagger/Apifox、分页、Lombok、参数校验、MyBatis-Plus、DTO/VO、Mapper、Service。  
> 说明：这是一份**纠错版笔记**。主要内容来自聊天；明确的技术性纠错参考文末官方资料。此前上传的 `Spring Boot用法解析.txt` 只是另一份笔记的描述和链接，并非那份 27 KB 笔记的正文，本文件不声称核对了那个未提供的正文。

## 一、先掌握后端项目的整体结构

前端通过 HTTP 访问后端暴露的 API。典型调用链：

```text
浏览器 / 手机 App
    │ 发送 HTTP 请求（URL + 方法 + 参数）
    ▼
Controller（接收请求，参数绑定/校验）
    │
    ▼
Service 接口 → ServiceImpl（业务规则、事务等）
    │
    ▼
Mapper（数据库访问）
    │
    ▼
MySQL
    │ 查询结果逐层返回
    ▼
Controller → JSON 响应 → 前端
```

DTO 用于接收/传递特定业务数据，Entity 通常映射数据库表，VO 用于组织对外返回的数据。它们是常见的分工习惯，不是 Java 语言强制规定。

## 二、API 接口到底是什么？

**HTTP API 接口**是服务暴露给调用方的一组约定：请求方法、URL、参数、响应结构，以及认证和错误处理要求。例如：

```http
POST /api/user/login
Content-Type: application/json

{"username":"roger","password":"示例密码"}
```

这不是一段 Java 代码，而是 HTTP 请求的简写。完整 URL 可能是 `http://localhost:8080/api/user/login`：其中 `localhost` 是本机、`8080` 是端口、`/api/user/login` 是路径。`/api` 往往只是人为约定的前缀，并不具备 HTTP 内置的特殊含义。

常见 HTTP 方法：

| 方法 | 常见用途 | 示例 |
| --- | --- | --- |
| GET | 获取资源 | `GET /api/users/1` |
| POST | 提交处理或创建资源 | `POST /api/user/login` |
| PUT | 整体替换/更新资源 | `PUT /api/users/1` |
| PATCH | 部分更新资源 | `PATCH /api/users/1` |
| DELETE | 删除资源 | `DELETE /api/users/1` |

**注意**：POST 不仅用于“新增”；登录就是 POST 的典型用法。请求方法表达语义，具体行为取决于接口设计。

### Spring Boot 如何暴露 HTTP 接口？

```java
@RestController
@RequestMapping("/api/users")
public class UserController {
    @GetMapping("/{id}")
    public String getUser(@PathVariable Long id) {
        return "查询用户：" + id;
    }
}
```

请求 `GET /api/users/1` 时，Spring MVC 找到匹配的方法并执行。`@RestController` 适合返回响应体（例如字符串、JSON）；`@RequestMapping` 设置通用映射路径；`@GetMapping` 匹配 GET 请求；`@PathVariable` 读取路径中的参数。

前端通常通过 `fetch` 或 Axios 发请求，而不是跨机器直接执行后端的 Java 方法：

```javascript
const response = await fetch('/api/user/login', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ username: 'roger', password: '示例密码' })
});
const result = await response.json();
```

这里假设前端已配置同源部署或开发代理；否则需要使用实际后端地址并处理跨域配置。

## 三、Swagger、OpenAPI 与 Apifox

- **OpenAPI**：描述 HTTP API 的规范（路径、参数、请求体、响应等）。
- **Swagger UI**：将 OpenAPI 描述呈现为可阅读、可尝试发送请求的文档界面。Swagger 生态不等同于 OpenAPI 规范本身。
- **Apifox**：可导入/管理接口定义，发送调试请求，并提供 Mock、测试流程与协作功能（具体取决于产品版本）。

**导入 Apifox 之后，完全可以在 Apifox 中测试接口，不必再用 Swagger UI 重复发送同一请求。** 但接口文档不会代替后端实现、服务器启动或数据库配置。导入后也要注意同步接口变更。

文档可能来自代码生成的 OpenAPI，也可能由团队先行设计，二者都是可行流程。Swagger UI 地址与项目集成方式有关，不能一概认定为 `/swagger-ui.html`。

## 四、分页查询

分页查询是按页获取列表数据，避免一次性取回全部记录。

```http
GET /api/users/page?page=2&pageSize=10
```

- `page=2`：第 2 页。
- `pageSize=10`：每页 10 条。
- 传统偏移分页的起始偏移量是 `(page - 1) * pageSize`，因此为 10。

```sql
SELECT id, username
FROM `user`
ORDER BY id
LIMIT 10, 10;
```

建议指定稳定的 `ORDER BY`，否则同一页的数据顺序可能不稳定。

典型响应：

```json
{
  "total": 1000,
  "records": [
    { "id": 11, "username": "user11" }
  ]
}
```

这里 `records` 仅作示例，实际第二页最多 10 条，`total` 是满足筛选条件的总数量，而不是当前页条数。查询总数通常需要额外计数开销；真实系统也可能采用不返回精确总数的游标分页。

### MyBatis-Plus 写法

```java
Page<User> page = new Page<>(2, 10);
Page<User> result = userMapper.selectPage(page, null);
List<User> records = result.getRecords();
long total = result.getTotal();
```

**重要前提**：需配置相应的分页拦截器（通常是 `MybatisPlusInterceptor` + `PaginationInnerInterceptor`），并按使用的 MyBatis-Plus 版本添加适当依赖；否则不应认为调用 `selectPage` 就一定完成了 SQL 分页。

偏移分页在较大偏移量下可能变慢，对大量实时更新的列表还要考虑游标/键集分页。

## 五、`@Data`：Lombok 减少样板代码

```java
@Data
public class UserLoginDTO {
    private String username;
    private String password;
}
```

`@Data` 是 Lombok 注解，相当于组合了 `@Getter`、`@Setter`、`@ToString`、`@EqualsAndHashCode`、`@RequiredArgsConstructor` 的主要功能：为字段生成读取/修改方法等。

**纠错**：此前聊天说它会生成“全参构造方法”是错误的。`@RequiredArgsConstructor` 为未初始化的 `final` 字段和特定 `@NonNull` 字段生成所需参数构造方法。上面这种没有必需字段的简单类，通常产生无参构造，而不是所谓的所有字段构造。

实际意义不在于节省单个 getter/setter，而在于项目中有大量 DTO、VO、Entity 时，减少机械重复和维护成本。

**注意**：

- `@Data` 默认生成的 `toString()` 可能打印密码等敏感字段；日志中尤其需要小心。
- 在带有关联关系、代理对象或特殊相等性规则的 Entity 上，不宜不加区分地使用 `@Data`。
- 需要精细控制时可选 `@Getter`、`@Setter`，或为敏感属性添加 `@ToString.Exclude` 等。

## 六、参数校验注解：`@NotBlank`、`@Size`、`message`

```java
@Data
public class UserLoginDTO {
    @NotBlank(message = "用户名不能为空")
    @Size(max = 50, message = "用户名不能超过50个字符")
    private String username;

    @NotBlank(message = "密码不能为空")
    @Size(max = 100, message = "密码不能超过100个字符")
    private String password;
}
```

这些注解来自 Bean Validation（Spring Boot 3 常见包名为 `jakarta.validation.constraints`）。`message` 是**校验不通过时的约束错误提示**，不是自己直接向前端发送响应的代码。

| 注解 | 检查内容 | 注意 |
| --- | --- | --- |
| `@NotNull` | 不能为 `null` | 空字符串仍可能通过 |
| `@NotEmpty` | 非 `null` 且非空 | 纯空格字符串可通过 |
| `@NotBlank` | 非 `null`，且有至少一个非空白字符 | 适合用户名等文本 |
| `@Size(min=6,max=20)` | 字符串长度/集合或数组大小在区间内 | **单独使用时允许 `null`** |
| `@Email` | 邮件地址格式约束 | 如必填需结合 `@NotBlank` 等 |
| `@Min(1)` / `@Max(100)` | 数值下限/上限 | 用于数值，而非字符串长度 |

### 最重要的纠错：`@Size` 没有 `value`

```java
@Size(max = 50)             // 正确：最多50
@Size(min = 6, max = 20)   // 正确：6至20
@Size(min = 50, max = 50)  // 正确：恰好50
@Size(value = 50)          // 错误：标准 @Size 没有 value 属性
@Size(50)                  // 错误：不能用位置参数代替 max
```

Java 注解并非都有 `value` 属性。只有**注解确实声明了 `value()` 元素**时，才能使用 `value=...`；符合省略条件时才能写成 `@Xxx(...)`。例如 `@GetMapping("/login")` 可以这么写，但 `@Size(50)` 不行。

### 校验如何触发？

```java
@PostMapping("/login")
public LoginVO login(@Valid @RequestBody UserLoginDTO dto) {
    // 校验通过后才会进入这里的正常业务逻辑
    return new LoginVO();
}
```

通常需要加入 `spring-boot-starter-validation` 依赖。`@RequestBody` 负责把 JSON 请求体绑定为对象；`@Valid` 请求进行约束校验。校验失败一般抛出框架处理的异常（如 `MethodArgumentNotValidException`），**并不意味着会自动返回之前示例中的自定义 `{code:400,msg:...}` JSON**。若要统一错误结构，需要约定响应模型并配置如 `@RestControllerAdvice` 的异常处理。

校验 DTO 不能取代 Service 业务校验、权限校验和数据库约束。

## 七、Entity、DTO、VO 与 Token

- **Entity**：面向数据库持久化的实体，例如 `User` 对应 `user` 表。
- **DTO**：某次操作的输入/传输数据，例如 `UserLoginDTO` 包含登录凭据。
- **VO**：面向接口响应的返回数据，例如 `LoginVO` 或 `UserVO`。

登录成功可在响应体的 `LoginVO` 中带 token：

```java
@Data
public class LoginVO {
    private Long id;
    private String username;
    private String token;
}
```

客户端后续访问受保护接口，通常在请求头传递：

```http
Authorization: Bearer <access-token>
```

**注意**：并非“VO 必须带 Token”。普通 `UserVO`、`OrderVO` 通常不需要；而登录响应也可以使用 HttpOnly Cookie 等其他凭证传递方案，具体由认证设计决定。JWT 是一种常见 Token 格式，但 Token 不等于 JWT。后端绝不应把数据库中的密码哈希、密钥等作为普通用户信息返回。

## 八、MyBatis-Plus：`@TableName`、`@TableId`、`@Mapper`

```java
@Data
@TableName("`user`")
public class User {
    @TableId(type = IdType.AUTO)
    private Long id;
    private String username;
    private String password; // 实际存储应为安全的密码哈希
}
```

### `@TableName`

指定 Java 实体映射的表名。反引号是 **MySQL 标识符引用语法**，如 `` `user` ``；它并不是 Java 注解的特殊语法。并非所有数据库都使用 MySQL 式反引号，且 `user` 在不同数据库上下文中的关键字地位可能不同。加引号主要用于避免标识符与保留字等冲突。

**纠错**：MyBatis-Plus 一般默认支持驼峰映射，如 `UserInfo` 对应 `user_info`，通常**不必**为了这一点强制写 `@TableName("user_info")`。如果实际表名不同、全局配置修改过，显式声明才很有帮助。

### `@TableId(type = IdType.AUTO)`

说明 `id` 是主键，并使用数据库生成的自增主键策略；数据库主键列也应相应支持自增。

其他常见策略：

- `IdType.ASSIGN_ID`：由框架分配 ID，默认采用雪花算法相关生成器。
- `IdType.INPUT`：插入前由应用提供 ID。

注解描述的是 **MyBatis-Plus 的映射策略**，不会单凭这行 Java 代码自动把现有数据库列改成自增。

### `@Mapper` 与 `BaseMapper<User>`

```java
@Mapper
public interface UserMapper extends BaseMapper<User> {
}
```

- `@Mapper`：让该接口被 MyBatis 的 Mapper 注册机制发现（需在正确扫描配置下）。
- `extends BaseMapper<User>`：继承通用 CRUD 方法，例如 `selectById`、`selectList`、`insert`、`updateById`、`deleteById`。
- 没有自己编写 `UserMapperImpl` 也能调用，是因为 MyBatis 在运行时创建 Mapper 代理对象。

```java
User user = userMapper.selectById(1L);
```

概念上执行的是“按主键查 `User` 对应表”，具体 SQL 字段、逻辑删除条件等受配置影响；不能保证固定生成某一条字面完全相同的 SQL。

`@Mapper` 和启动类上的 `@MapperScan("com.sky.mapper")` 可以作为注册 Mapper 的不同方式；正常配置下不需要每个接口同时重复配置。

复杂 SQL 可自定义 Mapper 方法，并用 SQL 注解或 XML 映射实现。声明一个方法本身不会自动让任意复杂查询获得实现。

## 九、Service 接口、ServiceImpl 与 `@Service`

```java
public interface UserService {
    User login(UserLoginDTO userLoginDTO);
}
```

这是**业务接口的契约**：要求某个实现提供 `login` 方法，接收 `UserLoginDTO`、返回 `User`。它不包含实际登录逻辑；返回 `User` 是当前项目的设计选择，也可以在其他项目返回专门的业务结果。

```java
@Service
public class UserServiceImpl implements UserService {
    private final UserMapper userMapper;

    public UserServiceImpl(UserMapper userMapper) {
        this.userMapper = userMapper;
    }

    @Override
    public User login(UserLoginDTO dto) {
        // 查询用户、验证密码、判断账号状态等
        throw new UnsupportedOperationException("待实现登录业务");
    }
}
```

此处特意用 `UnsupportedOperationException` 明确表示**业务尚未实现**，避免把示意代码误认为一个可以直接上线的登录流程。

`@Service` 是 Spring 的组件标记注解，是 `@Component` 的特化：**当类处于组件扫描范围内时，Spring 可以自动发现并注册该业务实现类为 Bean**。如果实现了 `UserService`，Controller 可以依赖接口类型接收其实现。

```java
@RestController
@RequestMapping("/api/user")
public class UserController {
    private final UserService userService;

    public UserController(UserService userService) {
        this.userService = userService;
    }

    @PostMapping("/login")
    public String login(@Valid @RequestBody UserLoginDTO dto) {
        User user = userService.login(dto);
        return "登录逻辑执行完成，用户ID=" + user.getId();
    }
}
```

**说明**：这里也仅是演示分层调用关系的骨架，不能当作真实登录 API。生产登录接口通常需要安全密码哈希校验、统一异常处理、Token 或 Session 发放、防暴力破解保护等，不能返回实体中的密码字段。

### Service 接口和实现类是不是必须分开？

**不是。** 小项目完全可以写一个 `@Service` 类，无单独接口。分离接口与实现有助于定义契约、隔离依赖、支持多个实现和测试替身，但无须为了“企业规范”机械增加文件。

### 为什么注入接口也能拿到实现？

Controller 依赖 `UserService` 类型，Spring 在容器中查找符合该类型的 Bean。例如 `UserServiceImpl implements UserService` 且已注册，就可通过构造器注入。若有多个同类型 Bean，可能需要 `@Primary` 或 `@Qualifier` 指定。

### 为什么推荐构造器注入？

在新代码中常优先使用构造器注入：依赖显式、方便测试，也能较好支持 `final` 字段。`@Autowired` 字段注入可以运行，但不是唯一方式；只有一个构造器的组件通常不用显式写 `@Autowired`。

## 十、将知识串起来：登录流程与角色分工

```text
1. 前端提交账号密码
     POST /api/user/login
2. Spring MVC 选择 Controller 方法
3. @RequestBody 把 JSON 变成 UserLoginDTO
4. @Valid 按 @NotBlank/@Size 校验格式
5. Controller 调用 userService.login(dto)
6. Spring 使用 UserServiceImpl 实现业务
7. Service 通过 UserMapper 查数据库
8. Service 验证密码哈希、账号状态等
9. 登录成功后，认证模块发放凭证
10. Controller 组装响应（如 LoginVO）
11. JSON 返回前端
12. 前端后续请求附带凭证，后端进行验证和授权
```

关键区别：**Mapper 做数据访问；Service 做业务判断；Controller 面向 HTTP；DTO/VO 是数据结构；Token 用于认证。**

## 十一、原聊天中需要特别修正的说法

| 原说法 / 示例 | 判断 | 修正 |
| --- | --- | --- |
| `@Size(value = 50)` / `@Size(50)` 表示恰好 50 | **错误** | 标准 `@Size` 根本没有 `value`，写 `@Size(min=50,max=50)` 才是恰好 50 |
| `@Data` 自动生成全参构造方法 | **错误** | 组合的是 `@RequiredArgsConstructor`，不是 `@AllArgsConstructor` |
| `UserInfo` 映射 `user_info` 需要加 `@TableName` | **错误** | 默认驼峰映射下可自动对应，视配置而定 |
| `@Valid` 失败后自动返回自定义 `code/msg` JSON | **不严谨** | 通常发生校验异常，统一格式由异常处理配置决定 |
| 只调用 MyBatis-Plus `selectPage` 即可分页 | **缺前提** | 需要有效的分页插件/拦截器配置 |
| `@Service` 一加就必然扫描注册 | **缺前提** | 需在 Spring 管理的组件扫描范围中 |
| DTO、VO、Entity 必须按此划分 | **不严谨** | 是常见架构实践，不是 Java/Spring 的强制规范 |
| 所有企业项目都必须写 Service 接口 + Impl | **不严谨** | 可按项目复杂度决定，不是强制要求 |
| `SELECT * FROM user` 形式在任意情况下都固定 | **简化示意** | SQL 受映射、字段、逻辑删除等配置影响 |
| 示例中明文密码写入数据库 | **不安全** | 真实系统存储强密码哈希，不存明文；示例仅用于理解调用链 |
| `@TableName("`user`")` 表示 `user` 必然是 MySQL 保留字 | **不严谨** | 反引号是 MySQL 标识符引用符，使用原因与具体数据库、标识符冲突有关 |
| 登录成功必须在 VO 返回 token | **不绝对** | 常见但非唯一认证方案，也可采用安全 Cookie/Session |

## 十二、建议的学习顺序和自测

推荐顺序：

1. 能读懂 `POST /api/user/login`、HTTP 请求与响应。
2. 熟悉 Controller 中 `@RestController`、`@RequestMapping`、`@GetMapping`、`@PostMapping`、`@RequestBody`、`@PathVariable`。
3. 理解 DTO、VO、Entity 的差异。
4. 能解释 `@Data`、`@NotBlank`、`@Size`、`@Valid` 各自在哪个环节生效。
5. 会用 `@TableName`、`@TableId`、`@Mapper`、`BaseMapper` 完成简单 CRUD。
6. 理解 Service、ServiceImpl、`@Service` 和构造器注入。
7. 练习 Apifox 调用登录、查询、分页接口并阅读报错。
8. 再学习认证授权、事务、异常处理、Redis 等。

**自测题**：

1. `POST /api/user/login` 是 Java 代码吗？路径与 HTTP 方法各表达什么？
2. `@Size(max=50)` 与 `@NotBlank` 为什么常常需要组合使用？
3. `@Size(50)` 能编译吗？为什么？
4. `@Valid` 和 `message` 的职责分别是什么？
5. Mapper 是接口，为什么无需手写实现类也可操作数据库？
6. `@Mapper` 与 `BaseMapper<User>` 分别负责什么？
7. 为什么 Controller 通常不直接编写数据库业务？
8. `UserService` 和 `UserServiceImpl` 为什么是两个概念？
9. 登录 Token 什么时候发放？后续通常在哪里传？
10. 为什么分页要指定稳定排序？为什么 `total` 不等于 `records.size()`？

## 十三、核对参考（官方）

- Lombok `@Data`：https://projectlombok.org/features/Data
- Jakarta Validation `@Size`：https://jakarta.ee/specifications/bean-validation/4.0/apidocs/jakarta.validation/jakarta/validation/constraints/size
- Spring 组件扫描与 stereotype 注解：https://docs.spring.io/spring-framework/reference/core/beans/classpath-scanning.html
- MyBatis-Plus 配置与 ID 策略：https://www.baomidou.com/reference/
- MyBatis-Plus 分页插件：https://baomidou.com/plugins/pagination/

> 学习提醒：先能准确描述整个调用链，再记住注解细节；不要把示意代码和可直接用于生产的实现混为一谈。
