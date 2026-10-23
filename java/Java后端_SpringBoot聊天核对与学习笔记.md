# Java 后端基础与 Spring Boot：聊天核对版学习笔记

> 适用范围：Java 17/21、Spring Boot 3.x、Spring MVC（Servlet 栈）、Jackson 2、Jakarta Validation、MyBatis-Plus、Lombok。
>
> 资料性质：根据本次聊天中讨论的知识点整理；对表述不准确处已标注修正。不同 Spring Boot/Jackson 版本可能有细微差异，项目实际配置优先。
>
> 阅读方法：先看「0. 错误核对」，再依次学习 Java 语法、Spring IoC、HTTP/MVC、异常与安全，最后用完整请求链路复习。

## 目录

- [0. 聊天正确性核对：需要修正或补充的地方](#0-聊天正确性核对需要修正或补充的地方)
- [1. 接口、实现类、向上/向下转型](#1-接口实现类向上向下转型)
- [2. 对象引用、组合、构造器、final](#2-对象引用组合构造器final)
- [3. Lambda、函数式接口、方法引用、回调](#3-lambda函数式接口方法引用回调)
- [4. Stream 与 Entity → VO](#4-stream-与-entity--vo)
- [5. 异常体系、throw / throws、异常传播](#5-异常体系throw--throws异常传播)
- [6. 字符串、编码、BigDecimal、日期与随机标识](#6-字符串编码bigdecimal日期与随机标识)
- [7. URL、HTTP 方法、请求组成](#7-urlhttp-方法请求组成)
- [8. JSON 与 Jackson](#8-json-与-jackson)
- [9. Spring 容器、Bean、扫描与依赖注入](#9-spring-容器bean扫描与依赖注入)
- [10. Spring MVC 映射与参数绑定](#10-spring-mvc-映射与参数绑定)
- [11. DTO / Entity / VO 与校验](#11-dto--entity--vo-与校验)
- [12. 异常处理、Filter、Interceptor 与 JWT](#12-异常处理filterinterceptor-与-jwt)
- [13. 菜品更新接口逐句解析](#13-菜品更新接口逐句解析)
- [14. MyBatis-Plus LambdaQueryWrapper 查询](#14-mybatis-plus-lambdaquerywrapper-查询)
- [15. 一次请求的实际执行顺序](#15-一次请求的实际执行顺序)
- [16. 练习题与复习清单](#16-练习题与复习清单)
- [参考资料](#参考资料)

---

## 0. 聊天正确性核对：需要修正或补充的地方

**总体结论：**绝大多数核心概念正确，但有些「方便入门的简化」不能直接当作严格定义。

| 聊天中的说法 | 核对结论与更准确的说法 |
|---|---|
| “组合就是一个对象持有另一个对象” | 适合作为入门类比，但严格 UML 中，成员引用通常只能说明**关联 / has-a**；**组合（composition）**强调强整体—部分关系以及生命周期归属。Spring Controller 持有单例 Service 通常不应按严格 UML 组合解释。 |
| “`final` 字段要在构造器执行完成前初始化” | 对**空白 `final` 实例字段**基本正确；还可以在字段声明或实例初始化块中赋值。每条构造路径必须满足确定赋值规则。`final` 引用不允许再次指向新对象，但对象内部可变。 |
| “`EmployeeService` 和 `EmployeeServiceImpl` 是两个对象” | 错误。前者是**接口类型**，后者是**实现类类型**；注入场景中一般是接口类型引用指向具体 Bean/代理。 |
| “Spring 一定 `new EmployeeServiceImpl()`” | 仅为类比。Spring 创建 Bean 可能涉及反射、工厂方法、代理等；不能把框架内部机制等同于显式的 `new`。 |
| “`@Bean` 必须与 `@Configuration` 同时使用” | 不必须。`@Bean` 也可出现在其他合适的 Spring 组件类中；但 `@Configuration + @Bean` 是常见、易于理解的配置形式，且涉及配置方法间调用时语义有区别。 |
| “只有一个匹配 Bean 才能注入” | 只有一个是最简单情况；多个候选 Bean 时，还可以用 `@Qualifier`、`@Primary`、某些情况下的名称匹配等进行消歧。 |
| “`(a, b) -> a - b` 是推荐的整数比较器” | **有整数溢出风险**。推荐 `Integer::compare`、`Comparator.naturalOrder()` 或 `Comparator.reverseOrder()`。 |
| “`String.length()` 就是字符数” | 精确说是 **UTF-16 代码单元数**，不是 Unicode 码点数、用户感知字符数，也不是 UTF-8 字节数。 |
| “`UUID.randomUUID()` 不具备安全随机性” | 这句不准确：**JDK 21 的 `UUID.randomUUID()` 明确使用密码学强随机数生成器**生成 v4 UUID。只是 UUID 的用途和格式仍以唯一标识为主；安全 Token 可以直接用 `SecureRandom` 生成足够熵的随机字节，更容易控制长度。 |
| “`@Valid` 失败都抛 `MethodArgumentNotValidException`” | 在常见 `@Valid @RequestBody DTO` 参数校验中是这样；如果还涉及方法级约束，Spring MVC 可能抛 `HandlerMethodValidationException`。 |
| “未知 JSON 字段一定不会出错” | 在 Spring Boot 3 + 常见 Jackson 自动配置中默认通常忽略未知字段，但**取决于 ObjectMapper 配置**；严格模式可拒绝。 |
| “Spring MVC 只在 Controller 返回后才做 JSON 序列化” | 对于 `@ResponseBody`/`@RestController`，响应体在 MVC HandlerAdapter 处理返回值时就可能写出；常规 `postHandle()` 往往已经太晚，不能修改它。 |
| “JWT 每次都需要在 Interceptor 校验” | 更准确：**每个需要 JWT 认证的请求**通常要校验所携凭据；公开路径可跳过。正式项目通常优先通过 Spring Security 的 **Filter Chain** 实现安全机制，不应只靠 MVC Interceptor。 |
| “JWT 有效就有权操作该资源” | 错误。**认证 Authentication** 只确认身份；**授权 Authorization** 仍需校验是否拥有该菜品、该订单等资源。 |
| “DTO 固定属于 Controller 层” | DTO 是**边界数据传输模型**，常用于 Controller ↔ Service；不是 Java/Spring 强制的层级归属。数据库持久化对象可以叫 Entity、DO、PO，命名随架构而变。 |
| “`throw`/`throws` 都让异常开始传播” | `throw` 是主动抛出异常；`throws` 是方法声明，主要用于声明受检异常，不会制造异常。运行时异常即使不写这两个关键字，也可由调用的代码抛出并沿栈传播。 |
| “POST 就是更新” | 不准确：POST 常用于创建/提交/触发处理；PUT 通常用于整体替换或更新，PATCH 用于部分更新。HTTP 动词语义与业务设计有关。 |
| “`selectCount` 检查不存在后就一定不会重复” | 不成立。并发请求存在竞态，**数据库唯一约束**才是防止重复用户名的关键机制，应用层预检查只是改善体验。 |

**使用本笔记时注意**：聊天中的不少 Java 片段省略 `getter/setter`、构造器、`Mapper` 字段、`import`、返回值等，属于“教学片段”，不要直接复制进完整项目后就期待能编译运行。

---

## 1. 接口、实现类、向上/向下转型

### 1.1 接口定义“做什么”，实现类定义“怎么做”

```java
public interface EmployeeService {
    Employee getById(Long id);
    void deleteById(Long id);
}

@Service
public class EmployeeServiceImpl implements EmployeeService {
    private final EmployeeMapper employeeMapper;

    public EmployeeServiceImpl(EmployeeMapper employeeMapper) {
        this.employeeMapper = employeeMapper;
    }

    @Override
    public Employee getById(Long id) {
        return employeeMapper.selectById(id);
    }

    @Override
    public void deleteById(Long id) {
        employeeMapper.deleteById(id);
    }
}
```

- `interface` 声明接口；`implements` 表示类实现接口。
- `@Override` 表示实现/重写约定的方法。
- 接口可以包含 `default`、`static` 等方法，不是所有接口方法都一定没有实现。
- `@Service` 使符合扫描条件的类作为 Spring 组件注册。
- Controller **不必须**依赖接口：直接依赖具体类也可工作；优先依赖接口是为了降低耦合、方便替换实现和测试。

### 1.2 多态、向上转型与向下转型

```java
EmployeeService service = new EmployeeServiceImpl(mapper); // 向上转型
EmployeeServiceImpl impl = (EmployeeServiceImpl) service;  // 向下转型
```

- **向上转型**：实现类引用可赋值给接口类型，通常隐式、安全。
- **向下转型**：接口类型转为具体实现类引用，需要兼容的运行时对象，否则 `ClassCastException`。
- 可以先用 `instanceof` 检查：

```java
if (service instanceof EmployeeServiceImpl impl) {
    // 这里可访问 EmployeeServiceImpl 自有的方法
}
```

- 转型**不创建对象**；改变的是引用可见的类型和操作范围。
- Spring 注入的对象有时是代理，**不要随意把接口引用强转为实现类**。

---

## 2. 对象引用、组合、构造器、final

### 2.1 声明引用 ≠ 创建对象

```java
EmployeeService service;                // 声明局部引用变量，尚未赋值
service = new EmployeeServiceImpl(mapper); // 创建对象并赋给引用
```

**特别注意**：Java 局部变量在赋值前不可读取；对象字段若不是 `final`，会得到相应默认值（引用默认 `null`）。

### 2.2 Controller 为什么持有 Service 引用

```java
public class EmployeeController {
    private final EmployeeService employeeService;

    public EmployeeController(EmployeeService employeeService) {
        this.employeeService = employeeService;
    }

    public Employee getEmployee(Long id) {
        return employeeService.getById(id);
    }
}
```

Controller 不负责员工业务细节，但仍须**调用** Service，所以通过字段保存依赖引用。

- **职责分离**：Controller 接收请求、解析参数、返回响应；Service 执行业务；Mapper 访问数据库。
- **依赖注入**：依赖对象由外部提供，而非 Controller 内部直接写死实现类。
- **普通 Java 也能注入**：

```java
EmployeeService service = new EmployeeServiceImpl(mapper);
EmployeeController controller = new EmployeeController(service);
```

`new` 仍发生，只是发生在 Controller 外部。在 Spring 场景，容器负责装配。

### 2.3 `final` 实例字段

```java
class Demo {
    private final int x;

    Demo(int value) {
        x = value;
    }
}
```

其他合法方式：`private final int x = 10;`，或在实例初始化块中赋值。空白 `final` 必须在每条构造路径中恰当初始化。

```java
private final List<String> names = new ArrayList<>();
names.add("Tom");  // 合法：修改对象内容
// names = new ArrayList<>(); // 非法：给 final 引用重新赋值
```

**术语修正**：Controller 持有 Service 字段，足以说明它们存在“关联/持有”的关系；严格的设计建模里，并不意味着二者一定构成生命周期绑定的“组合”。

---

## 3. Lambda、函数式接口、方法引用、回调

### 3.1 Lambda 是什么

Lambda 把**行为**作为函数式接口实例传递：

```java
(a, b) -> a + b
x -> System.out.println(x)
() -> System.out.println("完成")
```

目标类型必须是**函数式接口**：只有一个抽象方法（不计 `default`、`static`，以及按规则不计的 `Object` 公有方法）。

```java
@FunctionalInterface
interface Callback {
    void onComplete();

    default void hello() {
        System.out.println("hello");
    }
}
```

`@FunctionalInterface` 不是必需的，但能让编译器检查接口设计。

### 3.2 Lambda 与匿名内部类

```java
Runnable a = new Runnable() {
    @Override
    public void run() {
        System.out.println("run");
    }
};

Runnable b = () -> System.out.println("run");
```

两者在这个场景行为相同，但底层机制、`this` 绑定等并不完全等价；Lambda 不是可以无条件替代所有匿名内部类。

### 3.3 方法引用 `::`

```java
names.forEach(System.out::println);         // 对象::实例方法
Function<String, Integer> f = String::length; // 类::实例方法（接收者作为参数）
Function<String, Integer> g = Integer::parseInt; // 类::静态方法
Supplier<Employee> h = Employee::new;        // 构造器引用（要求可用的无参构造器）
```

方法引用并不当场执行方法。它产生符合函数式接口签名的行为实现，待使用时调用。

### 3.4 四个常用函数式接口

| 接口 | 抽象方法 | 输入 → 输出 | 适用场景 |
|---|---|---|---|
| `Predicate<T>` | `test(T)` | T → boolean | 过滤、条件判断 |
| `Function<T,R>` | `apply(T)` | T → R | 字段映射、转换 |
| `Consumer<T>` | `accept(T)` | T → void | 打印、消费、回调动作 |
| `Supplier<T>` | `get()` | 无 → T | 延迟提供对象/值 |

### 3.5 接口回调

```java
interface Callback {
    void onComplete();
}

class Task {
    void execute(Callback callback) {
        System.out.println("正在执行任务");
        callback.onComplete();
    }
}

new Task().execute(() -> System.out.println("任务完成"));
```

- `execute` 参数类型是 `Callback`；Lambda 为该函数式接口的抽象方法提供实现。
- **回调的核心**：调用方提供执行逻辑，被调用方在约定时机反过来调用它。
- 回调**不一定异步**，上例是同步回调。

---

## 4. Stream 与 Entity → VO

给定：

```java
List<Integer> numbers = List.of(5, 2, 8, 1, 4);
```

| 方法 | 作用 | 示例 |
|---|---|---|
| `forEach` | 对元素执行动作；终止操作 | `numbers.forEach(System.out::println);` |
| `filter` | 依据 Predicate 过滤 | `.filter(x -> x > 3)` |
| `map` | 依据 Function 转换元素 | `.map(x -> x * 2)` |
| `sorted` | 产生有序流 | `.sorted()` |
| `toList()` | 收集为 List（Java 16+） | `.toList()` |

```java
List<Integer> result = numbers.stream()
    .filter(x -> x > 3) // [5, 8, 4]
    .map(x -> x * 2)     // [10, 16, 8]
    .sorted()            // [8, 10, 16]
    .toList();
```

`filter/map/sorted` 为**中间操作**，具有惰性；通常等到 `toList/forEach/count` 等终止操作触发执行。

整数比较避免 `a - b` 溢出：

```java
numbers.stream().sorted(Comparator.reverseOrder()).toList();
```

**Employee → EmployeeVO：**

```java
List<EmployeeVO> vos = employees.stream()
    .map(e -> new EmployeeVO(e.getId(), e.getName()))
    .toList();
```

前提：`EmployeeVO` 存在匹配的构造器。也可以定义 `EmployeeVO convert(Employee employee)`，然后 `.map(this::convert)`。

> `Stream.toList()` 返回不可修改的 List；如果后续要 `add/remove`，可使用 `collect(Collectors.toCollection(ArrayList::new))`。

---

## 5. 异常体系、throw / throws、异常传播

### 5.1 异常分类

```text
Throwable
├── Error                     （严重错误，通常不主动捕获处理）
└── Exception
    ├── RuntimeException      （非受检异常）
    │   ├── NullPointerException
    │   └── IllegalArgumentException
    ├── IOException           （受检异常）
    └── SQLException          （受检异常）
```

- **受检异常**：编译器要求捕获或声明，例如 `IOException`。
- **运行时异常**：编译器不强制捕获/声明；不代表它不重要。
- 严格讲，`Error` 也属于非受检的 `Throwable`，但一般不称为 RuntimeException。

### 5.2 `throw` vs `throws`

```java
public void read() throws IOException {       // 声明可能传播 IOException
    throw new IOException("文件读取失败");     // 主动抛出异常对象
}
```

| 语法 | 位置 | 实际作用 |
|---|---|---|
| `throw` | 方法体 | 真正抛出某个 `Throwable` 对象 |
| `throws` | 方法声明 | 声明可能抛出的异常类型；对受检异常尤其重要 |

**无需主动 `throw` 也可能产生异常**：

```java
public void service() {
    mapper.query();   // mapper 内部发生运行时异常时，service 没 catch 则继续传播
}
```

如果下层抛的是受检异常，而本层又不捕获，就要相应地在方法上 `throws`（或按规则处理）。

### 5.3 异常传播链

```text
数据库发生故障
  ↓
Mapper 抛异常
  ↓（未捕获）
Service
  ↓（未捕获）
Controller
  ↓
Spring MVC 异常解析与处理
  ↓
HTTP 错误响应
```

**传播**是沿着调用栈找处理者，不是“必须层层显式写 `throws`”。运行时异常可自动沿栈传递。

### 5.4 什么时候捕获

1. **当前层可以恢复**：例如读配置失败后用默认配置。
2. **需要转换异常语义**：例如将数据库唯一键冲突转换为“用户名已存在”的业务错误，同时保留原始异常 cause。
3. **需要清理外部资源**：优先 `try-with-resources` 或 `finally`，而非单纯 catch。
4. **当前层无法处理**：不要捕获后仅 `printStackTrace()` 就吞掉；让它向上交给统一异常处理器。

### 5.5 业务错误 vs 系统故障

| 类型 | 例子 | 常见响应策略 |
|---|---|---|
| 业务错误 | 余额不足、库存不足、用户名冲突 | 4xx 或定义好的业务错误码及明确消息 |
| 系统故障 | 数据库连接失败、NPE、网络服务不可用 | 5xx、服务器记录日志、必要时告警 |

并非所有业务错误都用 HTTP 400，也不是所有系统故障都只能是 500；要按 API 语义选择 400、401、403、404、409、500、503 等状态。

---

## 6. 字符串、编码、BigDecimal、日期与随机标识

### 6.1 `String` 常用方法

```java
String s = "  hello  ";
s.trim();                          // "hello"：移除首尾部分空白
s.strip();                         // Java 11+，更符合 Unicode 空白处理
s.isEmpty();                       // 是否长度为 0
s.isBlank();                       // 是否空串或全空白，Java 11+
s.length();                        // UTF-16 代码单元数
"12345".matches("\\d+");          // true：正则完全匹配
```

**判空注意顺序**：

```java
boolean blank = s == null || s.isBlank(); // 避免对 null 调用方法
```

### 6.2 UTF-16 长度与 UTF-8 字节数

```java
String chinese = "你好";
chinese.length(); // 2：UTF-16 code units
chinese.getBytes(StandardCharsets.UTF_8).length; // 6

String emoji = "😀";
emoji.length(); // 2：补充平面码点使用 surrogate pair
emoji.codePointCount(0, emoji.length()); // 1 个 Unicode 码点
emoji.getBytes(StandardCharsets.UTF_8).length; // 4 字节
```

> 甚至码点数仍未必等于“用户眼中的字符数”，因为某些 Emoji 是多个码点组合而成。

### 6.3 `BigDecimal`

```java
BigDecimal a = new BigDecimal("10.50");
BigDecimal b = new BigDecimal("2.00");

a.add(b);                                    // 加
 a.subtract(b);                              // 减
 a.multiply(b);                              // 乘
 a.divide(b, 2, RoundingMode.HALF_UP);       // 除，保留两位小数
 a.compareTo(b);                             // 比大小
 a.setScale(2, RoundingMode.HALF_UP);         // 舍入
```

- 推荐从**字符串**构造，或 `BigDecimal.valueOf(0.1)`；避免 `new BigDecimal(0.1)` 带入浮点误差。
- `BigDecimal` 不可变，计算结果需要接住。
- 比较数值大小用 `compareTo`；`equals` 还会考虑 scale：`1.0` 与 `1.00` 的 `equals` 为 false。
- 除法有可能得到无限小数，未指定舍入策略时可能抛 `ArithmeticException`。
- 金额舍入方式应按**业务/财务规则**决定，不能所有业务都固定选 `HALF_UP`。

### 6.4 时间类型

| 类型 | 包含什么 | 典型用途 |
|---|---|---|
| `LocalDate` | 年月日 | 生日、日历日期 |
| `LocalDateTime` | 年月日时分秒、**无时区/偏移** | 当地日程、无时区日期时间 |
| `Instant` | 时间轴上的绝对时刻 | 日志、事件时间、时间戳 |
| `OffsetDateTime` | 日期时间 + UTC 偏移 | 带 `+08:00` 等偏移的时间 |
| `ZonedDateTime` | 日期时间 + 地区时区规则 | 涉及夏令时、未来跨时区预约 |

```java
LocalDate date = LocalDate.of(2026, 10, 23);
LocalDateTime local = LocalDateTime.of(2026, 10, 23, 10, 30);
Instant moment = Instant.now();
OffsetDateTime offset = OffsetDateTime.now(ZoneOffset.ofHours(8));
ZonedDateTime shanghai = moment.atZone(ZoneId.of("Asia/Shanghai"));
```

- `+08:00` 是 **UTC 偏移**，`Asia/Shanghai` 是**地区时区标识**；在有夏令时地区，偏移随日期可能变化。
- 时间戳通常从 `1970-01-01T00:00:00Z` 起算，要确认单位：`getEpochSecond()` 是秒；`toEpochMilli()` 是毫秒。
- `LocalDateTime` 不带时区，不要未经明确时区约定就转换成绝对时间。

### 6.5 日期格式

```java
DateTimeFormatter fmt = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
String text = local.format(fmt);
LocalDateTime parsed = LocalDateTime.parse("2026-10-23 10:30:00", fmt);
```

格式重点：`MM` 是月份；`mm` 是分钟；`HH` 是 24 小时制小时。

### 6.6 UUID / 安全随机标识

```java
String id = UUID.randomUUID().toString(); // 随机 v4 UUID（常用于资源 ID）

SecureRandom sr = new SecureRandom();
byte[] tokenBytes = new byte[32]; // 256 bit
sr.nextBytes(tokenBytes);
String token = Base64.getUrlEncoder().withoutPadding().encodeToString(tokenBytes);
```

- `UUID.randomUUID()` 在 JDK 21 使用密码学强随机生成器，适合很多随机唯一标识场景，**不保证绝对唯一**。
- 安全敏感的重置 Token、会话 Token 可采用 `SecureRandom` 的足够长度字节，再 Base64URL 编码。Token 还需要过期、存储/校验、撤销和防泄漏设计。
- **JWT** 是带签名声明的 Token 格式，不等同于上例随机不透明 Token。

---

## 7. URL、HTTP 方法、请求组成

### 7.1 URL 结构

```text
https://api.example.com:8443/employees/1001?detail=true
  └协议┘ └────域名────┘ └端口┘ └───路径────┘ └─查询参数─┘
```

- **协议**：常见 `http` / `https`。
- **域名**：通过 DNS 解析，得到相应 IP 地址；实际网络通信还有 CDN、反向代理等情况。
- **端口**：服务器连接目标服务的端口。HTTP 默认 80、HTTPS 默认 443；`8080` 常用于开发。
- **路径**：定位资源/接口。
- **Query**：`?key=value&key2=value2`，常传分页、搜索条件；需正确 URL 编码。

### 7.2 HTTP 方法

| 方法 | 常见语义 |
|---|---|
| `GET` | 查询资源，原则上不应修改服务器状态 |
| `POST` | 创建、提交或触发处理 |
| `PUT` | 更新或整体替换指定资源；协议语义要求幂等 |
| `PATCH` | 部分修改资源，不自动保证幂等 |
| `DELETE` | 删除资源；协议语义要求幂等 |

**幂等**：同一请求执行一次和重复多次，对目标状态的预期效果一致；不意味着每次 HTTP 响应完全相同。

### 7.3 Path、Query、Header、Body

```http
PUT /employees/1001?notify=true HTTP/1.1
Host: api.example.com
Authorization: Bearer <token>
Content-Type: application/json

{"name":"Tom","age":20}
```

| 位置 | 示例 | Spring MVC 注解 |
|---|---|---|
| Path | `/employees/1001` 中的 `1001` | `@PathVariable` |
| Query | `notify=true` | `@RequestParam` |
| Header | `Authorization` | `@RequestHeader` |
| Body | JSON 文本 | `@RequestBody` |

`@RequestParam` 不只处理 URL query，还可处理 Servlet 的表单请求参数；以上是最常见入门用法。

---

## 8. JSON 与 Jackson

### 8.1 JSON 的六类值

```json
{
  "id": 1001,
  "name": "Tom",
  "skills": ["Java", "SQL"],
  "active": true,
  "email": null,
  "department": { "name": "研发部" }
}
```

- 对象 `{}`：字符串键名对应值。
- 数组 `[]`：有序元素，元素类型可以不同。
- 字符串：**双引号**，可以含转义字符。
- 数字：不带引号；JSON 本身不区分 `int` 与 `double` 的 Java 类型。
- 布尔：仅小写 `true` / `false`。
- 空值：小写 `null`；和字符串 `"null"` 不同。
- 标准 JSON 不允许注释或末尾多余逗号；根值并不必须是对象，也可以是数组或标量。

### 8.2 请求反序列化、响应序列化

```text
请求 JSON（Body）
  → Tomcat 接收 HTTP
  → Spring MVC 的 @RequestBody 参数解析
  → HttpMessageConverter（例如 Jackson 转换器）
  → DTO
  → Controller

Controller 返回 Java VO
  → HttpMessageConverter / Jackson
  → JSON 响应 Body
```

- **反序列化**：JSON → Java 对象。
- **序列化**：Java 对象 → JSON。
- `@RequestBody` 指示从请求体读取数据；`@RestController` 使方法返回值默认作为响应体输出。

### 8.3 Jackson 常用注解与依赖

```java
@JsonProperty("employee_name")  // 改变 JSON 字段名
private String name;

@JsonIgnore                   // 忽略该属性的 JSON 映射
private String password;

@JsonFormat(pattern = "yyyy-MM-dd HH:mm:ss")
private LocalDateTime createTime;
```

- `jackson-annotations` 提供注解；`jackson-core` 实现基础 JSON 流处理；`jackson-databind` 处理 Java 对象数据绑定。
- **单独引入注解模块不能完成对象转换**。
- Spring Boot 3.x 的常见 `spring-boot-starter-web` 配置已经引入 JSON/Jackson 相关能力，通常无需单独加 `jackson-annotations`。
- Spring Boot 3.x 通常自动配置 `ObjectMapper`，不需要为了演示 `@Bean` 而另注册一份。若自己注册，应了解对自动配置的影响。
- 登录接口一般**会**反序列化 `LoginDTO`、序列化 `LoginVO`；JWT 签名/验证是另外的机制。

---

## 9. Spring 容器、Bean、扫描与依赖注入

### 9.1 容器是什么

Spring IoC 容器可以看成**管理 Bean 的运行时注册与装配机制**，负责创建 Bean、解析依赖、初始化/销毁管理等。

**Bean**：由 Spring 容器管理的对象（可能涉及代理）。

### 9.2 常见注解

| 注解 | 标注位置 | 功能 |
|---|---|---|
| `@Component` | 类 | 普通可扫描组件 |
| `@Service` | 类 | 语义上属于业务层的组件；派生自 `@Component` |
| `@Repository` | 类 | 数据访问组件；可能参与异常转换等机制 |
| `@RestController` | 类 | REST 控制器，也是 Spring 组件 |
| `@Configuration` | 类 | 配置类，也能作为组件管理 |
| `@Bean` | 方法 | 将方法返回对象注册为 Bean |

**两种注册方式**：

```java
@Service
class EmployeeServiceImpl implements EmployeeService { /* ... */ }
```

```java
@Configuration
class AppConfig {
    @Bean
    SomeClient someClient() {
        return new SomeClient();
    }
}
```

如果同一种类型的 Bean 使用多种方式重复注册，依赖注入时可能歧义。第三方对象常通过 `@Bean` 统一配置。

### 9.3 组件扫描

```java
@SpringBootApplication
public class Application {
    public static void main(String[] args) {
        SpringApplication.run(Application.class, args);
    }
}
```

Spring Boot 默认从启动类所在包及其子包扫描相关组件（可以显式调整扫描范围）。

```text
com.example
├── Application
├── controller/DishController
└── service/DishServiceImpl
```

启动类最好放在项目的根包，避免某些组件扫描不到。

### 9.4 构造器注入、Lombok

```java
@RestController
@RequiredArgsConstructor
class DishController {
    private final DishService dishService;
}
```

`@RequiredArgsConstructor` 是 **Lombok** 注解，给未初始化的 `final` 字段及符合条件的 `@NonNull` 字段生成构造器，类似：

```java
public DishController(DishService dishService) {
    this.dishService = dishService;
}
```

当 Spring Bean 只有一个构造器时，通常无需另写 `@Autowired`。这属于**构造器依赖注入**。

- `@Component`/`@Service` 注册并不等于马上给每个类注入对象；容器还会解析依赖，选择候选 Bean。
- 多候选时可使用 `@Qualifier("beanName")` 或 `@Primary` 等消除歧义。
- 依赖接口便于更换实现，但不是 Spring 的强制规定。
- Spring 容器默认作用域通常是 singleton，Controller 可以与其他组件共享同一 Service Bean。

---

## 10. Spring MVC 映射与参数绑定

### 10.1 `@RestController` / `@RequestMapping`

```java
@RestController
@RequestMapping("/employees")
public class EmployeeController {

    @GetMapping("/{id}")
    public EmployeeVO get(@PathVariable Long id) { /* ... */ }

    @PostMapping
    public EmployeeVO create(@Valid @RequestBody CreateEmployeeDTO dto) { /* ... */ }
}
```

- `@RestController` 可理解为 `@Controller + @ResponseBody`。
- `@RequestMapping` 可以匹配路径、HTTP 方法、Header、参数、请求或响应媒体类型等；写在类上通常定义共有前缀。
- `@GetMapping / @PostMapping / @PutMapping / @PatchMapping / @DeleteMapping` 是限定特定 HTTP 方法的派生映射注解。
- `GET /employees/1001` 会匹配 `@GetMapping("/{id}")` 并把 `id` 绑定为 `Long`。

### 10.2 三个最常用的参数注解

```java
@GetMapping("/{id}")
public EmployeeVO get(@PathVariable Long id,
                      @RequestParam(defaultValue = "false") boolean detail) {
    /* ... */
}

@PostMapping
public EmployeeVO create(@Valid @RequestBody CreateEmployeeDTO dto) {
    /* ... */
}
```

| 注解 | 从哪里取 | 主要用途 |
|---|---|---|
| `@PathVariable` | URI 模板 `{id}` | 指定资源 |
| `@RequestParam` | Query / 表单参数 | 分页、筛选 |
| `@RequestBody` | HTTP Body | JSON DTO |

参数类型不匹配可能在调用 Controller 之前报错，不能指望进入方法之后手动判断。

---

## 11. DTO / Entity / VO 与校验

### 11.1 三类对象的定位

```text
前端 JSON → 请求 DTO → Controller → Service
                                    ↓
                              Entity/DO → Mapper → 数据库
                                    ↓
                               响应 VO → JSON → 前端
```

- **DTO**：接口边界/层之间传递数据，例 `UpdateDishDTO`。
- **Entity / DO**：业务或持久化实体（应遵循项目约定），例 `Dish`。
- **VO**：接口响应的呈现结构，例 `DishVO`。
- 不要将 Entity 所有字段直接暴露给前端，尤其是密码哈希、内部状态、敏感元数据。

### 11.2 Jakarta Validation

```java
public class CreateEmployeeDTO {
    @NotBlank
    @Size(max = 50)
    private String name;

    @NotNull
    @Min(18)
    @Max(65)
    private Integer age;

    @Pattern(regexp = "^1\\d{10}$")
    private String phone;

    // getter / setter
}

@PostMapping("/employees")
public EmployeeVO create(@Valid @RequestBody CreateEmployeeDTO dto) {
    return employeeService.create(dto);
}
```

Spring Boot 3 常用 starter：

```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-validation</artifactId>
</dependency>
```

| 注解 | 规则 |
|---|---|
| `@NotNull` | 不允许 `null`（允许空串） |
| `@NotBlank` | 非 `null` 且至少有非空白字符（CharSequence） |
| `@Size(min,max)` | 限制字符串、集合、数组等长度/大小；**null 默认合法** |
| `@Min` / `@Max` | 数字上下界；**null 默认合法** |
| `@Pattern` | 正则匹配字符串；**null 默认合法** |
| `@DecimalMin` | 精确的小数边界，可用于 BigDecimal |

`@Valid` 负责触发所支持的级联/对象校验，具体规则由约束注解提供；需要项目中有验证实现。直接在方法参数上加约束还可能使用不同的方法校验路径。

### 11.3 处理异常输入

- **非法 JSON**、某些**字段类型转换失败**：常见 `HttpMessageNotReadableException` → HTTP 400。
- **DTO 属性校验失败**：常见 `MethodArgumentNotValidException` → HTTP 400。
- **方法级参数校验**：可能 `HandlerMethodValidationException`，须视 Spring 版本和签名处理。
- **未知字段**：取决于 Jackson 配置；Spring Boot 3 常见默认设置会忽略未知字段。
- **字段缺失**：不一定反序列化失败；可能保留 `null` / 默认值，然后由 `@NotNull` 等判断。

---

## 12. 异常处理、Filter、Interceptor 与 JWT

### 12.1 全局异常处理

```java
@RestControllerAdvice
public class GlobalExceptionHandler {

    @ExceptionHandler(MethodArgumentNotValidException.class)
    public ResponseEntity<Map<String, Object>> validationError(
            MethodArgumentNotValidException ex) {
        return ResponseEntity.badRequest().body(Map.of(
                "code", 400,
                "message", "请求参数不合法"
        ));
    }

    @ExceptionHandler(HttpMessageNotReadableException.class)
    public ResponseEntity<Map<String, Object>> invalidJson(
            HttpMessageNotReadableException ex) {
        return ResponseEntity.badRequest().body(Map.of(
                "code", 400,
                "message", "JSON 格式或字段类型错误"
        ));
    }

    @ExceptionHandler(Exception.class)
    public ResponseEntity<Map<String, Object>> fallback(Exception ex) {
        // 真实项目必须记录完整异常到服务端日志
        return ResponseEntity.internalServerError().body(Map.of(
                "code", 500,
                "message", "服务器内部错误"
        ));
    }
}
```

说明：需导入相关 Spring 类型；若使用方法级校验，应按需补充 `HandlerMethodValidationException` 处理器。示例未包括业务异常、自定义错误码等完整工程能力。

### 12.2 Filter 与 Interceptor

```text
Tomcat
  → Servlet Filter 链（常含 Spring Security Filter Chain）
  → DispatcherServlet
  → HandlerMapping 找到 Controller 方法
  → HandlerInterceptor.preHandle
  → 参数解析 / 校验 / Controller / Service / Mapper
  → MVC 处理返回值（可能已写出 JSON）
  → HandlerInterceptor.postHandle（正常完成路径）
  → HandlerInterceptor.afterCompletion（满足执行条件时）
  → Filter 链返回
```

| 对比 | Filter | Interceptor |
|---|---|---|
| 来源 | Servlet 规范 | Spring MVC |
| 主要位置 | DispatcherServlet 前后 | Handler 执行前后 |
| 可否了解具体 Handler | 通常不直接知道 | 可以 |
| 安全认证 | Spring Security 等通常走过滤器链 | 不建议单独充当安全边界 |

**易错点**：Spring MVC 处理 `@ResponseBody` 时，响应可能在 `postHandle` 之前就已经写出；想统一改变响应体可考虑 `ResponseBodyAdvice` 等更合适机制。

### 12.3 JWT 的流程

```text
登录：账号/密码验证 → 签发 JWT → 客户端保存
后续受保护请求：Authorization: Bearer <jwt>
  → 安全过滤器提取并验证签名/时效等
  → 创建当前请求的认证上下文
  → Controller 从可信上下文取得当前商家 ID
  → Service 检查该商家是否有权修改对应菜品
```

- **认证**回答“你是谁”，**授权**回答“你能做什么”。
- JWT 场景一般是每次受保护请求检查凭据，公开路径可不要求 JWT。
- 签名可在本地校验，但撤销列表、停用用户、权限变化等可能需要 Redis/DB 等配合。
- **HTTP 无状态**不意味着应用不能采用 Session；JWT 是一种认证方案，不是 HTTP 强制规则。
- `SecurityUtils.getCurrentMerchantId()` 不是 Spring 标准 API，一般是项目自定义工具。

---

## 13. 菜品更新接口逐句解析

聊天中的原始示例：

```java
@RestController
@RequestMapping("/api/merchant/dishes")
@RequiredArgsConstructor
public class DishController {

    private final DishService dishService;

    @PutMapping("/{id}")
    public DishVO updateDish(
            @PathVariable Long id,
            @Valid @RequestBody UpdateDishDTO dto) {

        Long merchantId = SecurityUtils.getCurrentMerchantId();

        return dishService.updateDish(id, merchantId, dto);
    }
}
```

逐句理解：

1. `@RestController`：类是 Spring MVC REST 控制器，返回值写响应体。
2. `@RequestMapping("/api/merchant/dishes")`：当前类的 URL 公共前缀。
3. `@RequiredArgsConstructor`：Lombok 生成必需参数构造器，使 `dishService` 可被构造器注入。
4. `private final DishService dishService`：Controller 保存 Service **接口类型**引用，且引用不可重新赋值。
5. `@PutMapping("/{id}")`：匹配 `PUT /api/merchant/dishes/{id}`。
6. `public DishVO updateDish(...)`：公开方法，返回 `DishVO`。
7. `@PathVariable Long id`：取 URL 中菜品 ID，转换为 Long。
8. `@Valid @RequestBody UpdateDishDTO dto`：JSON → DTO，并依据其约束规则校验。
9. `Long merchantId = SecurityUtils.getCurrentMerchantId()`：静态工具方法，从项目定义的可信认证上下文中获得当前商家 ID。
10. `return dishService.updateDish(id, merchantId, dto)`：调用业务层，返回由业务层生成的结果。

**业务安全检查不可省略**：Service 必须核查 `id` 对应菜品是否属于 `merchantId`，不能只因为用户已登录就允许更新。

下面是**单纯请求示意**，不是整个可运行项目：

```http
PUT /api/merchant/dishes/1001 HTTP/1.1
Authorization: Bearer <jwt>
Content-Type: application/json

{"name":"宫保鸡丁","price":18.50,"stock":100}
```

流程：`Tomcat → Filter/Security → DispatcherServlet → 映射 → @RequestBody/Jackson → @Valid → Controller → Service → Mapper → MySQL → DishVO/JSON`。

---

## 14. MyBatis-Plus LambdaQueryWrapper 查询

原始代码：

```java
Long count = employeeMapper.selectCount(
    new LambdaQueryWrapper<Employee>()
        .eq(Employee::getUsername, username)
);
```

分解：

1. `new LambdaQueryWrapper<Employee>()`：创建针对 `Employee` 实体的查询条件对象。
2. `.eq(Employee::getUsername, username)`：追加“username 等于参数 username”的条件。
3. `Employee::getUsername`：**方法引用**用于告诉 MyBatis-Plus 实体属性，并非在这里取出某个员工对象的用户名。
4. `employeeMapper.selectCount(...)`：统计满足条件的记录条数；常用 MyBatis-Plus 版本返回 `Long`。

类似 SQL：

```sql
SELECT COUNT(*)
FROM employee
WHERE username = ?;
```

- 参数一般交由 ORM/预编译参数绑定，不需要自己把用户输入拼到 SQL 字符串。
- `.eq()` 返回 Wrapper，可以接 `.ge()`、`.like()` 等形成链式条件。
- `count > 0` 可用于用户体验层面的“用户名已存在”检查，但应在数据库设置 `UNIQUE(username)` 并处理重复键异常，以防并发注册。

---

## 15. 一次请求的实际执行顺序

以 `PUT /api/merchant/dishes/1001` 为例，梳理不同技术的职责。

```text
[浏览器 / App]
    │  发送 HTTP：方法、URL、Header、JSON Body
    ▼
[网络 + Tomcat]
    │  接收网络请求，创建 Servlet 请求/响应对象
    ▼
[Filter 链 / Spring Security]
    │  认证 JWT，建立认证上下文（实际执行顺序取决于配置）
    ▼
[DispatcherServlet]
    │
    ├─ HandlerMapping：匹配 @PutMapping("/{id}")
    │
    ├─ HandlerInterceptor.preHandle：业务方法之前的拦截点
    │
    ├─ 解析 @PathVariable → Long id
    │  读取 @RequestBody：Jackson 反序列化 → UpdateDishDTO
    │  执行 @Valid（失败转异常处理）
    │
    └─ DishController.updateDish(...)
           │
           ├─ SecurityUtils：从可信上下文获取 merchantId
           └─ DishService：检查权限和业务条件
                    │
                    ▼
                  Mapper
                    │
                    ▼
                   MySQL
                    │
                    ▼
                返回 DishVO
                    │
                    ▼
[Spring MVC：响应序列化为 JSON]
    │
    ▼
[Interceptor 完成回调 / Filter 返回 / Tomcat 发响应]
```

**遇到异常时**：

- Mapper 的运行时异常可沿 Service → Controller 调用链传播。
- 可由 MVC 的异常解析器、`@RestControllerAdvice/@ExceptionHandler` 在适用范围内处理，转换为规范的 HTTP 错误响应。
- Filter 中、DispatcherServlet 之外发生的异常不一定能由 ControllerAdvice 接住；安全过滤器也可能有自身的异常处理策略。

---

## 16. 练习题与复习清单

### 16.1 必会解释的 10 个问题

1. `EmployeeService` 与 `EmployeeServiceImpl` 有什么关系？Controller 为什么依赖接口？
2. 声明引用、`new` 对象、构造器注入有什么区别？`final` 如何初始化？
3. `@Component/@Service/@Configuration/@Bean` 各做什么？Spring 如何扫描并装配 Bean？
4. Lambda 为什么必须对应函数式接口？方法引用与匿名内部类的区别是什么？
5. `filter/map/sorted/forEach` 哪些是中间操作、哪些是终止操作？
6. `throw`、`throws` 各做什么？受检异常与运行时异常如何传播？
7. `@PathVariable/@RequestParam/@RequestBody` 各从哪里拿数据？
8. Tomcat、DispatcherServlet、HttpMessageConverter、Jackson、DTO 分别负责什么？
9. `@Valid` 为什么不能单独完成字段校验？非法 JSON 与校验失败有什么不同？
10. JWT 认证成功后，为什么仍需在 Service 判断商家的资源权限？

### 16.2 建议手写（按优先级）

- [ ] **纯 Java**：接口 + 实现类 + 构造器注入 + `final`。
- [ ] **函数式接口**：匿名内部类 ↔ Lambda ↔ 方法引用的对照。
- [ ] **集合转换**：`List<Employee>` → `List<EmployeeVO>`（`map`）。
- [ ] **异常**：运行时异常从 Mapper 模拟向上抛；受检异常用 `throws`/`catch`。
- [ ] **MVC**：`GET /employees/{id}`、`GET /employees?page=...`、`POST /employees`。
- [ ] **DTO 校验**：`@NotBlank`、`@NotNull`、`@Size`、`@Min`，配合 `@Valid`。
- [ ] **异常统一返回**：`@RestControllerAdvice` 处理 DTO 校验异常和非法 JSON。
- [ ] **认证与授权**：过滤器验证 token，Service 检查菜品所属商家。
- [ ] **MyBatis-Plus**：`LambdaQueryWrapper.eq`、`selectCount`，以及数据库唯一约束。

### 16.3 高频速记

```text
interface  = 能做什么（契约）
implements = 怎么做（实现）
IoC        = 谁负责创建与管理依赖
DI         = 如何提供依赖（常用构造器）
@Bean      = 方法返回对象注册 Bean
@Component = 类作为可扫描组件
@RequestMapping = 什么请求交给什么方法
@RequestBody    = Body → Java 对象
@Valid          = 触发声明好的验证约束
DTO / Entity / VO = 请求或传输 / 业务或持久化 / 响应
throw      = 主动抛出
throws     = 声明可能抛出
Filter     = Servlet 过滤器层
Interceptor = Spring MVC Handler 层
JWT 认证   = 确认身份；资源授权 = 验证操作权限
```

---

## 参考资料

核对时主要参考官方资料；以下链接属于**外部核验依据**，与前面整理的聊天知识点区分开：

1. Spring Boot 3.5：Spring Beans and Dependency Injection  
   https://docs.spring.io/spring-boot/3.5/reference/using/spring-beans-and-dependency-injection.html
2. Spring MVC：Mapping Requests  
   https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-controller/ann-requestmapping.html
3. Spring MVC：`@RequestBody` 与 Validation  
   https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-controller/ann-methods/requestbody.html  
   https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-controller/ann-validation.html
4. Spring MVC：Handler Interception  
   https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-servlet/handlermapping-interceptor.html
5. Spring MVC：Interceptor 的安全边界提示  
   https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-config/interceptors.html
6. Lombok：`@RequiredArgsConstructor`  
   https://projectlombok.org/features/constructor
7. Java 21 Javadoc：`String`、`UUID`、`SecureRandom`  
   https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/lang/String.html  
   https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/UUID.html  
   https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/security/SecureRandom.html
8. MyBatis-Plus：条件构造器 `LambdaQueryWrapper`  
   https://baomidou.com/guides/wrapper/

> **本笔记的适用边界：**它完整涵盖了本次聊天里实际讨论的概念与案例，但不是 Spring Boot 全部知识体系，也不是可直接复制运行的整套项目代码。实践中遇到 API 行为差异，请先核对自己的 Spring Boot、Java、Jackson、MyBatis-Plus 版本及配置。
