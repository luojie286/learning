# Java 学习笔记：泛型、反射、List 与 equals

## 1. `Pair<Integer>` 与 `Pair<Number>` 的关系

假设：

```java
Integer extends Number
```

不能推出：

```java
Pair<Integer> extends Pair<Number>
```

也就是说，Java 泛型默认是**不变的（invariant）**。

例如：

```java
Pair<Integer> p1 = new Pair<>();
Pair<Number> p2 = p1;   // 编译错误
```

原因是如果允许这样赋值，会破坏类型安全：

```java
Pair<Integer> p1 = new Pair<>();
Pair<Number> p2 = p1;

p2.setFirst(3.14);  // Double 也是 Number
```

这样就会把 `Double` 放进本应只保存 `Integer` 的 `Pair<Integer>` 中。

### 结论

```text
A extends B
```

不代表：

```text
Generic<A> extends Generic<B>
```

---

## 2. `? extends Number`

```java
Pair<? extends Number> p = new Pair<Integer>();
```

`? extends Number` 表示：

> `?` 可以是 `Number` 本身，也可以是 `Number` 的任意子类。

因此它可以匹配：

```java
Pair<Number>
Pair<Integer>
Pair<Double>
Pair<Float>
```

例如：

```java
Pair<? extends Number> p1 = new Pair<Integer>();
Pair<? extends Number> p2 = new Pair<Double>();
Pair<? extends Number> p3 = new Pair<Number>();
```

可以近似理解为：

```text
Pair<? extends Number>
        ↓
匹配 Pair<Number>
     Pair<Integer>
     Pair<Double>
     Pair<Float>
     ...
```

### 为什么不能随便写入

```java
Pair<? extends Number> p = new Pair<Integer>();

p.setFirst(123); // 编译错误
```

因为编译器只知道它是“某个 `Number` 子类”，但不知道具体到底是 `Integer`、`Double` 还是其他类型。

所以 `? extends T` 更适合**读取**。

---

# 3. 泛型与反射

Java 泛型和反射结合时，最重要的概念是：

> Java 泛型存在类型擦除，但部分写在类结构声明中的泛型信息会保存在 `.class` 元数据里，因此反射有时能获取泛型信息。

## 3.1 类型擦除

```java
class Box<T> {
    private T value;
}
```

运行时：

```java
Box<Integer> a = new Box<>();
Box<String> b = new Box<>();

System.out.println(a.getClass() == b.getClass());
```

结果：

```text
true
```

因为运行时它们本质上都是同一个 `Box` 类。

也就是说，通常不能通过：

```java
box.getClass()
```

直接知道对象最初是 `Box<Integer>` 还是 `Box<String>`。

---

## 3.2 字段上的泛型信息可以反射获取

例如：

```java
class Student {
    private List<String> names;
}
```

反射：

```java
Field field = Student.class.getDeclaredField("names");

System.out.println(field.getType());
System.out.println(field.getGenericType());
```

结果类似：

```text
interface java.util.List
java.util.List<java.lang.String>
```

区别：

```java
field.getType();
```

只能拿到原始类型：

```text
List
```

而：

```java
field.getGenericType();
```

可以拿到：

```text
List<String>
```

---

## 3.3 `ParameterizedType`

```java
Field field = Student.class.getDeclaredField("names");
Type type = field.getGenericType();

if (type instanceof ParameterizedType pt) {
    Type[] args = pt.getActualTypeArguments();
    System.out.println(args[0]);
}
```

输出：

```text
class java.lang.String
```

可以理解为：

```text
List<String>
 │    │
 │    └── 实际泛型参数 String
 └──── 原始类型 List
```

常用方法：

```java
pt.getRawType();
pt.getActualTypeArguments();
```

---

## 3.4 方法泛型信息

```java
class UserService {
    public List<String> getNames(Map<Integer, String> map) {
        return null;
    }
}
```

反射：

```java
Method method = UserService.class.getMethod("getNames", Map.class);

Type returnType = method.getGenericReturnType();
Type[] parameterTypes = method.getGenericParameterTypes();
```

可以获取：

```text
List<String>
Map<Integer, String>
```

而普通 API：

```java
method.getReturnType();
method.getParameterTypes();
```

只能得到原始类型。

### 常见对照

| 普通反射 | 泛型反射 |
|---|---|
| `getType()` | `getGenericType()` |
| `getReturnType()` | `getGenericReturnType()` |
| `getParameterTypes()` | `getGenericParameterTypes()` |

---

## 3.5 获取父类泛型参数

```java
class Parent<T> {
}

class Son extends Parent<String> {
}
```

反射：

```java
Type type = Son.class.getGenericSuperclass();
```

可以得到：

```text
Parent<java.lang.String>
```

继续解析：

```java
ParameterizedType pt = (ParameterizedType) type;
Type[] args = pt.getActualTypeArguments();

System.out.println(args[0]);
```

输出：

```text
class java.lang.String
```

### 总结

- 对象实例本身的泛型参数通常因为类型擦除拿不到。
- 字段、方法、父类、接口声明上的泛型信息可以通过反射读取。

---

# 4. Java `List` 的基本使用

`List` 的特点：

- 有序
- 可重复
- 支持下标访问

最常见实现：

```java
ArrayList
LinkedList
```

常见声明：

```java
List<String> list = new ArrayList<>();
```

推荐使用接口类型接收具体实现。

---

## 4.1 常用方法

### 添加

```java
list.add("A");
list.add("B");
list.add(1, "X");
```

### 获取

```java
String s = list.get(0);
```

### 修改

```java
list.set(1, "Java");
```

### 删除

按下标：

```java
list.remove(1);
```

按元素：

```java
list.remove("Java");
```

### `List<Integer>` 删除陷阱

```java
List<Integer> list = new ArrayList<>();
list.add(10);
list.add(20);
list.add(30);

list.remove(1);
```

删除的是下标 `1` 的元素，也就是 `20`。

如果想删除值 `1`：

```java
list.remove(Integer.valueOf(1));
```

### 查询

```java
list.contains("A");
list.indexOf("A");
```

### 大小

```java
list.size();
list.isEmpty();
list.clear();
```

---

# 5. List 的遍历

## 5.1 增强 for

```java
for (String s : list) {
    System.out.println(s);
}
```

## 5.2 普通 for

```java
for (int i = 0; i < list.size(); i++) {
    System.out.println(list.get(i));
}
```

## 5.3 Iterator

```java
Iterator<String> it = list.iterator();

while (it.hasNext()) {
    String s = it.next();
    System.out.println(s);
}
```

### 遍历时删除

推荐：

```java
Iterator<String> it = list.iterator();

while (it.hasNext()) {
    String s = it.next();
    if (s.equals("A")) {
        it.remove();
    }
}
```

不要直接：

```java
for (String s : list) {
    if (s.equals("A")) {
        list.remove(s);
    }
}
```

否则容易出现：

```text
ConcurrentModificationException
```

---

# 6. `ArrayList` 的实现原理

`ArrayList` 底层本质上是：

> 动态数组。

内部可以近似理解为：

```java
Object[] elementData;
int size;
```

例如：

```text
0      1      2      3      4
[A]    [B]    [C]    null   null

size = 3
```

注意：

```text
size != 数组容量
```

---

## 6.1 `add()` 原理

简化理解：

```java
public boolean add(E e) {
    if (size == elementData.length) {
        grow();
    }

    elementData[size] = e;
    size++;
    return true;
}
```

---

## 6.2 扩容机制

数组长度固定，所以容量不够时需要：

```text
创建更大的新数组
→ 复制旧数组元素
→ elementData 指向新数组
```

典型扩容思想约为：

```text
1.5 倍
```

核心计算：

```java
int newCapacity = oldCapacity + (oldCapacity >> 1);
```

因为：

```text
oldCapacity >> 1
≈ oldCapacity / 2
```

---

## 6.3 ArrayList 的复杂度

| 操作 | 时间复杂度 |
|---|---:|
| `get(index)` | `O(1)` |
| `set(index)` | `O(1)` |
| 尾部 `add()` | 平均 `O(1)` |
| 中间插入 | `O(n)` |
| 删除 | `O(n)` |
| `contains()` | `O(n)` |

中间插入或删除需要移动数组元素。

---

# 7. `LinkedList` 的实现原理

`LinkedList` 底层是：

> 双向链表。

节点可近似理解为：

```java
class Node<E> {
    E item;
    Node<E> next;
    Node<E> prev;
}
```

结构：

```text
[A] ⇄ [B] ⇄ [C]
```

内部一般维护：

```java
Node<E> first;
Node<E> last;
int size;
```

---

## 7.1 查询

```java
list.get(100);
```

不能像数组一样随机访问，只能沿链表寻找。

双向链表通常会优化：

```text
index < size / 2
→ 从头找

否则
→ 从尾找
```

因此随机访问复杂度：

```text
O(n)
```

---

## 7.2 插入与删除

如果已经拿到节点位置，修改链表结构只需要修改前后指针：

```text
O(1)
```

但如果是：

```java
list.remove(100);
```

仍然需要先找到第 100 个节点，因此整体通常还是：

```text
O(n)
```

所以“LinkedList 插入删除一定是 O(1)”是不严谨的。

---

# 8. ArrayList 与 LinkedList 对比

| 对比项 | ArrayList | LinkedList |
|---|---|---|
| 底层 | 动态数组 | 双向链表 |
| 随机访问 | `O(1)` | `O(n)` |
| 尾部添加 | 平均 `O(1)` | `O(1)` |
| 中间插入 | `O(n)` | 定位通常 `O(n)` |
| 中间删除 | `O(n)` | 定位通常 `O(n)` |
| 内存占用 | 较小 | 较大 |
| 缓存友好性 | 好 | 较差 |

实际开发中，大多数场景优先使用：

```java
ArrayList
```

---

# 9. `List.of()`

Java 9 以后可以：

```java
List<String> list = List.of("A", "B", "C");
```

它返回的是：

> 不可修改的 List。

可以读取：

```java
list.get(0);
list.size();
```

不能修改：

```java
list.set(0, "X");    // 错误
list.add("D");       // 错误
list.remove("A");    // 错误
```

学习时可以近似理解成：

```text
“只读数组”
```

但严格来说它不是数组，而是：

```text
不可修改 List
```

另外：

```java
List.of("A", null);
```

不允许包含 `null`。

如果想得到可修改 List：

```java
List<String> list = new ArrayList<>(List.of("A", "B", "C"));
```

---

# 10. 如何重写 `equals()`

`equals()` 用来定义：

> 两个对象在业务意义上什么时候算相等。

例如：

```java
class Student {
    private String name;
    private int age;

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        Student other = (Student) o;

        return age == other.age
                && Objects.equals(name, other.name);
    }
}
```

---

## 10.1 标准步骤

### 第一步：判断是不是同一个对象

```java
if (this == o) {
    return true;
}
```

### 第二步：排除 `null` 和不同类型

```java
if (o == null || getClass() != o.getClass()) {
    return false;
}
```

### 第三步：强转

```java
Student other = (Student) o;
```

### 第四步：比较字段

基本类型：

```java
age == other.age
```

引用类型：

```java
Objects.equals(name, other.name)
```

推荐 `Objects.equals()`，因为可以安全处理 `null`。

---

# 11. 为什么重写 equals 必须重写 hashCode

通常重写 `equals()` 时，也必须重写：

```java
hashCode()
```

例如：

```java
@Override
public int hashCode() {
    return Objects.hash(name, age);
}
```

完整写法：

```java
class Student {
    private String name;
    private int age;

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        Student other = (Student) o;

        return age == other.age
                && Objects.equals(name, other.name);
    }

    @Override
    public int hashCode() {
        return Objects.hash(name, age);
    }
}
```

必须满足：

```text
如果 a.equals(b) == true
那么 a.hashCode() 必须 == b.hashCode()
```

这是因为：

```java
HashMap
HashSet
```

等哈希集合同时依赖 `equals()` 和 `hashCode()`。

---

# 12. equals 的基本规则

`equals()` 一般需要满足：

1. 自反性：`x.equals(x)` 为 `true`
2. 对称性：`x.equals(y)` 与 `y.equals(x)` 结果一致
3. 传递性：`x == y`、`y == z`，则 `x == z`
4. 一致性：对象未修改时，多次调用结果一致
5. 对 `null` 返回 `false`

---

# 13. 本次学习重点总结

## 泛型

```text
Integer extends Number
```

不代表：

```text
Pair<Integer> extends Pair<Number>
```

使用：

```java
Pair<? extends Number>
```

可以匹配 `Number` 及其子类对应的泛型类型。

## 泛型 + 反射

重点掌握：

```java
Type
ParameterizedType
getGenericType()
getGenericReturnType()
getGenericParameterTypes()
getGenericSuperclass()
```

## List

重点掌握：

```text
ArrayList：动态数组
LinkedList：双向链表
```

以及复杂度差异。

## `List.of()`

```text
不可修改 List
```

可以近似理解为“只读数组”，但正式表述不要称为数组。

## equals / hashCode

记住标准模板：

```java
@Override
public boolean equals(Object o) {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    Xxx other = (Xxx) o;

    return 字段比较;
}

@Override
public int hashCode() {
    return Objects.hash(参与 equals 的字段);
}
```

