# 09 · JavaBean 与反射

> 一句话概括：JavaBean 是约定，反射是运行时看穿对象的眼睛。

---

## 一、JavaBean：一种约定，不是新语法

JavaBean 不是：

- ❌ 新的数据类型
- ❌ 新的关键字
- ❌ 一个必须继承的父类

而是一套**编程约定**——满足约定的类就是 JavaBean。

### 典型 JavaBean

```java
public class User {

    private String name;

    public User() {                          // ① public 无参构造
    }

    public String getName() {                // ② getter
        return name;
    }

    public void setName(String name) {       // ③ setter
        this.name = name;
    }
}
```

### 四大特征

| 特征 | 说明 |
|------|------|
| `public class` | 类公开 |
| `private` 成员变量 | 字段私有 |
| `public` 无参构造 | 便于框架反射创建 |
| `getter` / `setter` | 公开访问入口 |

### 现代 Java 开发中的常见用途

- Spring 的 Bean
- ORM 映射（MyBatis / Hibernate）
- DTO / VO / POJO
- 配置对象
- 各种数据传输对象

---

## 二、反射：运行时看穿对象

> **反射** = 在程序运行时，**动态地**获取类的信息（字段、方法、构造器）并操作对象。

### 为什么需要反射

很多场景**编译期不知道要操作哪个类**：

- 框架要实例化你写的类（Spring 创建 Bean）
- 通用工具要读 / 写任意对象的字段（JSON 序列化）
- 注解处理器要根据注解生成调用

这些都必须靠反射完成。

### 反射能拿到的信息

```java
Class<?> clazz = obj.getClass();   // 拿到「类对象」

Field[]     fields     = clazz.getDeclaredFields();      // 所有字段
Method[]    methods    = clazz.getDeclaredMethods();     // 所有方法
Constructor<?>[] ctors = clazz.getDeclaredConstructors();// 所有构造器
Annotation[] annos     = clazz.getAnnotations();         // 注解
```

### 通过反射创建对象和调用方法

```java
// 1. 拿到 Class 对象
Class<?> clazz = Class.forName("com.foo.User");

// 2. 调用无参构造创建实例
Object user = clazz.getDeclaredConstructor().newInstance();

// 3. 调用 setter
Method setName = clazz.getMethod("setName", String.class);
setName.invoke(user, "罗杰");

// 4. 读私有字段
Field nameField = clazz.getDeclaredField("name");
nameField.setAccessible(true);   // 绕过 private 限制
String value = (String) nameField.get(user);
```

---

## 三、反射的三个层级入口

| 入口 | 场景 |
|------|------|
| `Class.forName("全限定类名")` | 类名是字符串（最常用，框架首选） |
| `对象.getClass()` | 已经有一个实例，反查其类型 |
| `类名.class` | 编译期已知类型 |

---

## 四、反射的代价

| 维度 | 影响 |
|------|------|
| 性能 | 反射调用比直接调用**慢 10-100 倍**（JIT 难优化） |
| 安全 | `setAccessible(true)` 可绕过 private 封装 |
| 复杂度 | 代码难读，错误延迟到运行时才发现 |

> 反射是**运行时元编程能力**，性能敏感的核心路径**不要**滥用——框架代码用，业务热路径用直接调用。

---

## 五、Java 反射与注解、动态代理的关系

```
注解（Annotation）   → 给类/方法/字段打标签
   ↓
反射（Reflection）   → 在运行时读取这些标签和类结构
   ↓
动态代理（Proxy）    → 在运行时根据接口动态生成实现类
```

三者一起构成了 Java 框架（Spring / MyBatis / Hibernate 等）的底层基石。

---

## 📌 易错点

1. **JavaBean 不是语法**——只是约定。不写 getter/setter 编译能过，但**框架可能用不了**。
2. **`getDeclaredFields()` 不包括父类字段**，`getFields()` 只包括 public 字段（含继承）。
3. **`setAccessible(true)` 才能访问 private 字段/方法**，否则抛 `IllegalAccessException`。
4. **反射调用可能抛一堆受检异常**——`ClassNotFoundException`, `NoSuchMethodException`, `InvocationTargetException` 等都要处理。
5. **反射性能差**——不要用在热点路径上。
6. **`ClassNotFoundException`（类找不到）和 `NoClassDefFoundError`（类初始化失败）是不同的**，不要混淆。

---

## 🧠 速记口诀

```
JavaBean：private 字段 + public 无参构造 + getter/setter
反射：运行时拿 Class，拿 Field、Method、Constructor
setAccessible(true) 才能碰 private
注解 → 反射 → 动态代理，Java 框架的三大底层支柱
```

---

## ✅ 自测问题

1. JavaBean 的四个特征是什么？
2. 反射能在运行时拿到类的哪些信息？
3. `obj.getClass()`、`Class.forName(...)`、`User.class` 三种拿 Class 的方式各自适用什么场景？
4. 为什么反射要先调用 `setAccessible(true)` 才能访问 private 字段？
5. 反射的最大代价是什么？什么情况下不应该用反射？
6. Java 框架（Spring 等）底层最依赖哪三项能力？

---

> 来源：原始 `反射.md`（原文只有标题碎片，本文件按主题重构并补全）+ 原始 `2026-09-09_Java学习对话总结.md`（JavaBean 部分）