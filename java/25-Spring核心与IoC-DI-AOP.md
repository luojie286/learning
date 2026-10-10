# 25 · Spring 核心（IoC · DI · AOP）

> 一句话概括：Spring Framework 的三大支柱——IoC 容器负责创建并管理对象，DI 把依赖交给对象，AOP 把日志/事务等横切逻辑从业务方法中抽离。

## 一、Spring 是什么，解决什么问题

Spring 是 Java 生态的**应用开发框架**。不用框架时，业务类自己 `new` 依赖：

```java
public class UserService {
    private final UserDao userDao = new UserDao();  // 写死实现，难替换、难测试
}
```

引入 Spring 后，对象创建与依赖组装交给容器：

```text
        Spring IoC 容器
          │       │
       UserDao  UserService
          └───────>│   注入依赖
```

**核心思想：业务类专注业务，Spring 管对象和对象之间的关系。** 框架不是魔法，本质是一堆提供通用代码、约定和扩展点的 JAR。

## 二、IoC 与 Bean 容器

- **Bean**：被 Spring IoC 容器管理的普通 Java 对象，不是特殊数据类型。
- **容器**（`BeanFactory` / `ApplicationContext`）：发现注册 Bean 定义 → 创建 Bean 并注入依赖 → 管理作用域与生命周期回调 → 按需提供 Bean。

```java
@Component
public class UserManager {}            // 在组件扫描范围内即可被注册为 Bean
UserManager m = context.getBean(UserManager.class);  // 业务中优先用注入，而非到处 getBean()
```

> 常见误解：IoC ≠ "Spring 自动创建程序里所有的类"——只管理注册进容器的 Bean；Bean 默认作用域 singleton，但不是唯一作用域。

## 三、DI 与构造器注入

**DI 是实现 IoC 的常见方式**：对象不自己创建依赖，由容器传入。

```java
@Service
public class UserService {
    private final UserDao userDao;
    public UserService(UserDao userDao) {   // 构造器注入
        this.userDao = userDao;
    }
}
```

执行逻辑：扫描到两个类 → 创建 `UserDao` Bean → 创建 `UserService` 时发现构造器依赖 → 把 `UserDao` 注入。

**优先构造器注入**：依赖明确、便于测试；单构造器场景通常可省略 `@Autowired`。注意 `private final UserService x;` 只是声明字段，`final` 必须在构造器路径中赋值——Spring 构造器注入的本质是容器提供参数，再用普通 Java 赋值语句初始化。

## 四、常见组件注解

| 注解 | 用途 |
|---|---|
| `@Component` | 通用组件 |
| `@Service` | 业务层组件 |
| `@Repository` | 数据访问层组件 |
| `@Controller` / `@RestController` | MVC 控制器 / REST 控制器（=`@Controller`+`@ResponseBody`） |
| `@Configuration` + `@Bean` | 配置类 / 用方法显式定义 Bean |
| `@Autowired` | 自动装配；单构造器可省略 |

`@Service` 只声明组件身份，**不会自动执行业务方法**，也不生成业务实现。

## 五、AOP 与声明式事务

**AOP** 把多个方法共同需要的横切关注点（日志、耗时统计、事务、权限）抽成切面：

```text
外部调用 → Spring 代理对象 → 附加逻辑（前） → 业务方法 → 附加逻辑（后）
```

事务案例：

```java
@Transactional
public void transfer() { /* 扣 A、加 B，整体成功或整体回滚 */ }
```

三个必须知道的细节：

1. 不是加注解就必然生效——需要事务配置 + 通过代理调用。
2. 默认对 `RuntimeException` 和 `Error` 回滚；受检异常要配 `rollbackFor`。
3. **同类内部自调用会绕过代理**，事务可能失效。

现阶段理解"代理对象在调用前后加功能"即可，不必深挖代理源码。反射（`Class.forName` / `getMethod` / `invoke`）是 Spring 读取类元数据、管理 Bean 的机制之一，但 Spring 还依赖扫描、BeanDefinition、后处理器、动态代理等——不只靠反射（反射细节见 `09-JavaBean与反射.md`）。

## 六、AOP 注解实战（选读）

```java
@Aspect
@Component
public class LogAspect {
    @Before("execution(* com.example.service.UserService.*(..))")
    public void logBefore() { System.out.println("调用前记录日志"); }

    @After("execution(* com.example.service.UserService.*(..))")
    public void logAfter() { System.out.println("方法执行结束"); }
}
```

- `execution(* com.example.service.UserService.*(..))`：任意返回类型 + 该类任意方法 + 任意参数。
- `@Before` 方法前执行；`@After` 方法结束时执行（含抛异常）；`@Around` 环绕控制、自己决定何时 `proceed()`。
- 原理是**动态代理**（接口用 JDK 代理，无接口用 CGLIB），不改目标源码。
- 最大陷阱：`methodA()` 内部直接调本类的 `@Transactional methodB()` 会**绕过代理**，事务/AOP 都不生效——内部调用没再经过代理对象。

## 📌 易错点

- "IoC = Spring 创建所有对象"——只创建注册进容器的 Bean。
- "`@Service` 会自动跑业务"——只是身份声明，方法要被调用才执行。
- "`private final X x;` 不用初始化"——必须初始化，只是由构造器完成。
- "`@Transactional` 加了就回滚"——受检异常默认不回滚；类内自调用可能失效。
- "删掉注解程序就崩"——注解是元数据，是读取它的框架让功能生效；没有注解也能用 XML/配置类实现同样功能。

## 🧠 速记口诀

```text
IoC 管对象，DI 传依赖，构造注入最推荐
Bean 是容器里的普通对象，singleton 是默认
AOP 靠代理，事务三坑：配置、回滚规则、自调用
注解只是元数据，框架读到才生效
```

## ✅ 自测问题

1. Bean 和普通 Java 对象是什么关系？`ApplicationContext` 是什么？
2. IoC 和 DI 分别强调什么？为什么推荐构造器注入？
3. `private final UserService userService;` 这行代码创建对象了吗？
4. `@Transactional` 默认对什么异常回滚？为什么类内部自调用可能失效？
5. Spring 只靠反射工作吗？

> 来源：原始《Spring 基础学习笔记》(20261010-Spring基础学习笔记.md)、《Java 后端学习笔记：Spring、Web 请求处理与数据库设计》(Java后端学习笔记_Spring与Web请求处理_2026-10-08.md)，两文重叠部分已去重合并。
