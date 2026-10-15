# Java `ThreadLocal` 与 Spring MVC JWT 拦截器：对话纠错学习笔记

> 整理日期：2026-10-14  
> 学习背景：从 C++ 过渡到 Java 后端，围绕 `BaseContext` 和 `JwtTokenAdminInterceptor` 理解 Java 语法、线程上下文、Spring MVC 拦截器与 JWT 认证。  
> **资料范围**：以本次聊天贴出的代码及讨论为主；对其中的易错结论，依据文末 JDK、Lombok、Spring 官方文档核对。**未提供** `JwtUtil`、`EmployeeMapper`、`Result`、项目拦截器注册配置的完整实现，相关细节以“依赖具体实现”标明。

## 目录

1. [对话正确性核查：关键修正](#1-对话正确性核查关键修正)
2. [BaseContext 完整代码与设计目的](#2-basecontext-完整代码与设计目的)
3. [Java 基础语法与 C++ 对照](#3-java-基础语法与-c-对照)
4. [ThreadLocal 原理、API 与清理](#4-threadlocal-原理api-与清理)
5. [为什么不用 Lombok 的 @Data](#5-为什么不用-lombok-的-data)
6. [JWT 管理端拦截器逐段解析](#6-jwt-管理端拦截器逐段解析)
7. [Spring MVC 请求执行链与拦截器注册](#7-spring-mvc-请求执行链与拦截器注册)
8. [边界条件、安全性与改进建议](#8-边界条件安全性与改进建议)
9. [自测题及参考答案](#9-自测题及参考答案)
10. [官方资料](#10-官方资料)

---

## 1. 对话正确性核查：关键修正

| 对话中的表述 | 核查结果 | 正确理解 |
|---|---|---|
| `ThreadLocal` 自带 `set/get/remove` 方法 | ✅ 正确 | `set(T)`、`get()`、`remove()` 是 JDK `ThreadLocal` 的实例方法，作用于**当前线程**所关联的值。 |
| `remove()` 结束线程或进程？回答“不会” | ✅ 正确 | 它只移除**当前线程对应此 `ThreadLocal` 的值**，线程和 JVM 均继续运行。 |
| `get()` 会因不同线程而返回不同的值 | ✅ 正确，但需补充 | 同一个线程多次读取通常获得它之前保存的值；若没有设置，当前代码默认返回 `null`；`set()` 或 `remove()` 会影响以后读取的结果。 |
| `static ThreadLocal` 意味着所有线程共享同一个用户 ID | ❌ 如果这样理解就是错误 | 共享的是 `ThreadLocal` **对象引用**，而非每个线程读到的值。 |
| `static` 表示“整个程序只有一个对象” | ⚠️ 简化过度 | 这里字段属于**声明它的类（以及对应的类加载器上下文）**，不是严格意义上的全进程唯一对象；一般单应用开发时可按类共享字段理解。 |
| Java 普通 `Long id` “所有线程都共享” | ❌ 不准确 | 是否共享取决于它是局部变量、实例字段还是静态字段，以及对象是否被线程共享。**方法内局部变量一般不因同名而共享。** |
| 给 `@Data` 类添加一个 `static final ThreadLocal`，会生成 `getCURRENT_ID()` | ❌ 错误 | **Lombok `@Data` 会完全跳过 `static` 字段**，不为其生成 Getter/Setter。此前给出的“会生成该方法”的示例应撤销。 |
| `final` 基本类型值不能重新赋值；引用不能改指向 | ✅ 正确 | 但引用指向的对象是否能被修改，要看对象自身的设计。`final` 不是“对象不可变”。 |
| 类、方法定义的右花括号后不必写 `;` | ✅ 正确 | Java 的变量声明与 `return` 等语句通常需要分号；类和带方法体的方法定义不要求在 `}` 后加分号。某些位置额外分号合法，但无必要。 |
| `@Component` 后拦截器就会生效 | ❌ 不充分 | `@Component` 只让它成为 Spring Bean；一般还需要将拦截器注册到 Spring MVC 的执行链并配置路径。 |
| `afterCompletion()` “请求结束必执行” | ❌ 不准确 | 只有当**当前拦截器的 `preHandle()` 成功返回 `true`**时，才进入其后续完成回调资格；异步请求要额外考虑。 |
| 构造器注入 + `final` “保证依赖不为 null” | ⚠️ 不准确 | `final` 保证字段赋值规则，不自动保证非空。正常 Spring 依赖解析会注入 Bean，但不能将非空归因于 `final`。 |
| `JwtUtil.parseToken()` 一定校验签名、issuer 和过期时间 | ⚠️ 无法据此确认 | 这取决于**未提供的 `JwtUtil` 实现和解析配置**；不能仅凭方法名或注释下结论。 |
| `Result.error()` 会生成固定 `{ "code":0,"msg":"..." }` | ⚠️ 不能确定 | JSON 字段名及状态码取决于 `Result` 类、Jackson 配置及返回对象。 |
| JWT 里 `exp` 以日期字符串表示 | ❌ 容易误导 | JWT 标准中的 `exp` 是 **NumericDate**（以秒计的时间数值），某些解析库会转换为 Java `Date`。 |
| `long` 不能放进 `ThreadLocal<Long>` | ❌ 如此理解不对 | `setCurrentId(long id)` 合法；Java **自动装箱**把 `long` 转为 `Long`。选择 `Long` 还是 `long` 取决于是否允许 `null`。 |

**结论**：本次对话关于 `final`、工具类、`ThreadLocal` 三个基础方法和 JWT 拦截流程的大方向基本正确。需要重点更正的是 **`@Data` 对静态字段的行为、普通变量是否线程共享、拦截器注册及回调边界、未提供代码不能断言的 JWT 校验行为**。

---

## 2. BaseContext 完整代码与设计目的

```java
package com.sky.context;

public final class BaseContext {

    private static final ThreadLocal<Long> CURRENT_ID =
            new ThreadLocal<>();

    private BaseContext() {
    }

    public static void setCurrentId(Long id) {
        CURRENT_ID.set(id);
    }

    public static Long getCurrentId() {
        return CURRENT_ID.get();
    }

    public static void removeCurrentId() {
        CURRENT_ID.remove();
    }
}
```

**用途**：以当前正在执行的 Java 线程为范围，保存一个员工/登录用户 ID，随后可以在 Controller、Service 等调用链中读取，无需层层传入该参数。在本次代码的管理端拦截器中，它保存的是 **`employeeId`**。

- `setCurrentId(id)`：把 ID 绑定到**当前执行线程**。
- `getCurrentId()`：读取**当前执行线程**此前绑定的 ID；本实现未设置时返回 `null`。
- `removeCurrentId()`：解除**当前线程**与此 `ThreadLocal` 值的关联，避免线程复用时读到旧身份。

它是一个**工具类**：使用类方法、禁止外部实例化、禁止继承。注意：这并不意味着 `ThreadLocal` 是全局用户 ID，也不意味着它能跨线程自动传播用户信息。

### 理解这行声明

```java
private static final ThreadLocal<Long> CURRENT_ID = new ThreadLocal<>();
```

从左到右拆解：

| 部分 | 作用 |
|---|---|
| `private` | 字段仅在 `BaseContext` 内部可直接访问。 |
| `static` | 字段属于类，而不是 `BaseContext` 的某个实例。 |
| `final` | 字段引用初始化后不能重新赋值。 |
| `ThreadLocal<Long>` | 字段类型：维护“当前线程 → `Long` 值”的线程局部存储机制。 |
| `CURRENT_ID` | 变量名，常量风格命名；这里“常量”的是**引用**而非里面的 ID。 |
| `new ThreadLocal<>()` | 创建 `ThreadLocal` 对象；`<>` 根据左侧泛型类型推断为 `<Long>`。 |
| `;` | 结束字段声明语句。 |

### 为什么 `static final` 依然能调用 `set()`？

```java
CURRENT_ID.set(100L);          // ✅ 修改当前线程绑定的值
CURRENT_ID.remove();           // ✅ 移除当前线程的绑定
// CURRENT_ID = new ThreadLocal<>(); // ❌ final 字段不能重新赋值
```

变量持有的 `ThreadLocal` **引用**不变，但通过该对象执行的方法可以修改与线程相关联的状态。

---

## 3. Java 基础语法与 C++ 对照

### 3.1 `package` 与 `namespace`

```java
package com.sky.context;
```

Java 包名用于组织类和减少命名冲突。常见项目路径：

```text
src/main/java/com/sky/context/BaseContext.java
```

可粗略类比 C++ 的 `namespace`，但并不完全相同：Java 包名还与源码组织、类加载及导入规则有关。

### 3.2 `public final class`、私有构造器

```java
public final class BaseContext {
    private BaseContext() {}
}
```

- `public`：其他包中的代码也可访问此类（在可访问的模块/类路径前提下）。
- `final class`：不能被其他类 `extends` 继承；C++ 可用 `class A final { ... };` 类比。
- `private BaseContext()`：其他类不能直接调用 `new BaseContext()`；用于工具类的防实例化设计。
- `static` 方法可通过 `BaseContext.getCurrentId()` 调用，不需要创建对象。

不能把 `final class` 误解为“这个类里的所有数据都不能改变”。

### 3.3 Java 的分号规则

**需要 `;` 的典型例子**：

```java
int x = 10;
Long id = 12L;
CURRENT_ID.set(id);
return CURRENT_ID.get();
```

**不需要在右花括号后加 `;` 的典型例子**：

```java
class Example {
    Example() { }

    void run() {
        if (true) {
            System.out.println("OK");
        }
    }
}
```

对比：C++ 的类定义通常以 `};` 收尾；Java 的普通类定义以 `}` 收尾。Java `for` 循环头部含分号（例如 `for (int i = 0; i < 3; i++)`），不要误解为“`for` 完全不用分号”。

### 3.4 `final`：基本类型与引用类型

```java
final int number = 10;
// number = 20; // 编译失败：不能再次赋值

final java.util.List<String> names = new java.util.ArrayList<>();
names.add("Alice"); // 合法：修改对象状态
// names = new java.util.ArrayList<>(); // 编译失败：不能更换引用
```

**口诀：`final` 约束的是变量能否被重新赋值；它不自动保证对象不可变。**

### 3.5 `long` 与 `Long`、泛型和自动装箱

```java
long a = 10L;       // 基本类型，不可能为 null
Long b = 10L;       // 包装类型，可以为 null
ThreadLocal<Long> id = new ThreadLocal<>();
id.set(a);          // 合法：a 自动装箱为 Long
```

- Java 泛型不能直接写 `ThreadLocal<long>`；要写 `ThreadLocal<Long>`。
- `setCurrentId(long id)` 与 `setCurrentId(Long id)` **均可编译**。
- 若输入不应该是 `null`，使用 `long` 可避免传入 `null`；若允许缺省值，使用 `Long` 更合适。实际选择依业务约定。
- C++ 的模板可以接收基础数值类型，这一点与 Java 泛型不同。

### 3.6 `this`、`@Override` 和 `throws IOException`

```java
private final JwtUtil jwtUtil;

public JwtTokenAdminInterceptor(JwtUtil jwtUtil, ...) {
    this.jwtUtil = jwtUtil;
}
```

`this.jwtUtil` 是当前对象字段，右侧的 `jwtUtil` 是构造器参数。此处用参数初始化字段。

```java
@Override
public boolean preHandle(...) throws IOException { ... }
```

- `@Override`：告诉编译器此方法是覆盖父类/接口声明的方法；签名不匹配时可帮助发现错误。
- `throws IOException`：方法可能向调用方抛出受检异常（例如写 HTTP 响应时出现 I/O 错误），不是“在这里一定抛出异常”。
- `implements HandlerInterceptor`：实现 Spring MVC 提供的接口；接口里的默认方法允许只重写需要的回调。

---

## 4. ThreadLocal 原理、API 与清理

### 4.1 一个对象、多个线程各有自己的值

```text
            BaseContext.CURRENT_ID（同一 ThreadLocal 对象）
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
    工作线程 A             工作线程 B            工作线程 C
    值：100L               值：200L               未设置 → null
```

这是**逻辑示意图**，不是准确的 JVM 内存布局。实现上，线程对象维护自己的 ThreadLocal 相关存储。不要把它理解成 `ThreadLocal` 内部简单维护了一个跨线程共享且不隔离的 `Long` 字段。

错误推论：

```java
private static Long currentId;
```

就等价于 `ThreadLocal<Long>`。**不等价**：如果多个线程读写同一个 `static Long` 字段，它们确实可能相互覆盖；`ThreadLocal` 会隔离当前线程读取的值。另一方面，**方法的局部变量本就有各自执行上下文**，不能说“所有普通变量都在线程间共享”。

### 4.2 三个关键 API

| 方法 | 所作用的范围 | 返回值 / 后果 |
|---|---|---|
| `set(T value)` | 当前线程 | 保存/覆盖这个线程的值；返回 `void`。 |
| `get()` | 当前线程 | 返回这个线程的值。此处首次读取且未设置过时返回 `null`。 |
| `remove()` | 当前线程 | 删除这个线程的绑定；返回 `void`，**不会终止线程或进程**。 |

可以运行的示例：

```java
public class ThreadLocalDemo {
    private static final ThreadLocal<Long> ID = new ThreadLocal<>();

    public static void main(String[] args) throws InterruptedException {
        Thread a = new Thread(() -> {
            try {
                ID.set(100L);
                System.out.println("线程A: " + ID.get()); // 100
            } finally {
                ID.remove();
            }
        });

        Thread b = new Thread(() -> {
            try {
                ID.set(200L);
                System.out.println("线程B: " + ID.get()); // 200
            } finally {
                ID.remove();
            }
        });

        a.start();
        b.start();
        a.join();
        b.join();

        System.out.println("main线程: " + ID.get()); // null
    }
}
```

输出行的**先后顺序**不固定，但每个线程读到的值彼此独立。`join()` 是为了等待两个子线程完成。

### 4.3 “get 是动态的”如何正确理解

`get()` **不会按时间自动变化**，而是：读取“当前执行线程”此时与该 `ThreadLocal` 关联的值。例如：

```java
ID.set(1L);
ID.get();          // 1L
ID.set(2L);
ID.get();          // 2L
ID.remove();
ID.get();          // null（当前使用 new ThreadLocal<>()）
```

如果通过 `ThreadLocal.withInitial(...)` 提供默认值，那么清理后再次 `get()` 可能重新初始化为非 `null` 值；因此“任何 `ThreadLocal` 未设置时一定为 `null`”也不严谨。

### 4.4 为什么 Web 项目要 `remove()`

```text
请求 A（用户100） → Tomcat 工作线程 T → set(100) → 业务处理 → remove()
请求 B（用户200） → Tomcat 工作线程 T → set(200) → 业务处理 → remove()
```

容器会复用工作线程。如果请求结束后不清理，即使后一次请求某条分支没有执行 `set()`，代码仍可能读到上一次请求留下的身份信息。长期保留大型线程局部对象也可能造成资源滞留。

一般临时使用场景建议 `try/finally`：

```java
try {
    ID.set(100L);
    // 使用当前线程的上下文
} finally {
    ID.remove();
}
```

在 Web MVC 拦截器中可以利用其生命周期回调做清理，但**必须理解回调何时才会触发**（见第 7、8 节）。

### 4.5 Java 与 C++ 类比

C++：

```cpp
thread_local long long currentId = 0;
```

可以帮助理解“每个线程各自拥有值”。但不是严格等价：Java `ThreadLocal<Long>` 是对象/API 机制，并且当前代码的“未设置”状态为 `null`；上例 C++ 初始值为 `0`，语义不同。如果需要严格区分“有值/无值”，C++ 可考虑 `std::optional` 等方案。

---

## 5. 为什么不用 Lombok 的 `@Data`

典型 DTO/实体类：

```java
import lombok.Data;

@Data
public class EmployeeDTO {
    private Long id;
    private String name;
}
```

`@Data` 会综合生成常用的 `getter`、非 `final` 实例字段的 `setter`、`equals()`、`hashCode()`、`toString()` 和适当的构造器。它是为了减少**数据对象**的样板代码。

而 `BaseContext` 的情况不同：

```java
private static final ThreadLocal<Long> CURRENT_ID = new ThreadLocal<>();
```

> **重点纠错：官方 Lombok 文档明确说明，`@Data` 会完全跳过 `static` 字段，不会为 `CURRENT_ID` 自动生成 `getCURRENT_ID()`。**

即使使用其他代码生成方式，自动获取“`ThreadLocal<Long>` 对象本身”也并非我们想要的接口。我们的真实业务 API 是：

```java
BaseContext.setCurrentId(100L); // 存一个 Long
Long id = BaseContext.getCurrentId(); // 取一个 Long
BaseContext.removeCurrentId();  // 清理
```

这是一种**封装**：对外公开操作当前身份的语义，不公开底层存储容器。还要注意 `@Data` 并不保证生成工具类需要的私有构造器，因此不能拿它代替工具类设计。

---

## 6. JWT 管理端拦截器逐段解析

分析对象：聊天中给出的 `JwtTokenAdminInterceptor implements HandlerInterceptor`，不是一套已经验证可运行的完整项目。下面按照代码**实际执行顺序**解释。

### 6.1 类声明及三个依赖

```java
@Component
public class JwtTokenAdminInterceptor implements HandlerInterceptor {
    private final JwtUtil jwtUtil;
    private final EmployeeMapper employeeMapper;
    private final ObjectMapper objectMapper;

    public JwtTokenAdminInterceptor(
            JwtUtil jwtUtil,
            EmployeeMapper employeeMapper,
            ObjectMapper objectMapper) {
        this.jwtUtil = jwtUtil;
        this.employeeMapper = employeeMapper;
        this.objectMapper = objectMapper;
    }
}
```

- `@Component`：让类在组件扫描涵盖该包的情况下被 Spring 注册为 Bean；**仅有此注解尚不足以令其参与某些请求的拦截**。
- `implements HandlerInterceptor`：表明它提供 Spring MVC 处理器拦截逻辑。
- `JwtUtil`：本项目自定义的 JWT 工具类；`parseToken()` 的校验行为由**它的实现**决定。
- `EmployeeMapper`：访问员工表的 Mapper；`selectById()` 具体是否存在、如何查询，取决于 Mapper 定义。
- `ObjectMapper`：Jackson 的 JSON 序列化/反序列化工具；这里负责把错误返回对象写到响应体。
- 构造器注入：由 Spring 提供依赖，三个字段用 `final` 限制以后重新赋值。`final` 本身不保证依赖不为 `null`。

### 6.2 `preHandle()` 的返回语义

```java
@Override
public boolean preHandle(
        HttpServletRequest request,
        HttpServletResponse response,
        Object handler) throws IOException {
    // ...
}
```

- `request`：读取 HTTP 请求方法、请求头等。
- `response`：设置 HTTP 状态码、响应类型、写响应体。
- `handler`：Spring MVC 选中的处理器，不一定是 Controller 方法。
- 返回 `true`：继续后面的拦截器以及处理器执行流程。
- 返回 `false`：阻止后续处理器执行；若负责拒绝请求，应自行准备相应的 HTTP 响应。

### 6.3 入口先清理旧上下文

```java
BaseContext.removeCurrentId();
```

避免线程池线程带着上个请求遗留的 ID 开始此次处理。它是**防御性清理**，不能取代请求完成时正常的清理。

### 6.4 特殊请求放行

```java
if ("OPTIONS".equalsIgnoreCase(request.getMethod())) {
    return true;
}

if (!(handler instanceof HandlerMethod)) {
    return true;
}
```

- `OPTIONS` 通常可能是浏览器跨域预检，但**并非所有 `OPTIONS` 都是预检**。这里的代码实际含义是“所有 OPTIONS 请求在此拦截器中放行”。CORS 仍需要独立、正确的服务端配置。
- `equalsIgnoreCase()`：忽略字符串大小写进行比较。
- `handler instanceof HandlerMethod`：判断这次映射目标是不是 Controller 处理方法；`!` 是否定运算符。
- 这两种放行并不是“整个系统不再处理请求”，只是**该拦截器允许继续**。

### 6.5 从请求头中取 token

```java
String token = request.getHeader("token");

if (token == null || token.isBlank()) {
    return reject(response, 401, "请先登录");
}
```

该项目**约定** HTTP 请求头名为 `token`，不是自动读取 `Authorization: Bearer ...`。前后端必须约定一致。`isBlank()` 是 Java 11+ 提供的字符串方法，能够判断空串或仅空白字符。

`reject()` 会设置响应并返回 `false`，所以当前代码不再执行后面的认证逻辑。

### 6.6 解析 JWT 与读取声明（Claims）

```java
Long employeeId;
String role;

try {
    Claims claims = jwtUtil.parseToken(token);

    if (claims.getExpiration() == null) {
        return reject(response, 401, "登录凭证无效");
    }

    employeeId = Long.valueOf(claims.getSubject());
    role = claims.get("role", String.class);

    if (employeeId <= 0 || role == null) {
        return reject(response, 401, "登录凭证无效");
    }

    if (!"admin".equals(role) && !"user".equals(role)) {
        return reject(response, 401, "登录凭证无效");
    }

} catch (JwtException | IllegalArgumentException exception) {
    return reject(response, 401, "登录凭证无效或已过期");
}
```

几个知识点：

1. **JWT 常见结构**：`header.payload.signature`。Payload 中包含 claims，但 JWT **签名不等于加密**，不应把明文 JWT Payload 视为秘密信息。
2. `getSubject()` 读取标准 `sub` 字段；`Long.valueOf(...)` 将表示数字的字符串转为 `Long`。非法数值会抛异常，走这里的 `catch`。
3. `get("role", String.class)` 读取应用自定义的 `role` claim，并要求用字符串形式返回。
4. `getExpiration()` 读取标准 `exp` 过期声明。这个代码显式拒绝缺少 `exp` 的情况，但**是否已经验证签名、过期和签发者**必须检查 `JwtUtil.parseToken()` 的代码。
5. JWT 标准中 `exp` 为 NumericDate 类型（数值形式、秒精度），不要把它理解为一定在 JSON 里存字符串日期。
6. `catch (A | B e)` 是 Java 的多重异常捕获，表示捕获其中任一指定类型。
7. `employeeId <= 0` 会把 `Long` 自动拆箱比较；此处根据 `Long.valueOf(...)` 的行为，不会正常返回一个 `null` 的 ID。
8. 先接受 `admin`/`user` 两类合法值，随后又禁止非 `admin` 访问该管理端接口。这两步分别对应“类型白名单检查”和“当前接口授权检查”，可以理解为不同目的，但对本拦截器而言也有简化空间。

**注意**：若 `parseToken()` 返回 `null`，这段代码访问 `claims.getExpiration()` 会产生 `NullPointerException`，当前 `catch` 未捕获它；正常解析器应该明确约定成功返回非空 Claims、失败抛出可识别异常，但**需要查看 `JwtUtil` 实现**。此外，任何不在 `JwtException`/`IllegalArgumentException` 范围的异常也不会被这段 `catch` 自动处理。

### 6.7 鉴权：只允许管理员

```java
if (!"admin".equals(role)) {
    return reject(response, 403, "无权访问运营接口");
}
```

使用常量放在左边的 `"admin".equals(role)`，可以在 `role == null` 时避免调用空引用的方法。因为前面已检查 `role == null`，这里也相当于一种稳妥写法。

- **401 Unauthorized**：通常表示尚未通过有效的身份认证、token 无效或已失效。
- **403 Forbidden**：通常表示身份已确认，但当前用户没有请求该资源的权限。

这里的状态码设计总体合理；具体 API 约定应与前端统一。

### 6.8 查数据库确认员工仍有效

```java
Employee employee = employeeMapper.selectById(employeeId);

if (employee == null) {
    return reject(response, 401, "登录账号不存在，请重新登录");
}

if (!Integer.valueOf(1).equals(employee.getStatus())) {
    return reject(response, 401, "账号已停用");
}
```

理由：JWT 即使仍处于有效期，也不能证明员工数据库记录**现在**仍然存在或处于启用状态。因此此处额外查询实时状态。

- `employee == null`：查询不到员工记录。
- `Integer.valueOf(1).equals(employee.getStatus())`：检查状态是否为 `1`。此写法在 `getStatus()` 返回可为 `null` 的 `Integer` 时也不会空指针。
- `1` 是否代表“正常”，取决于该项目自己的状态定义；本段代码假定它是启用状态。
- 每次查数据库增加开销，但带来更及时的停用控制；实际项目还需综合缓存、一致性和撤销策略评估。

### 6.9 通过后写入 ThreadLocal

```java
BaseContext.setCurrentId(employeeId);
return true;
```

**只在全部校验通过后**设置当前员工 ID，避免未认证请求留下合法用户上下文。此后同一个处理线程里的业务代码可调用：

```java
Long employeeId = BaseContext.getCurrentId();
```

但是如果执行流切换到别的线程（例如 `@Async`、某些 `CompletableFuture` 任务、异步 Servlet 处理），普通 `ThreadLocal` **不会自动同步到另一个线程**。

### 6.10 正常完成后的清理

```java
@Override
public void afterCompletion(
        HttpServletRequest request,
        HttpServletResponse response,
        Object handler,
        Exception exception) {
    BaseContext.removeCurrentId();
}
```

这就是对成功参与处理流程的拦截器做后置清理。**不是“无论发生什么一定调用”**：该拦截器的 `preHandle()` 返回 `false` 时，它自己的 `afterCompletion()` 不会被调用；异步处理也有特例，详见第 8 节。

### 6.11 `reject()` 统一返回错误

```java
private boolean reject(
        HttpServletResponse response,
        int status,
        String message) throws IOException {

    response.setStatus(status);
    response.setContentType("application/json;charset=UTF-8");

    objectMapper.writeValue(
            response.getWriter(),
            Result.error(message)
    );

    return false;
}
```

- `private`：只允许类内部调用，是一个辅助方法。
- `setStatus(status)`：设置 HTTP 状态码（如 401、403）。
- `setContentType(...)`：声明 JSON 格式及 UTF-8 字符集。
- `response.getWriter()`：获取用于写响应文本的输出流接口。
- `objectMapper.writeValue(writer, object)`：把 Java 对象序列化成 JSON 写入响应。
- `Result.error(message)`：**本项目自定义**的错误结果对象。不能仅凭本段代码认定它生成的字段是 `code`、`msg` 或其他固定结构。
- 返回 `false`：和 `preHandle()` 的返回约定配合，停止进入后续 Controller。

---

## 7. Spring MVC 请求执行链与拦截器注册

### 7.1 典型同步请求流程

```text
客户端发来请求
    │
    ▼
Servlet 容器 / 过滤器链（Filters）
    │
    ▼
Spring MVC DispatcherServlet
    │
    ▼
选择 Handler + 匹配的 Interceptor 链
    │
    ▼
JwtTokenAdminInterceptor.preHandle()
    │
    ├─ token/权限/状态不通过 ──► reject() 写响应，返回 false
    │
    └─ 全部通过 ──► BaseContext.setCurrentId(employeeId)
                           │
                           ▼
                       Controller
                           │
                           ▼
                         Service
                           │
                           ▼
                          Mapper
                           │
                           ▼
                        响应处理
                           │
                           ▼
                   afterCompletion()
                           │
                           ▼
                   BaseContext.removeCurrentId()
```

**线程范围限制**：这个图假定核心业务沿当前同步请求处理线程执行。切到其他线程时，`BaseContext` 的值不会自动跟着请求转移。

### 7.2 重要补充：`@Component` 之后仍需注册

例如，你的后台接口路径是 `/admin/**`，可以在配置类中注册：

```java
package com.sky.config;

import com.sky.interceptor.JwtTokenAdminInterceptor;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

@Configuration
public class WebMvcConfiguration implements WebMvcConfigurer {

    private final JwtTokenAdminInterceptor adminInterceptor;

    public WebMvcConfiguration(JwtTokenAdminInterceptor adminInterceptor) {
        this.adminInterceptor = adminInterceptor;
    }

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(adminInterceptor)
                .addPathPatterns("/admin/**")
                .excludePathPatterns("/admin/employee/login");
    }
}
```

这里只是**示范配置**，不是声称项目已有该配置。需要按你的真实 API 路径确认：

- **登录接口通常要排除认证拦截**，否则还没登录就无法调用登录接口。
- 非 `/admin/**` 路径未必被这个管理员拦截器覆盖；不能据此认为其他路径都已受保护。
- `@Component` 注册为 Bean，`addInterceptors()` 将它绑定到 MVC 请求执行链；两者各司其职。

Spring 官方文档还提醒：拦截器并不是构建完整安全边界的理想机制，因为请求路径与控制器映射规则可能产生不一致。更复杂、对安全要求较高的项目，应考虑 Spring Security 或与 Servlet Filter 链更紧密结合的认证方案。这不意味着教学项目中使用 MVC 拦截器就完全不可行，而是要认识其边界。

### 7.3 Spring 重要术语对照

| 术语 | 作用 |
|---|---|
| Spring Bean | 由 Spring 容器管理的对象。 |
| 依赖注入（DI） | 容器将需要的依赖对象提供给目标对象，构造器注入是常见方式。 |
| `HandlerInterceptor` | 在 Spring MVC 处理器调用前后插入逻辑。 |
| `HandlerMethod` | Spring MVC 中表示映射到控制器方法的处理器对象。 |
| Servlet Filter | 在 Servlet 过滤链中处理请求与响应，执行范围和时机与 MVC 拦截器不同。 |
| `ObjectMapper` | Jackson 的 Java 对象与 JSON 转换工具。 |
| JWT | 可携带身份声明的令牌格式；签名可以用于完整性和来源校验，但不意味着内容加密。 |
| `ThreadLocal` | 将数据关联到执行线程，而不是关联到整个 Web 会话或所有线程。 |

---

## 8. 边界条件、安全性与改进建议

### 8.1 `afterCompletion()` 的调用条件

Spring 的 `HandlerInterceptor` 文档明确指出，只有**当前拦截器的 `preHandle()` 成功完成并返回 `true`**，才会调用该拦截器的 `afterCompletion()`。

你的代码在 `preHandle()` **最开始先 `removeCurrentId()`**，并且在所有校验通过后才 `setCurrentId(employeeId)`：

- 正常拒绝请求 (`reject() → false`)：不会执行当前拦截器自己的 `afterCompletion()`，但此前没有为本请求写入新 ID。
- 正常放行、同步业务完成：`afterCompletion()` 清理 ID。
- 若引入新逻辑（例如设置 ID 后又直接返回 `false` 或抛异常），必须重新检查清理路径。

这比笼统地说“请求结束一定会 remove”更精确。

### 8.2 异步处理特别注意

普通 `ThreadLocal` 和线程绑定，不等于请求绑定。对于 Spring MVC 异步请求：

- 启动异步处理后，当前 Servlet 工作线程可能在 `afterCompletion()` 执行前就退出本轮处理。
- `AsyncHandlerInterceptor.afterConcurrentHandlingStarted()` 是清理原线程 ThreadLocal 的一个适用回调。
- 其他线程若需要员工身份，应设计显式的上下文传递与清理策略，不能假设换线程仍读得到同一个 `employeeId`。

这份原拦截器代码已说明“用于同步请求”，因此**适用于其明确限定的典型同步链路**；如果项目改为异步，就不能机械照搬。

### 8.3 JWT 验证边界（不能凭注释代替检查）

应当实际打开 `JwtUtil.parseToken()` 确认：

1. 使用可信的签名校验密钥 / 算法，拒绝签名不符和不符合要求的算法。
2. 处理过期时间，拒绝没有 `exp` 或已经过期的 token（当前调用者额外拒绝缺少 `exp`）。
3. 若业务约定了 `issuer`、`audience`，按策略核验相关声明。
4. 正确处理 `sub`、`role` 的缺失、类型不匹配与异常。
5. 明确刷新、注销和撤销 token 的策略。

目前仅能确认：**拦截器调用了 `jwtUtil.parseToken(token)` 并做了一些额外声明及账号状态检查**。不能独立证明 JWT 校验实现完整、无漏洞。

### 8.4 HTTP 与项目配置上的其他细节

- 该代码使用 `jakarta.servlet.*`，属于新一代 Jakarta Servlet API 的包名；较旧的 Java EE/Spring Boot 2 项目常用 `javax.servlet.*`。
- `request.getHeader("token")` 读取项目自定义的 header，必须与前端发送格式保持一致。
- `OPTIONS` 放行不等于跨域就会成功，仍需正确配置 CORS。
- `!(handler instanceof HandlerMethod)` 放行是当前的设计选择；若希望更强的安全边界，需要审查整个路由、静态资源及过滤器配置。
- 数据库查询失败、序列化响应失败等异常不一定被 `catch (JwtException | IllegalArgumentException)` 捕获，应由服务端统一异常处理和日志负责。
- `Result.error(message)` 的 JSON 结构、`employeeMapper.selectById()` 的 SQL、`JwtUtil` 是否是 Bean，均依赖本次对话**未提供的代码**，不能替它们作确定性判断。
- 不要直接相信客户端提交的员工 ID；应当以验证后的凭据及后端查询结果作为可信身份来源。

### 8.5 可以考虑的改进方向（不属于原代码的既有事实）

1. **先确认拦截器注册路径**，特别是放行登录和无需登录的接口。
2. **完善真实 JWT 解析器的单元测试**：签名无效、过期、缺少 `exp`、错误 `sub`、错误 `role`。
3. **为管理员账号状态写测试**：不存在、禁用、正常。
4. **增加上下文隔离测试**：两个线程同时请求读取不同 ID，且线程复用时不残留旧值。
5. 如果将来使用**异步**或完整企业级权限体系，调整上下文清理和认证架构，不要只把简单同步拦截器当成完整的安全框架。

---

## 9. 自测题及参考答案

建议先独立回答，再展开下方答案。

<details>
<summary>Q1. `private static final ThreadLocal<Long> CURRENT_ID` 中三个修饰符分别是什么作用？</summary>

`private`：只允许类内部访问字段；`static`：字段属于类；`final`：字段引用不能被重新赋值。`set()` 仍可修改当前线程关联的值。

</details>

<details>
<summary>Q2. `final int x = 10` 与 `final List<Integer> x = new ArrayList<>()` 的行为有什么区别？</summary>

前者变量不能被改赋为其他整数；后者引用不能重新指向其他 List，但仍可通过 `add()` 等方法修改可变 List 对象。

</details>

<details>
<summary>Q3. 为什么不把 ThreadLocal 换成 `private static Long currentId`？</summary>

多个线程可能读写同一个静态 `Long` 字段，造成相互覆盖。`ThreadLocal` 让不同线程的 `set/get` 访问各自关联的值。

</details>

<details>
<summary>Q4. ThreadLocal 的 `get()` 没有先 `set()` 会返回什么？</summary>

`new ThreadLocal<>()` 在当前线程未设置过时默认返回 `null`；若显式使用自定义初始值机制，结果可以不同。

</details>

<details>
<summary>Q5. 为什么不使用 Lombok 的 `@Data` 自动生成 BaseContext 的业务方法？</summary>

`@Data` 完全跳过 `static` 字段，不会自动生成对应 Getter/Setter；而且业务需要的是 `Long` 的读取/写入/清理，不是直接暴露 `ThreadLocal` 对象。

</details>

<details>
<summary>Q6. `ThreadLocal<Long>.set(longValue)` 为什么可以编译？</summary>

Java 自动装箱：`long` 会在调用需要 `Long` 参数的方法时转换为 `Long`。

</details>

<details>
<summary>Q7. `preHandle()` 返回 true / false 分别代表什么？</summary>

`true`：继续执行 MVC 处理链；`false`：当前请求在此处理链上停止往下走，通常要自行写入拒绝响应。

</details>

<details>
<summary>Q8. `@Component` 是否足够让拦截器自动拦截 `/admin/**`？</summary>

通常不够，还需要用 `WebMvcConfigurer#addInterceptors()` 等机制注册并限定作用路径。

</details>

<details>
<summary>Q9. 为什么验证了 JWT 之后还要查数据库？</summary>

JWT 里的信息可能是签发时的快照。员工被停用、删除后，若只检查 token 可能继续允许访问；数据库可校验当下的账号状态，但引入额外查询开销。

</details>

<details>
<summary>Q10. `afterCompletion()` 是否在 `preHandle()` 返回 false 后依然执行？</summary>

不会对**当前这个拦截器**执行其 `afterCompletion()`。它要求该拦截器自身的 `preHandle()` 成功返回 `true`；异步处理又有额外回调规则。

</details>

<details>
<summary>Q11. 为什么 `"admin".equals(role)` 通常比 `role.equals("admin")` 稳妥？</summary>

当 `role` 可能为 `null` 时，前者返回 `false`，后者会抛出 `NullPointerException`。

</details>

<details>
<summary>Q12. 在 `@Async` 方法中调用 `BaseContext.getCurrentId()` 一定能获取拦截器存的值吗？</summary>

不能。普通 `ThreadLocal` 的值跟随线程，不会在切换到其他执行线程时自动复制。应根据实际异步模型设计身份上下文传递方式。

</details>

---

## 10. 官方资料

- [Oracle Java ThreadLocal API（Java 24，英文）](https://docs.oracle.com/en/java/javase/24/docs/api/java.base/java/lang/ThreadLocal.html)：`get/set/remove` 的精确定义。
- [Project Lombok：`@Data`](https://projectlombok.org/features/Data)：说明 `static` 字段被跳过。
- [Project Lombok：`@Getter/@Setter`](https://projectlombok.org/features/GetterSetter)：类级别注解针对非静态字段。
- [Spring Framework：MVC Interceptors](https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-config/interceptors.html)：如何注册拦截器，以及安全使用上的注意事项。
- [Spring Framework：HandlerInterceptor 源码/Javadoc](https://github.com/spring-projects/spring-framework/blob/main/spring-webmvc/src/main/java/org/springframework/web/servlet/HandlerInterceptor.java)：`preHandle`、`afterCompletion` 的执行约定。
- [Spring Framework：AsyncHandlerInterceptor 源码/Javadoc](https://github.com/spring-projects/spring-framework/blob/main/spring-webmvc/src/main/java/org/springframework/web/servlet/AsyncHandlerInterceptor.java)：异步处理与 ThreadLocal 清理。
- [RFC 7519：JSON Web Token (JWT)](https://www.rfc-editor.org/rfc/rfc7519)：JWT 声明（如 `sub`、`exp`）的规范。

---

## 一句话串联全部知识

> `JwtTokenAdminInterceptor` 在同步 HTTP 请求进入 Controller 前读取并校验 token，确认用户具有管理员权限且账号仍有效后，将 `employeeId` 存入 `BaseContext` 的 `ThreadLocal`；同一处理线程的业务代码用 `getCurrentId()` 读取 ID；流程完成时移除线程局部值，避免线程复用产生旧身份信息。`BaseContext` 是 **Java 静态工具类 + 线程局部存储** 的封装，而不是依靠 `@Data` 自动生成 Getter/Setter 的数据对象。
