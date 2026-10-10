# 35 · 工程实践（Git · 调试 · Spike）

> 一句话概括：版本控制管代码流转（add→commit→push），调试器管缺陷定位（断点+F10/F11），Spike 管技术不确定性（时间盒内用最小成本换答案）。

## 一、Git 核心流程

```text
工作区 --git add--> 暂存区 --git commit--> 本地仓库 --git push--> 远程仓库
```

- `git add .` 是暂存不是提交；`git commit -m "..."` 形成本地 commit；`git push` 才上远程。`git status` 看状态，`git log --oneline` 看历史。
- 关联远程：`git remote add origin https://github.com/用户名/仓库名.git`，首次 `git push -u origin main`（`-u` 建立跟踪，之后直接 `git push`）。
- **仓库根目录**：目录下有 `.git/` 就是仓库根。Git 从当前目录**向上**找 `.git`，不向下找——在子目录外执行会报 `fatal: not a git repository`。用 `git rev-parse --show-toplevel` 确认根。**避免嵌套仓库**：别在已有 `.git` 的子目录外再 `git init`。
- 平台：GitHub 首选（学习/展示/开源），GitLab 偏 DevOps，Gitee 国内访问快。GitLens 是 VS Code 的 Git 增强插件（行级历史、代码追责），辅助工具边用边熟即可。

## 二、VS Code 调试

核心五件套：**断点、F5、F10、VARIABLES、CALL STACK**。

| 快捷键 | 含义 |
|---|---|
| F5 | 继续运行到下一个断点 |
| F10 | 步过——执行当前行，不进方法 |
| F11 | 步入——进入方法内部 |
| Shift+F11 | 步出 |
| Shift+F5 | 停止 |

WATCH 持续观察表达式（`list.size()` 等）；条件断点让大循环只在 `i == 7823` 时停；异常断点在抛出某类异常（如 NPE）时立即暂停。CALL STACK 回答"代码是从哪一路调用过来的"——Java 后端典型：`Controller → Service → ServiceImpl → Mapper`。推荐流程：定位大概位置 → 打断点 → F5 → 看 VARIABLES → F10 逐行 → 可疑处 F11 → 看 CALL STACK → 找第一个异常状态。

## 三、Spike：技术预研实验

**Spike = 正式开发前的时间受限技术侦察**，目的是"能不能做、怎么做、风险是什么"，不是交付功能。与正式开发的区别：代码可以简陋、可丢弃、不要求生产质量。要点：

1. **时间盒**（Time Box）：2 小时/半天/一天，不无限研究。
2. **只打关键未知点**：想验证"DataChannel 能否传 5GB 文件"，就只做这个，不先写用户系统。
3. **最低成本消除最大不确定性**：几十到几百行最小 Demo 足矣。

完整 Spike 模板：背景 → 不确定点 → 验证目标 → 时间盒 → 实验方案 → 测试结果 → 结论 → 后续决策。

## 四、AI 时代的后端学习法

知识分三层：**必须理解**（数据库、网络、并发、JVM、操作系统、架构）、**会使用**（Spring Boot、MyBatis、Redis、MQ、Git、Linux）、**随用随查**（API 参数、注解细节、配置字段）。AI 擅长第三层和模板代码，替代不了第一层。工作方式：**自己先设计 → 自己先尝试 → AI 补充 → 读 AI 的代码 → Debug → 确认自己能解释**。

## 五、VS Code 插件与 REST Client

两个必装扩展包：**Extension Pack for Java**（语言支持/调试/JUnit/Maven）和 **Spring Boot Extension Pack**（Spring 工具/Initializr/Dashboard）。按需再加：Database Client（连 MySQL）、REST Client（在 `.http` 文件里直接发请求）、Docker。学数据库直接用 DBeaver（独立软件）。原则：**只装用得上的，别靠插件替代手写代码**。VS Code 轻量够用，项目复杂了再考虑 IDEA。

REST Client 示例 `api.http`：

```http
### 查询用户
GET http://localhost:8080/users/1

### 创建用户
POST http://localhost:8080/users
Content-Type: application/json

{ "name": "Alice" }
```

启动后端后点"发送请求"即可测接口——比切到浏览器/Postman 更顺。

## 📌 易错点

- `git add` 完以为已提交/已推送——三个阶段别混。
- 在错误目录执行 Git 命令——先确认仓库根。
- 调试只会 F5 不会 F10/F11——逐行+步入才是定位利器。
- Spike 写成完整系统——违背"最低成本换答案"的初衷。

## 🧠 速记口诀

```text
add 暂存、commit 本地、push 远程
F10 看流程、F11 看细节、CALL STACK 看来路
Spike：时间盒内最小 Demo，只验证最大的不确定
AI 补代码，你补理解——能解释才算学会
```

## ✅ 自测问题

1. `git add`、`git commit`、`git push` 各做什么？
2. 为什么在 `E:\codex\l4` 会报 `not a git repository`？如何确认仓库根？
3. F10 和 F11 的区别？CALL STACK 解决什么问题？
4. Spike 和正式开发的区别？一个完整 Spike 应包含哪些部分？
5. AI 时代哪些后端知识"必须理解"，哪些可以"随用随查"？

> 来源：原始《学习笔记：Git、VS Code、MySQL 与 Java 后端基础》(本次聊天学习笔记_Git_VSCode_MySQL_Java.md §1-11/§23，MySQL 底层链路部分并入 `32-MySQL基础.md`、bind/listen/accept 部分并入 `18-网络编程基础.md`)、《Spike 学习笔记》(Spike学习笔记.md)。
