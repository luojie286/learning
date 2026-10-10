# 33 · Maven 与依赖管理

> 一句话概括：Maven 是 Java 项目的依赖管理 + 构建 + 结构约定工具——在 pom.xml 里声明"坐标"，它自动下载 jar、管理传递依赖、统一编译测试打包流程。

## 一、解决什么问题

不用 Maven：手动下载 jar → 拷进项目 → 配 classpath → 处理 jar 间依赖 → 换电脑重来。用 Maven：pom.xml 里写一个 `<dependency>`，它自动"读坐标 → 查本地仓库 → 没有就从远程仓库（Maven Central）下载 → 连带下载传递依赖 → 加入 classpath"。

**约定优于配置（Convention over Configuration）**——标准目录固定：

```text
my-project/
├── pom.xml
└── src/
    ├── main/java       # 源代码
    ├── main/resources  # 配置与资源
    └── test/java       # 测试代码
```

编译产物在 `target/`。

## 二、pom.xml 核心概念

- **坐标**：`groupId`（组织）+ `artifactId`（项目/模块）+ `version`（版本）三者定位一个构件。
- **仓库**：远程仓库 → 本地仓库（默认 `C:\Users\用户名\.m2\repository`）→ 项目。
- **传递依赖**：A 依赖 B、B 依赖 C → C 自动进来。Starter 方便就是因为一个入口间接引入大量组件。
- **依赖冲突**：两条路径引入同一构件的不同版本 → 用 `mvn dependency:tree` 排查，用 `<exclusions>` 排除。
- **scope**：`compile`（默认，全程需要）/ `test`（仅测试，如 JUnit）/ `runtime`（运行时需要）/ `provided`（编译需要，运行环境提供）。
- **properties**：集中管理版本号、编译级别（`maven.compiler.source/target`）。
- **parent POM**：如 `spring-boot-starter-parent` 统一提供依赖与插件版本。
- **dependencies vs dependencyManagement**：前者真正引入依赖；后者只管统一版本，不自动引入，子模块仍需自己声明。
- **settings.xml**：环境级配置（本地仓库位置、镜像、私服、代理），在 Maven 安装目录或 `~/.m2/`。

## 三、生命周期、命令与插件

生命周期主线：`validate → compile → test → package → verify → install → deploy`。常用命令：

```bash
mvn compile    # .java → .class（target/classes/）
mvn test       # 先编译再跑测试
mvn package    # 编译+测试+打包 → target/*.jar
mvn install    # 再装到本地仓库，供其他项目引用
mvn clean      # 删 target/
mvn dependency:tree
```

**Maven 指挥流程，插件真正干活**：编译用 `maven-compiler-plugin`、测试用 `maven-surefire-plugin`、打 jar 用 `maven-jar-plugin`。`mvn dependency:tree` = dependency 插件 + tree Goal。关系：`Lifecycle → Phase → Plugin → Goal`。

## 四、与其他工具的关系

- **Maven vs import**：Maven 让依赖"存在"（下载并加入 classpath）；import 只是告诉编译器类的全限定名——import 的类必须有对应 jar 在 classpath，两者配合。
- **Maven vs Spring Boot**：Maven 是构建/依赖工具，Spring Boot 是框架；starter 依赖靠 Maven 传递引入。
- **是否必须安装**：用 `mvn` 命令需要装并配 PATH（Windows 下载 `-bin.zip`，把 `bin` 加进 Path，`mvn -v` 验证）；项目带 `mvnw.cmd`（Maven Wrapper）则免装；IDEA 自带 Maven。无论哪种都需要 JDK——Maven 本身跑在 JVM 上。

## 📌 易错点

- `dependencyManagement` 写了不等于引入了依赖。
- 依赖冲突靠 `dependency:tree` 看，别瞎猜。
- `mvn package` 会先跑测试，测试不过打不了包。
- 连接池等运行时才用的驱动，scope 用 `runtime`。
- 配了镜像/私服但 settings.xml 位置不对，等于没配。

## 🧠 速记口诀

```text
坐标三段：groupId、artifactId、version
本地仓库 ~/.m2，没有就去 Central 拉
compile test package install，一步一步往后走
依赖树 dependency:tree，冲突 exclusion 排
Maven 拉包，import 引用，各司其职
```

## ✅ 自测问题

1. Maven 坐标是什么？三者各表示什么？
2. 什么是传递依赖？Starter 为什么方便？
3. `mvn compile/test/package/install` 的区别？
4. `dependencies` 和 `dependencyManagement` 有何不同？
5. 项目有 `mvnw.cmd` 时还需要全局安装 Maven 吗？

> 来源：原始《Maven 与 Java 后端学习顺序笔记》(Maven与Java后端学习顺序笔记.md，其中"Java 后端学习顺序/Nginx 学习时机"部分已并入各主题与根 README 学习路径)。
