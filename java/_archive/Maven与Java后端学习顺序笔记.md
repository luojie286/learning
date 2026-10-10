# Maven 与 Java 后端学习顺序笔记

## 一、Maven 是什么

Maven 可以理解为 Java 项目的：

- 依赖管理工具
- 项目构建工具
- 项目结构与配置管理工具

它最核心解决两个问题：

1. 自动管理第三方依赖
2. 统一 Java 项目的编译、测试、打包流程

---

## 二、Maven 解决的依赖管理问题

如果不使用 Maven，Java 项目需要手动：

1. 下载 jar 包
2. 放入项目
3. 配置 classpath
4. 处理 jar 包之间的依赖关系
5. 换电脑后重新配置

使用 Maven 后，只需要在 `pom.xml` 中声明依赖，例如：

```xml
<dependency>
    <groupId>com.mysql</groupId>
    <artifactId>mysql-connector-j</artifactId>
    <version>...</version>
</dependency>
```

Maven 会自动：

```text
读取依赖坐标
↓
检查本地仓库
↓
如果没有则从远程仓库下载
↓
下载该依赖的传递依赖
↓
加入项目 classpath
```

---

## 三、Maven 标准项目结构

典型 Maven 项目：

```text
my-project/
├── pom.xml
└── src/
    ├── main/
    │   ├── java/
    │   └── resources/
    └── test/
        ├── java/
        └── resources/
```

主要目录：

- `src/main/java`：Java 源代码
- `src/main/resources`：配置文件、资源文件
- `src/test/java`：测试代码
- `pom.xml`：Maven 核心配置
- `target/`：编译、测试、打包后的输出目录

Maven 强调：

> Convention over Configuration，约定优于配置。

---

## 四、pom.xml

POM 全称：

```text
Project Object Model
```

最基础的 Maven 坐标：

```xml
<groupId>com.example</groupId>
<artifactId>demo</artifactId>
<version>1.0-SNAPSHOT</version>
```

三者作用：

```text
groupId
→ 组织、公司、项目组

artifactId
→ 具体项目或模块

version
→ 版本
```

三者组合后可以定位一个 Maven 构件。

---

## 五、dependencies

项目依赖一般放在：

```xml
<dependencies>
    <dependency>
        ...
    </dependency>
</dependencies>
```

一个 `<dependency>` 表示引入一个依赖。

---

## 六、Maven 仓库

Maven 主要涉及：

```text
远程仓库
↓
本地仓库
↓
当前项目
```

Windows 默认本地仓库一般位于：

```text
C:\Users\用户名\.m2\repository
```

如果本地没有某个依赖，Maven 会从远程仓库下载，然后缓存到本地仓库。

常见远程仓库：

```text
Maven Central
```

---

## 七、传递依赖

假设：

```text
项目 A
↓
依赖 B
↓
B 又依赖 C
```

那么 Maven 通常会自动把 C 也加入项目。

这叫：

> Transitive Dependency，传递依赖。

例如 Spring Boot Starter 之所以方便，就是因为一个 Starter 可以间接引入大量实际需要的依赖。

---

## 八、依赖冲突

例如：

```text
项目
├── A
│   └── C 1.0
└── B
    └── C 2.0
```

就会出现依赖版本冲突。

常用排查命令：

```bash
mvn dependency:tree
```

它可以查看项目完整的依赖树。

还可以使用：

```xml
<exclusions>
    <exclusion>
        ...
    </exclusion>
</exclusions>
```

排除某些传递依赖。

---

## 九、dependency scope

常见 scope：

```text
compile
test
runtime
provided
```

### compile

默认值。

通常表示：

```text
编译需要
测试需要
运行需要
```

### test

只在测试阶段使用。

例如 JUnit：

```xml
<scope>test</scope>
```

### runtime

编译阶段不一定需要，但程序运行时需要。

### provided

编译时需要，但预计运行环境会提供。

---

## 十、Maven 生命周期

Maven 构建有一套生命周期。

常见阶段：

```text
validate
↓
compile
↓
test
↓
package
↓
verify
↓
install
↓
deploy
```

现阶段最重要的是：

```text
compile
test
package
install
```

---

## 十一、常用 Maven 命令

### 编译

```bash
mvn compile
```

将：

```text
.java
↓
.class
```

生成结果一般在：

```text
target/classes/
```

### 测试

```bash
mvn test
```

会先完成编译，然后执行测试代码。

### 打包

```bash
mvn package
```

一般流程：

```text
compile
↓
test
↓
package
```

最终生成：

```text
target/*.jar
```

### 安装到本地仓库

```bash
mvn install
```

相当于：

```text
编译
↓
测试
↓
打包
↓
安装到 ~/.m2/repository
```

### 清理

```bash
mvn clean
```

主要删除：

```text
target/
```

常见组合：

```bash
mvn clean package
mvn clean install
```

---

## 十二、Plugin 与 Goal

Maven 本身主要负责协调构建过程，真正执行编译、测试、打包的是插件。

例如：

```text
编译
→ maven-compiler-plugin

测试
→ maven-surefire-plugin

打 jar
→ maven-jar-plugin
```

可以理解为：

```text
Maven
→ 指挥构建流程

Plugin
→ 真正执行具体任务
```

插件内部还有 Goal。

例如：

```bash
mvn dependency:tree
```

可以理解为：

```text
dependency 插件
+
tree Goal
```

核心关系：

```text
Lifecycle
↓
Phase
↓
Plugin
↓
Goal
```

---

## 十三、properties

可以在 `pom.xml` 中定义公共属性：

```xml
<properties>
    <maven.compiler.source>17</maven.compiler.source>
    <maven.compiler.target>17</maven.compiler.target>
</properties>
```

也可以保存依赖版本：

```xml
<properties>
    <junit.version>...</junit.version>
</properties>
```

使用：

```xml
<version>${junit.version}</version>
```

---

## 十四、Parent POM

Spring Boot 项目常见：

```xml
<parent>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-parent</artifactId>
    <version>...</version>
</parent>
```

父 POM 可以统一提供：

- 依赖版本
- 插件版本
- 构建配置
- 公共属性

---

## 十五、dependencies 与 dependencyManagement

### dependencies

```xml
<dependencies>
```

表示：

> 真正引入依赖。

### dependencyManagement

```xml
<dependencyManagement>
```

主要用于：

> 统一管理版本，但不会自动把依赖真正引入项目。

子模块仍然需要自己声明 dependency。

---

## 十六、Maven 多模块

大型项目常见结构：

```text
project/
├── pom.xml
├── user-module/
│   └── pom.xml
├── order-module/
│   └── pom.xml
└── common-module/
    └── pom.xml
```

父项目可以：

```xml
<modules>
    <module>user-module</module>
    <module>order-module</module>
    <module>common-module</module>
</modules>
```

用于统一管理多个 Maven 模块。

---

## 十七、settings.xml

`pom.xml` 是项目配置。

`settings.xml` 更偏向当前开发环境配置。

常见配置：

- Maven 本地仓库位置
- 镜像
- 私服
- 代理
- 认证信息

常见位置：

```text
Maven安装目录/conf/settings.xml
```

或者：

```text
C:\Users\用户名\.m2\settings.xml
```

---

## 十八、Maven 和 import 的区别

例如：

```java
import com.fasterxml.jackson.databind.ObjectMapper;
```

`import` 只负责告诉 Java 编译器：

> 这个类的完整名字是什么。

但是它要求对应 jar 已经存在于 classpath 中。

Maven 则负责：

```text
dependency
↓
下载 jar
↓
加入 classpath
```

所以：

```text
Maven
→ 让依赖存在

import
→ 在源码中引用依赖里的类
```

---

## 十九、Maven Wrapper

项目中如果出现：

```text
mvnw
mvnw.cmd
.mvn/
pom.xml
```

说明项目使用 Maven Wrapper。

Windows 可以：

```bash
.\mvnw.cmd package
```

这类项目通常不要求系统提前安装全局 Maven。

---

## 二十、Maven 是否需要安装

分情况。

### 情况 1：直接使用 mvn

如果希望：

```bash
mvn compile
mvn package
```

那么通常需要安装 Maven，并配置环境变量。

### 情况 2：使用 Maven Wrapper

如果项目自带：

```text
mvnw.cmd
```

通常不需要提前安装 Maven。

### 情况 3：IDE 自带 Maven

IntelliJ IDEA 等 IDE 可以管理自己的 Maven，因此即使系统命令行里没有 `mvn`，IDE 内也可能正常使用 Maven。

但无论哪种方式：

> Maven 都需要 JDK，因为 Maven 本身运行在 JVM 上。

---

## 二十一、Windows Maven 安装

当 CMD 输入：

```cmd
mvn -v
```

出现：

```text
'mvn' 不是内部或外部命令
```

说明：

```text
Windows 当前找不到 mvn
```

常见原因：

1. Maven 没安装
2. Maven 安装了，但 `bin` 没加入 PATH

### 下载版本

Windows 下载：

```text
Binary zip archive
```

不要下载：

```text
Source zip archive
Source tar.gz archive
```

本次选择：

```text
apache-maven-3.9.16-bin.zip
```

### 配置 PATH

假设解压到：

```text
D:\dev\apache-maven-3.9.16
```

需要将：

```text
D:\dev\apache-maven-3.9.16\bin
```

加入 Windows `Path`。

然后关闭旧 CMD，重新打开，执行：

```cmd
mvn -v
```

还可以执行：

```cmd
where mvn
```

检查系统实际调用的是哪个 Maven。

---

## 二十二、Maven 与 Spring Boot

Maven 和 Spring Boot 不是同一个东西。

```text
Spring Boot
→ Java 后端框架

Maven
→ 项目与依赖管理工具
```

例如：

```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-web</artifactId>
</dependency>
```

Maven 负责把 Spring Boot Web 所需依赖下载下来。

Spring Boot 则负责实际的 Web 开发能力。

---

## 二十三、Spring Boot Starter

Starter 可以理解为：

> 一组预先组合好的依赖。

例如：

```text
spring-boot-starter-web
↓
Spring MVC
Spring Core
Jackson
Tomcat
Logging
...
```

Maven 通过传递依赖自动把这些组件引入项目。

---

# Java 后端学习顺序

## 一、SQL 和 Spring 谁先学

建议：

```text
Java 基础
↓
Maven
↓
SQL / MySQL 基础
↓
Spring
↓
Spring Boot
↓
MyBatis / MyBatis-Plus
↓
Redis
↓
项目
```

不需要把 MySQL 全部学完才开始 Spring。

更合理的是：

```text
MySQL 基础
↓
开始 Spring
↓
Spring 与数据库继续并行深入
```

---

## 二、进入 Spring 前 MySQL 学到什么程度

至少掌握：

```text
数据库、表、行、列
主键、外键
CRUD
WHERE
ORDER BY
GROUP BY
HAVING
JOIN
聚合函数
索引基本概念
事务基本概念
```

必须能自己写：

```sql
SELECT *
FROM user
WHERE age > 18
ORDER BY age DESC;
```

以及：

```sql
SELECT u.name, o.id
FROM user u
JOIN orders o
ON u.id = o.user_id;
```

不用一开始就深入：

- Buffer Pool
- Redo Log
- Undo Log
- MySQL 内核
- 优化器底层实现

这些后面再补。

---

# Nginx 应该什么时候学

不建议现在立刻学。

更适合在：

```text
Spring Boot 基础
↓
能写完整后端项目
↓
Linux 基础
↓
服务器部署
↓
Nginx
```

之后学习。

原因是有了一个真正的 Spring Boot 服务后，Nginx 会非常直观。

例如：

```text
用户
↓
Nginx :80 / :443
↓
Spring Boot :8080
```

---

## Nginx 第一阶段需要掌握

只需要掌握：

```text
Nginx 是什么
正向代理与反向代理
server
location
proxy_pass
静态资源
upstream
简单负载均衡
HTTPS 基本配置
日志
常用启动 / 重载命令
```

示例：

```nginx
server {
    listen 80;

    location /api/ {
        proxy_pass http://127.0.0.1:8080/;
    }
}
```

含义：

```text
用户访问 Nginx :80
↓
Nginx 接收请求
↓
转发到 Spring Boot :8080
```

---

# 当前推荐路线

结合当前 Java 后端学习阶段：

```text
Maven
↓
SQL / MySQL 基础
↓
Spring IOC / DI / Bean / AOP
↓
Spring Boot
↓
MyBatis / MyBatis-Plus
↓
Redis
↓
做完整后端项目
↓
Linux 部署
↓
Nginx
↓
Docker
↓
MQ / 更深入的后端组件
```

---

# 当前阶段自检

## Maven

应该能够解释：

- Maven 是干什么的
- `pom.xml` 是什么
- dependency 是什么
- Maven 坐标是什么
- 本地仓库是什么
- 什么是传递依赖
- `mvn compile/test/package/install` 的区别
- Plugin 与 Goal 的关系
- Maven 是否一定需要全局安装
- Windows 如何配置 Maven PATH

应该亲手执行：

```bash
mvn -v
mvn compile
mvn test
mvn package
mvn dependency:tree
```

并观察：

```text
target/
.m2/repository/
```

## SQL

应该能独立完成基本 CRUD、JOIN、GROUP BY，并理解索引和事务的基本作用。

## Spring

SQL 基础掌握后即可开始，不需要等数据库高级内容全部学完。

## Nginx

暂时不需要深入，等 Spring Boot 项目真正部署服务器时再集中学习。
