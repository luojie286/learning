# 学习笔记：Git、VS Code、MySQL 与 Java 后端基础

> 整理自本次聊天  
> 重点：Git 基础操作、VS Code 调试、MySQL 连接机制、JDBC/驱动、端口与 TCP 服务端流程

---

## 1. GitLens 是什么

GitLens 是 VS Code 中的 Git 增强插件。

它不会替代 Git，而是把 Git 的历史信息、提交信息、代码作者、分支关系等更直观地展示出来。

常见用途：

- 查看某一行代码是谁修改的
- 查看某个文件的提交历史
- 查看某一行的历史
- 查看 commit 改了哪些内容
- 比较两个版本的代码
- 查看分支和提交图

核心关系：

```text
Git
↓
真正负责版本控制

VS Code 自带 Git
↓
提供基础图形界面

GitLens
↓
增强历史追踪、提交查看、代码追责等能力
```

GitLens 属于辅助工具，不需要专门投入大量时间学习，边做项目边熟悉即可。

---

## 2. VS Code 中创建本地 Git 仓库

### 2.1 图形界面方式

打开项目文件夹后：

```text
源代码管理
→ Initialize Repository
```

本质上就是：

```bash
git init
```

执行后，项目目录中会生成隐藏目录：

```text
.git/
```

只要某个目录下存在 `.git`，它就是 Git 仓库根目录。

---

### 2.2 命令行方式

进入项目目录：

```bash
git init
```

检查状态：

```bash
git status
```

如果看到：

```text
On branch main
```

说明当前目录是 Git 仓库。

如果看到：

```text
fatal: not a git repository
```

说明当前目录及其父目录中都找不到 `.git`。

---

## 3. Git 的三个核心阶段

这是本次聊天最重要的 Git 基础。

```text
工作区
↓ git add
暂存区
↓ git commit
本地仓库
↓ git push
远程仓库
```

### 3.1 `git add`

```bash
git add .
```

作用：

> 把工作区中的修改放入暂存区。

它不等于提交。

VS Code 中通常不直接显示 `git add`，而显示：

```text
暂存更改
```

或者使用文件右侧的：

```text
+
```

点击 `+` 相当于：

```bash
git add 文件名
```

点击“更改”区域整体的 `+`，相当于：

```bash
git add .
```

---

### 3.2 `git commit`

```bash
git commit -m "initial commit"
```

作用：

> 把暂存区内容保存到本地 Git 仓库，形成一个 commit。

如果 `git status` 显示：

```text
No commits yet
Changes to be committed:
```

说明：

- `git add` 已经完成
- 但还没有 `git commit`

提交成功后通常可以看到：

```text
nothing to commit, working tree clean
```

---

### 3.3 判断是否提交成功

推荐：

```bash
git status
```

如果看到：

```text
nothing to commit, working tree clean
```

说明当前没有未提交修改。

再看：

```bash
git log --oneline
```

如果能看到：

```text
a8c21f3 initial commit
```

说明已经产生 commit。

---

## 4. 本地仓库与远程仓库

### 4.1 推荐远程仓库

对于 Java 后端学习、项目展示、以后参与开源：

> 首选 GitHub。

其他常见平台：

- GitLab：偏 DevOps、CI/CD、自托管
- Gitee：国内访问更方便

---

### 4.2 关联远程仓库

在 Git 仓库根目录执行：

```bash
git remote add origin https://github.com/用户名/仓库名.git
```

检查：

```bash
git remote -v
```

例如：

```text
origin  https://github.com/xxx/repo.git (fetch)
origin  https://github.com/xxx/repo.git (push)
```

---

### 4.3 推送到远程

第一次：

```bash
git push -u origin main
```

之后通常：

```bash
git push
```

其中：

```text
origin
```

是远程仓库别名。

```text
main
```

是本地分支名。

```text
-u
```

用于建立跟踪关系。

---

## 5. Git 仓库根目录问题

曾出现：

```text
PS E:\codex\l4> git status
fatal: not a git repository
```

原因是实际 `.git` 位于：

```text
E:\codex\l4\sky-take-out\.git
```

而当前终端在：

```text
E:\codex\l4
```

Git 会从当前目录向上寻找 `.git`，不会向下寻找。

所以：

```bash
cd E:\codex\l4\sky-take-out
```

后再运行 Git 命令即可。

---

## 6. 把 `l4` 改成仓库根目录

原结构：

```text
E:\codex\l4\
└── sky-take-out\
    └── .git\
```

目标：

```text
E:\codex\l4\
├── .git\
└── sky-take-out\
```

注意：

> 不建议保留 `sky-take-out/.git` 后再在 `l4` 运行 `git init`，否则会形成嵌套 Git 仓库。

正确思路：

1. 先移走或备份子目录中的 `.git`
2. 在 `l4` 执行 `git init`
3. 重新 `git add`
4. 重新 commit
5. 重新关联远程仓库

检查仓库根目录：

```bash
git rev-parse --show-toplevel
```

理想结果：

```text
E:/codex/l4
```

---

# 7. MySQL 图形界面工具

推荐：

> DBeaver

原因：

- 免费
- 支持 MySQL、PostgreSQL、SQLite 等多种数据库
- 界面直观
- 适合 Java 后端学习
- 后期不会局限在 MySQL

其他：

| 工具 | 特点 |
|---|---|
| DBeaver | 推荐，免费、通用 |
| DataGrip | JetBrains 生态体验好 |
| MySQL Workbench | MySQL 官方工具 |
| Navicat | 好用，但正版收费 |
| VS Code 插件 | 适合临时查询，不建议当主力 |

学习阶段推荐组合：

```text
MySQL 命令行
+
DBeaver
```

图形界面负责：

- 看表
- 看字段
- 看数据
- 看索引
- 执行 SQL

但 SQL 本身仍然要会手写。

---

## 8. DBeaver 安装选项

Windows 普通 Intel/AMD 电脑：

```text
Windows
→ x86
→ EXE
```

安装范围：

```text
For me
```

即可。

数据库导航模式：

```text
Full
```

适合学习完整数据库结构。

匿名统计：

```text
Do not share anonymous usage statistics
```

不影响功能。

示例数据库和每日提示：

```text
都可以不勾
```

---

# 9. VS Code 编译 Java

VS Code 本身不是 Java 编译器。

真正负责 Java 编译的是：

```text
JDK 中的 javac
```

例如：

```bash
javac Hello.java
```

会生成：

```text
Hello.class
```

运行：

```bash
java Hello
```

流程：

```text
Hello.java
↓ javac
Hello.class
↓ java
JVM
```

---

## 10. Maven 项目编译

对于 Maven 项目，一般不逐个编译 Java 文件。

编译：

```bash
mvn compile
```

打包：

```bash
mvn package
```

编译后的 `.class` 通常在：

```text
target/classes/
```

---

# 11. VS Code 调试核心技巧

最重要的是：

```text
断点
F5
F10
F11
VARIABLES
CALL STACK
```

## 11.1 断点

点击代码左侧行号，可以设置红色断点。

程序以 Debug 模式运行后，到断点位置会暂停。

## 11.2 常用快捷键

| 操作 | 快捷键 | 含义 |
|---|---|---|
| Continue | F5 | 继续运行到下一个断点 |
| Step Over | F10 | 执行当前行，不进入方法 |
| Step Into | F11 | 进入当前方法 |
| Step Out | Shift + F11 | 从当前方法返回 |
| Stop | Shift + F5 | 停止调试 |

重点：

```text
F10
= 我不关心这个方法内部

F11
= 我要进入这个方法内部看
```

## 11.3 VARIABLES

调试暂停后，可以直接查看当前变量值。

## 11.4 WATCH

用于持续观察表达式，例如：

```text
list.size()
user.getId()
map.get("abc")
i < nums.length
```

## 11.5 CALL STACK

用于查看当前代码是从哪里一路调用过来的。

Java 后端常见：

```text
Controller
↓
Service
↓
ServiceImpl
↓
Mapper
```

## 11.6 条件断点

例如：

```java
for (int i = 0; i < 10000; i++) {
    process(i);
}
```

只想在：

```text
i == 7823
```

时停下，可以设置条件断点。

## 11.7 异常断点

可以设置：

> 一旦抛出某类异常，立即暂停。

例如：

```text
NullPointerException
```

## 11.8 调试的正确思路

推荐流程：

```text
1. 找大概错误位置
2. 打断点
3. F5 启动
4. 看 VARIABLES
5. F10 逐行执行
6. 遇到可疑方法用 F11
7. 看 CALL STACK
8. 找第一个异常状态
```

---

# 12. Java 中怎么写 SQL

Java 中常见方式：

```text
JDBC
MyBatis 注解
MyBatis XML
```

## 12.1 JDBC

SQL 本质上通常是 Java 字符串：

```java
String sql = "SELECT * FROM user WHERE id = ?";
```

执行：

```java
PreparedStatement ps = connection.prepareStatement(sql);
ps.setLong(1, 1L);
ResultSet rs = ps.executeQuery();
```

不要直接拼接用户输入，推荐使用参数绑定。

## 12.2 MyBatis 注解

查询：

```java
@Select("SELECT * FROM user WHERE id = #{id}")
User getById(Long id);
```

插入：

```java
@Insert("INSERT INTO user(username, password) VALUES(#{username}, #{password})")
void insert(User user);
```

更新：

```java
@Update("UPDATE user SET username = #{username} WHERE id = #{id}")
void update(User user);
```

删除：

```java
@Delete("DELETE FROM user WHERE id = #{id}")
void deleteById(Long id);
```

## 12.3 MyBatis XML

```xml
<select id="getById" resultType="com.example.User">
    SELECT *
    FROM user
    WHERE id = #{id}
</select>
```

---

# 13. JDBC、MyBatis 与数据库的关系

整体：

```text
Java
↓
MyBatis
↓
JDBC
↓
数据库驱动
↓
MySQL
```

MyBatis 并没有绕开 JDBC，而是帮你简化：

- PreparedStatement 创建
- 参数绑定
- ResultSet 解析
- Java 对象映射
- 大量模板代码

---

# 14. 数据库驱动是什么

数据库驱动可以理解成：

> Java 与具体数据库之间的适配器 + 通信实现。

例如：

```text
Java
↓
JDBC API
↓
MySQL Driver
↓
MySQL Protocol
↓
TCP
↓
MySQL Server
```

JDBC 是统一规范。

MySQL Connector/J 是 MySQL 对 JDBC 的具体实现。

MySQL Maven 依赖示例：

```xml
<dependency>
    <groupId>com.mysql</groupId>
    <artifactId>mysql-connector-j</artifactId>
</dependency>
```

---

# 15. 驱动是不是协议层

不能简单说：

```text
数据库驱动 = 协议层
```

更准确：

> 驱动是应用程序中的一个库，它内部实现并使用数据库应用层协议。

```text
MySQL Driver
↓
实现 MySQL Protocol
↓
通过 TCP 通信
```

核心记忆：

> 协议是规则，驱动是实现这些规则的代码。

---

# 16. MySQL 为什么需要端口

IP 只能定位到哪台机器。

端口用来定位这台机器上的哪个网络服务。

例如：

```text
192.168.1.10:3306 → MySQL
192.168.1.10:8080 → Spring Boot
192.168.1.10:6379 → Redis
```

所以：

```text
IP 找机器
端口找服务/进程
```

JDBC URL：

```text
jdbc:mysql://localhost:3306/java_app
```

拆开：

```text
localhost
→ 哪台机器

3306
→ 哪个服务

java_app
→ MySQL 中的哪个数据库
```

---

# 17. MySQL 为什么需要监听端口

MySQL Server 是服务器程序，需要等待 Java、DBeaver 等客户端连接。

所以会监听：

```text
3306
```

大致过程：

```text
MySQL 启动
↓
绑定 3306
↓
监听 3306
↓
等待客户端连接
```

监听的含义：

> 告诉操作系统：如果有客户端连接这个端口，把连接请求交给我。

---

# 18. bind 与 listen 的区别

服务端底层典型流程：

```text
socket()
↓
bind()
↓
listen()
↓
accept()
```

### bind

> 把 socket 绑定到某个本地 IP + 端口。

例如：

```text
127.0.0.1:3306
0.0.0.0:3306
```

可以记成：

```text
bind = 我占用哪个地址
```

### listen

> 把 socket 变成监听 socket，开始等待客户端建立 TCP 连接。

```text
listen = 我开始等客户端
```

### accept

> 接收一个具体的客户端连接。

```text
accept = 接到一个具体客户端
```

---

# 19. 只 bind 不 listen 能不能接 TCP 数据

对于 TCP 服务端，仅 bind 还不够。

完整流程：

```text
socket
↓
bind
↓
listen
↓
accept
↓
read/write
```

TCP 数据不是直接“扔到端口”，而是：

```text
先建立 TCP 连接
↓
再通过连接发送业务数据
```

服务端存在两类 socket：

```text
监听 socket
↓
负责等待新连接

已连接 socket
↓
负责和某个客户端 read/write
```

Java 中：

```java
ServerSocket serverSocket = new ServerSocket(8080);
Socket socket = serverSocket.accept();
InputStream in = socket.getInputStream();
```

---

# 20. TCP 与 UDP 的区别之一

TCP 服务端：

```text
bind
↓
listen
↓
accept
↓
read/write
```

UDP：

```text
bind
↓
recvfrom/sendto
```

UDP 不需要 `listen` 和 `accept`。

---

# 21. MySQL 查询的底层完整链路

例如 Java 查询：

```java
User user = userMapper.getById(10L);
```

SQL：

```sql
SELECT id, username, age
FROM user
WHERE id = 10;
```

整体链路：

```text
Java
↓
MyBatis
↓
JDBC
↓
MySQL Driver
↓
TCP
↓
MySQL Server
↓
SQL 解析
↓
优化器
↓
InnoDB
↓
Buffer Pool
↓
B+Tree 索引
↓
磁盘
↓
结果返回
↓
Java 对象
```

## 21.1 SQL 解析

包括：

```text
词法分析
语法分析
语义检查
权限检查
```

## 21.2 优化器

SQL 描述：

> 我要什么。

优化器决定：

> 怎么拿。

例如主键查询可能使用 PRIMARY 索引，而不是全表扫描。

## 21.3 InnoDB

粗略理解：

```text
MySQL Server
负责理解和优化 SQL

InnoDB
负责真正读写数据
```

## 21.4 Buffer Pool

InnoDB 会优先从内存中的 Buffer Pool 读取数据页。

命中：

```text
直接从内存读
```

未命中：

```text
磁盘读取
↓
放入 Buffer Pool
↓
再读取
```

## 21.5 B+Tree 索引

主键索引可以快速定位数据。

```text
id = 10
↓
B+Tree
↓
定位叶子节点
↓
得到整行数据
```

## 21.6 返回 Java

```text
InnoDB
↓
MySQL Server
↓
MySQL Protocol
↓
TCP
↓
MySQL Driver
↓
JDBC ResultSet
↓
MyBatis
↓
User 对象
```

---

# 22. UPDATE 的底层过程

例如：

```sql
UPDATE user
SET age = 21
WHERE id = 10;
```

大致流程：

```text
Java
↓
MyBatis
↓
JDBC
↓
Driver
↓
TCP
↓
MySQL
↓
解析
↓
优化
↓
InnoDB
↓
B+Tree 找记录
↓
Buffer Pool 修改数据页
↓
Undo Log
↓
Redo Log
↓
COMMIT
↓
后续把脏页刷回磁盘
```

## 22.1 Undo Log

主要用于：

```text
事务回滚
MVCC
```

可以粗略理解为保存修改前的数据版本。

## 22.2 Redo Log

主要用于：

```text
持久性
崩溃恢复
```

用于在崩溃后恢复已经提交但可能还没有完整刷入数据文件的修改。

---

# 23. AI 时代后端知识怎么学

结论：

> 传统后端核心知识仍然有价值，但学习方式应该改变。

建议分三层。

## 第一层：必须理解

```text
数据库
网络
并发
JVM
操作系统
后端架构
```

## 第二层：会使用

```text
Spring Boot
MyBatis
Redis
MQ
Git
Linux
```

## 第三层：随用随查

```text
API 参数
注解细节
配置字段
IDE 按钮
第三方 SDK
冷门框架功能
```

AI 很适合：

- 查 API
- 写模板代码
- 补 CRUD
- 查配置
- 生成重复逻辑
- 辅助 Debug

更合理的工作方式：

```text
自己先设计
↓
自己先尝试
↓
AI 补充
↓
阅读 AI 代码
↓
Debug
↓
确认自己能解释
```

---

# 24. 本次学习主线

```text
Git
→ 理解版本控制流程

VS Code
→ 学会编译与 Debug

Java
→ 通过 JDBC / MyBatis 发 SQL

JDBC
→ 统一数据库访问接口

Driver
→ 实现具体数据库通信

TCP
→ Java 与 MySQL 之间通信

IP + Port
→ 找到 MySQL 服务

bind/listen/accept
→ 服务端接受连接

MySQL Server
→ 解析 SQL、优化 SQL

InnoDB
→ 真正读写数据

Buffer Pool
→ 内存缓存

B+Tree
→ 快速定位数据

Undo/Redo Log
→ 保证事务与恢复
```

---

# 25. 推荐复习重点

1. Git 的 `add → commit → push`
2. VS Code 的 `断点 + F10 + F11 + VARIABLES + CALL STACK`
3. JDBC / MyBatis / Driver / MySQL 的关系
4. IP、端口、bind、listen、accept 的区别
5. 一次 SELECT 从 Java 到 MySQL 再返回的完整链路
6. Buffer Pool 与 B+Tree 的基本作用
7. Undo Log / Redo Log 的基本区别

---

# 26. 自测题

### Git

1. `git add` 和 `git commit` 的区别是什么？
2. `git commit` 和 `git push` 的区别是什么？
3. 为什么在 `E:\codex\l4` 中可能提示 `not a git repository`？
4. `.git` 目录有什么作用？
5. `git remote add origin ...` 中的 `origin` 是什么？

### VS Code

6. F10 和 F11 有什么区别？
7. VARIABLES 和 WATCH 各适合做什么？
8. CALL STACK 能解决什么问题？
9. 为什么条件断点比普通断点更适合调试大循环？

### JDBC / MySQL

10. JDBC 是什么？
11. 数据库驱动是什么？
12. 为什么说驱动不是协议本身？
13. MyBatis 有没有绕过 JDBC？
14. Java 的 SQL 最终是谁执行的？

### 网络

15. 为什么 MySQL 要监听 3306？
16. IP 和端口分别解决什么问题？
17. `bind` 和 `listen` 的区别是什么？
18. 为什么 TCP 服务端不能只 `bind`？
19. `accept` 返回的 socket 和监听 socket 有什么区别？
20. TCP 和 UDP 服务端流程有什么不同？

### MySQL 底层

21. 一次 SELECT 从 Java 到 MySQL 的完整链路是什么？
22. 优化器负责什么？
23. InnoDB 负责什么？
24. Buffer Pool 为什么存在？
25. B+Tree 索引为什么能加速查询？
26. Undo Log 和 Redo Log 分别解决什么问题？

---

# 27. 最后压缩成几句话

```text
Git：
add 是暂存，commit 是本地提交，push 是远程推送。

VS Code Debug：
断点 + F10/F11 + VARIABLES + CALL STACK 是核心。

Java 访问 MySQL：
Java → MyBatis → JDBC → Driver → TCP → MySQL。

网络：
IP 找机器，端口找服务。
bind 占地址，listen 等连接，accept 接客户端。

MySQL 查询：
解析 SQL → 优化 → InnoDB → Buffer Pool / B+Tree → 返回结果。

事务：
Undo Log 更偏回滚/MVCC，
Redo Log 更偏持久性/崩溃恢复。
```
