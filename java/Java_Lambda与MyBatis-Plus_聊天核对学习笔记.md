# Java Lambda 与 MyBatis-Plus 查询条件写法：聊天核对学习笔记

> 整理日期：2026-10-23  
> 范围：本次聊天实际讲解的 Java Lambda 表达式、函数式接口、集合与 Stream、方法引用，以及 `LambdaQueryWrapper` 的示例。最初提出的“详细讲解 MyBatis-Plus 查询条件构造器”尚未在聊天中完整展开；本文仅在末尾提供少量**官方资料核对后的延伸补充**。

## 一、核对结论与纠错

原回答的核心知识点**基本正确**，但应补齐以下前提：

| 原聊天中的内容 | 核对结果 | 准确说法 |
|---|---|---|
| Lambda 简化函数式接口实现 | 正确 | Lambda 需要具有兼容的**目标函数式接口类型**。函数式接口只要求一个抽象方法，可以有 `default`、`static` 方法。 |
| `Consumer`、`Supplier`、`Function`、`Predicate` | 正确 | 这是四种常用的核心函数式接口，并非 Java 中仅有的四种。 |
| `filter`、`map`、`forEach`、`sort` 示例 | 正确 | `List.forEach`/`List.sort` 与 `Stream` 的方法不要混为一谈；`filter`、`map` 属于 Stream 中间操作。 |
| `nums.stream()...toList()` | **缺少版本条件** | `Stream.toList()` 从 **Java 16** 开始支持，返回**不可修改的 List**。Java 8 用 `collect(Collectors.toList())`。 |
| `List.of("Alice", "Bob")` | **缺少版本条件** | `List.of()` 从 **Java 9** 开始支持，返回不可修改的 List。Java 8 可改用 `Arrays.asList(...)`。 |
| `Arrays.asList(5, 2, 8, 1, 9)` 后使用 `sort` | 正确，但有前提 | `Arrays.asList` 返回**长度固定**的 List，允许修改已有元素或排序，但不能 `add/remove`。 |
| `User::getAge` 不是直接调用 getter | 正确 | 这是**方法引用**；MyBatis-Plus 的 Lambda Wrapper 可据此识别实体属性，再按配置映射为数据库字段。 |
| MyBatis-Plus `like(User::getName, "张")` | 正确 | 默认相当于 `LIKE '%张%'`；具体 SQL 形式通常带有绑定参数，而不是直接拼接输入值。 |
| Lambda 捕获局部变量必须 `final` 或 effectively final | 正确 | 只对**被捕获的局部变量/方法参数**有这个限制，不等于 Lambda 内无法修改任何对象的状态。 |

> 易记：**Lambda 是行为的简写；`::` 是方法引用；MyBatis-Plus 中的 `User::getName` 是“描述要使用哪个属性”，而不是马上读取某个 User 的姓名。**

## 二、Lambda 基础

### 2.1 语法与规则

```java
(参数) -> 表达式

(参数1, 参数2) -> {
    // 多行语句
    return 结果; // 仅当目标方法需要返回值时
}
```

- 只有一个参数时，可以省略括号：`x -> x * 2`。
- 无参数必须写 `()`：`() -> 100`。
- 单个表达式可以省略 `{}` 和 `return`。
- 多条语句需要 `{}`；若目标方法需返回值，必须沿所有正常完成的执行路径返回相应类型的值。
- Lambda 不能脱离目标类型单独成为一个 Java 表达式声明：一般赋给函数式接口变量，或作为期待该接口的方法参数。

### 2.2 对比匿名内部类

```java
// 匿名内部类
Comparator<Integer> cmp1 = new Comparator<Integer>() {
    @Override
    public int compare(Integer a, Integer b) {
        return Integer.compare(a, b);
    }
};

// Lambda
Comparator<Integer> cmp2 = (a, b) -> Integer.compare(a, b);

// 方法引用（签名适配时可以进一步简化）
Comparator<Integer> cmp3 = Integer::compare;
```

不建议使用 `(a, b) -> a - b` 比较任意 `int`，因为减法可能整数溢出。使用 `Integer.compare(a, b)` 更稳妥。

## 三、函数式接口

**函数式接口**：只有一个需要实现的抽象方法的接口。通常可以用 `@FunctionalInterface` 显式标记。

| 接口 | 方法 | 输入 / 输出 | 常见场景 |
|---|---|---|---|
| `Consumer<T>` | `accept(T)` | 一个输入，无返回值 | 打印、遍历处理 |
| `Supplier<T>` | `get()` | 无输入，返回一个值 | 延迟提供值、创建对象 |
| `Function<T, R>` | `apply(T)` | 一个输入，返回转换结果 | `map` |
| `Predicate<T>` | `test(T)` | 一个输入，返回 `boolean` | `filter` |

```java
import java.util.function.*;

Consumer<String> printer = s -> System.out.println(s);
Supplier<Integer> supplier = () -> 100;
Function<Integer, Integer> doubled = x -> x * 2;
Predicate<Integer> greaterThanTen = x -> x > 10;

printer.accept("Hello");             // Hello
System.out.println(supplier.get());   // 100
System.out.println(doubled.apply(5)); // 10
System.out.println(greaterThanTen.test(11)); // true
```

其他常见接口还有 `Comparator<T>`、`Runnable`、`BiFunction<T,U,R>`、`UnaryOperator<T>` 等，不能将“常见四大接口”理解成全部。

## 四、集合与 Stream 中的 Lambda

### 4.1 遍历与排序

```java
import java.util.*;

List<Integer> nums = new ArrayList<>(Arrays.asList(5, 2, 8, 1, 9));

// 遍历（List/Iterable 的 forEach）
nums.forEach(x -> System.out.println(x));

// 升序
nums.sort((a, b) -> Integer.compare(a, b));
// 等价写法：nums.sort(Integer::compare);

// 降序
nums.sort((a, b) -> Integer.compare(b, a));
// 或：nums.sort(Comparator.reverseOrder());
```

注意：`new ArrayList<>(...)` 得到的是可增删的列表；直接使用 `Arrays.asList(...)` 则为固定长度列表，也能 `sort`，但不能 `add/remove`。

### 4.2 Stream：过滤、映射、排序、收集

```java
import java.util.*;
import java.util.stream.Collectors;

List<Integer> nums = Arrays.asList(5, 2, 8, 1, 9);

// Java 8 及以上
List<Integer> result = nums.stream()
    .filter(x -> x > 3)    // 保留 5、8、9
    .map(x -> x * 2)       // 转换为 10、16、18
    .sorted()              // 排序
    .collect(Collectors.toList());

System.out.println(result); // [10, 16, 18]
```

如果项目使用 Java 16+，末尾也可以写 `.toList()`，但其返回列表不可修改。`Collectors.toList()` **不保证**具体的列表实现或可修改性；如果明确需要可修改列表，写：

```java
.collect(Collectors.toCollection(ArrayList::new));
```

三个重要区别：

1. `filter(Predicate)`：按照条件**筛选**元素。
2. `map(Function)`：把每个元素**转换**为另一个元素。
3. `forEach(Consumer)`：对每个元素**执行操作**，不生成映射后的新 Stream。

`filter`、`map`、`sorted` 都是中间操作；`collect`、`toList`、`forEach` 是终止操作。Stream 的中间操作通常在执行终止操作时才真正处理数据。

## 五、方法引用 `::`

若 Lambda 的主要工作就是调用已有方法，可在**目标函数式接口类型兼容**时写成方法引用。

| 种类 | 示例 | 近似等价的 Lambda |
|---|---|---|
| 静态方法 | `Integer::parseInt` | `s -> Integer.parseInt(s)` |
| 已有对象的实例方法 | `System.out::println` | `x -> System.out.println(x)` |
| 某类任意对象的实例方法 | `String::length` | `s -> s.length()` |
| 构造方法 | `ArrayList::new` | `() -> new ArrayList<>()` |

```java
import java.util.*;
import java.util.stream.Collectors;

List<String> names = Arrays.asList("Alice", "Bob");

List<Integer> lengths = names.stream()
    .map(String::length)
    .collect(Collectors.toList());

System.out.println(lengths); // [5, 3]
```

`String::length` 并不是当场执行 `length()`；当 Stream 处理每个 `String` 元素时，才把该元素作为方法接收者去调用。

## 六、局部变量捕获与作用域

```java
int n = 10;
Function<Integer, Integer> f = x -> x + n; // 正确：n 没有被重新赋值

// n = 20; // 若有这行赋值，前面 Lambda 捕获 n 将不能通过编译
```

被 Lambda 捕获的局部变量必须声明为 `final`，或者实际上从未被重新赋值（**effectively final**）。这不禁止修改被引用对象的内部状态，但涉及共享可变状态、并发时仍需注意安全问题。

## 七、关联 MyBatis-Plus：LambdaQueryWrapper

这一部分对应原聊天已有的**方法引用示例**，并在官方文档基础上增加少量解释。假设项目中存在 `User` 实体及 `getAge()`、`getName()`、`getScore()` 方法，且字段映射分别为 `age`、`name`、`score`。

```java
LambdaQueryWrapper<User> wrapper = new LambdaQueryWrapper<>();

wrapper.eq(User::getAge, 18)
       .like(User::getName, "张")
       .ge(User::getScore, 90);

// 例如：userMapper.selectList(wrapper);
```

理解为逻辑条件：

```sql
WHERE age = ?
  AND name LIKE ?
  AND score >= ?
```

对应参数示意：`18`、`%张%`、`90`。这只是便于理解的 SQL 示意；真实打印 SQL、占位方式、表名依赖具体 MyBatis-Plus 配置与执行环境。

- `eq`：相等（`=`）。
- `like`：模糊匹配，默认在关键词两侧加 `%`。
- `ge`：大于等于（`>=`）。
- 不显式连接时，多数链式条件默认使用 `AND`。

### 7.1 为什么使用 `User::getName`？

```java
// 普通 QueryWrapper：传数据库字段名字符串
new QueryWrapper<User>().eq("name", "张三");

// LambdaQueryWrapper：传实体方法引用
new LambdaQueryWrapper<User>().eq(User::getName, "张三");
```

后一种形式借助 MyBatis-Plus 的字段解析机制和实体映射，提高类型检查与重构支持。它不表示 Java 此刻对某个 `User` 对象执行了 getter。字段实际如何对应数据库列，仍受实体映射、注解、命名规则等影响。

### 7.2 额外补充：动态条件与括号（原聊天未详细讲解）

```java
String keyword = "张";
Integer minAge = 18;

LambdaQueryWrapper<User> query = new LambdaQueryWrapper<User>()
    .like(keyword != null && !keyword.isEmpty(), User::getName, keyword)
    .ge(minAge != null, User::getAge, minAge);
```

条件参数为 `false` 时，该查询条件不会加入。

```java
LambdaQueryWrapper<User> query = new LambdaQueryWrapper<User>()
    .eq(User::getAge, 18)
    .and(w -> w.like(User::getName, "张")
               .or()
               .like(User::getName, "李"));
```

逻辑上相当于 `age = 18 AND (name LIKE '%张%' OR name LIKE '%李%')`。这是 MyBatis-Plus 文档支持的条件嵌套写法，**不是**本次聊天中已经完整展开的内容。

## 八、最容易混淆的五个点

1. **Lambda 与方法引用不是同一个符号**：`->` 表示 Lambda，`::` 表示引用已有方法/构造器；都需要兼容的目标类型。
2. **函数式接口不等于只有一个方法**：其关键是只有一个抽象方法，允许默认方法和静态方法。
3. **`List.forEach` 不等于 Stream 的中间操作**：`forEach` 是执行操作；`map` 才是元素转换。
4. **JDK 版本影响示例**：Java 8 支持 Lambda、Stream；Java 9 支持 `List.of`；Java 16 支持 `Stream.toList`。
5. **方法引用不能简单等同于“执行方法”**：`User::getName` 是可以被框架检查、解释的引用，不会立即调用某个对象上的 `getName()`。

## 九、自测

1. `Predicate<Integer> p = x -> x > 10;` 为什么可以赋值？`test(11)` 返回什么？
2. 将 `s -> System.out.println(s)` 改写为方法引用。
3. `nums.stream().filter(x -> x > 3).map(x -> x * 2)` 中 `filter` 和 `map` 分别接收哪类函数式接口？
4. Java 8 中如何代替 `stream().toList()`？如果要确保收集结果可增删，应如何写？
5. `new LambdaQueryWrapper<User>().eq(User::getName, "张三")` 中，`User::getName` 是否会立即调用实体对象的方法？
6. `eq(User::getAge, 18).like(User::getName, "张")` 默认是什么逻辑关系？

**参考答案**：① `Predicate` 的单一抽象方法 `test` 与该 Lambda 签名匹配，结果为 `true`；② `System.out::println`；③ `Predicate` 与 `Function`；④ `collect(Collectors.toList())`，确保可修改则 `collect(Collectors.toCollection(ArrayList::new))`；⑤ 不会；⑥ `AND`。

## 十、参考资料（官方）

1. Oracle：Lambda Expressions  
   https://docs.oracle.com/javase/tutorial/java/javaOO/lambdaexpressions.html
2. Oracle：Method References  
   https://docs.oracle.com/javase/tutorial/java/javaOO/methodreferences.html
3. Oracle：Java 16 `Stream` API（`toList`）  
   https://docs.oracle.com/en/java/javase/16/docs/api/java.base/java/util/stream/Stream.html
4. Oracle：Java 9 `List` API（`List.of`）  
   https://docs.oracle.com/javase/9/docs/api/java/util/List.html
5. MyBatis-Plus：条件构造器  
   https://baomidou.com/guides/wrapper/

---

**最终记忆主线**：函数式接口提供目标类型 → Lambda `->` 实现行为 → 方法引用 `::` 复用已有方法 → Stream 通过 `filter/map` 组合处理集合 → MyBatis-Plus 在 Lambda Wrapper 中解析实体 getter 方法引用来组织查询条件。
