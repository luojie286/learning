# 01 · 第一个 CMakeLists

> 一句话概括：CMake 用一份 `CMakeLists.txt` 描述「**源文件怎么变成可执行文件**」，再由它生成对应平台真正的构建文件（Makefile / Visual Studio 工程 / Ninja）。
>
> ⚠️ 说明：原始笔记（`day1.md`）只有三行，且存在**语法错误**（用了 C++ 风格的 `//` 注释），本文已修正并在第四节逐条对照。

---

## 一、最小可运行示例

`CMakeLists.txt` 放在与源码同级的目录下：

```cmake
# CMakeLists.txt

# ① 要求的最低 CMake 版本
cmake_minimum_required(VERSION 3.10)

# ② 设定项目名称
project(my_project)

# ③ 定义可执行目标：把 main.cpp 变成可执行文件 my_project
add_executable(my_project main.cpp)
```

配套的 `main.cpp`：

```cpp
#include <iostream>

int main() {
    std::cout << "Hello CMake\n";
    return 0;
}
```

---

## 二、逐行解读

### ① `cmake_minimum_required(VERSION 3.10)`

**每个 `CMakeLists.txt` 的第一行**，声明本项目要求的最低 CMake 版本。

- 版本写低了：新语法可能不被支持，报错
- 版本写高了：老环境跑不起来
- 一般写你实际会用到的最低特性版本，或团队约定的版本

### ② `project(my_project)`

设定项目名称，同时会隐式设置一批变量（如 `PROJECT_NAME`、`PROJECT_SOURCE_DIR`）。

还可以顺带声明语言与版本：

```cmake
project(my_project VERSION 1.0 LANGUAGES CXX)
```

### ③ `add_executable(my_project main.cpp)`

**定义构建目标**：第一个参数是「要生成的可执行文件的名字」，后面跟**构成它的源文件列表**。

```cmake
add_executable(my_project main.cpp util.cpp)   # 多个源文件直接往后列
```

> 注意区分这两个名字：**目标名**（第一个参数）和 **`project()` 的项目名**是两回事，前者决定产物文件名。

---

## 三、怎么跑起来（out-of-source 构建）

```bash
mkdir build          # 建一个专门的构建目录
cd build
cmake ..             # 生成构建文件（.. 指向含 CMakeLists.txt 的目录）
cmake --build .      # 执行构建（等价于 make / ninja）
```

产物在 `build/` 目录里 → **源码目录保持干净**。

```text
项目/
├── CMakeLists.txt
├── main.cpp
└── build/            ← 所有中间产物、可执行文件都在这里，可以随时整个删掉重来
```

> 这是 CMake 的推荐用法（out-of-source build），**不要**直接在源码目录里 `cmake .`。

---

## 四、原始记录的逐条修正

原始笔记 `day1.md` 原文：

```cmake
cmake_minimum_required(VERSION_ 3.10) //要求的最低cmake版本

project(my_poject) //设定项目名称

add_executable(myproject main.cpp) //提前进行配置
```

| 原文 | 问题 | 修正 |
|---|---|---|
| `//要求的最低cmake版本` | ❌ **CMake 的注释符是 `#`，不是 `//`**；写 `//` 会直接报语法错误 | `# 要求的最低 CMake 版本` |
| `VERSION_ 3.10` | ❌ 多了一个下划线（`VERSION_` 不是合法关键字） | `VERSION 3.10` |
| `my_poject` | ❌ 拼写错误 | `my_project` |
| `//提前进行配置` | ⚠️ 表述不清。`add_executable` 的准确作用是**定义一个可执行目标**（声明产物名 + 源文件列表），不只是「配置」 | `# 定义可执行目标` |

> 📌 **最容易踩的坑**：C++ / Java 写惯了 `//`，写 CMake 时手会自己打出来。**CMake 里注释只有 `#`。**

---

## 五、常用命令速查

| 命令 | 作用 |
|------|------|
| `cmake_minimum_required(VERSION x.y)` | 声明最低 CMake 版本（必备首行） |
| `project(name)` | 定义项目名与语言 |
| `add_executable(target src...)` | 由源文件生成**可执行文件** |
| `add_library(target src...)` | 生成**库**（`STATIC` / `SHARED`） |
| `target_include_directories(t PRIVATE dir)` | 给目标添加头文件搜索路径 |
| `target_link_libraries(t PRIVATE lib)` | 给目标链接库 |
| `set(VAR value)` | 定义变量 |
| `aux_source_directory(dir VAR)` | 收集目录下所有源文件到变量 |

现代 CMake 推荐 **面向目标（target-based）** 的写法：把「头文件路径、链接库、编译选项」都挂在具体 target 上，而不是全局设置。

---

## 📌 易错点

1. **CMake 用 `//` 写注释**——必须是 `#`，这是从 C++ 转过来最常犯的错。
2. **`cmake_minimum_required` 不是第一行**——它前面不能有其他命令。
3. **忘了 `VERSION` 与版本号之间的写法**——是 `VERSION 3.10`，不是 `VERSION_3.10` 或 `VERSION_ 3.10`。
4. **在源码目录里 `cmake .`**——会污染源码目录，应建 `build/` 做 out-of-source 构建。
5. **改了 `CMakeLists.txt` 之后只 `make`**——`make` 会自动重跑 cmake，但如果加了**新源文件**且使用 `aux_source_directory`，建议手动重跑 `cmake ..`。
6. **把 `project()` 的名字当成产物名**——产物名由 `add_executable` 的第一个参数决定。
7. **文件名必须叫 `CMakeLists.txt`**——拼错（如 `CMakeList.txt`）CMake 找不到。

---

## 🧠 速记口诀

```text
CMake 注释只有 #，没有 //
三件套：cmake_minimum_required → project → add_executable
add_executable(产物名 源文件...)，产物名 ≠ project 名
out-of-source：mkdir build && cd build && cmake .. && cmake --build .
源文件变可执行，全在 build/ 里，随时可删
现代写法：面向 target 挂 include / link / option
```

---

## ✅ 自测问题

1. `CMakeLists.txt` 里注释该用什么符号？为什么从 C++ 转过来容易写错？
2. 最小可运行的三行命令分别是什么、各起什么作用？
3. `project(my_project)` 和 `add_executable(other_name main.cpp)` 里的两个名字分别决定什么？
4. 什么是不污染源码的构建方式？写出完整命令流程。
5. CMake 本身会编译代码吗？它和 `make` 是什么关系？
6. 「面向 target 的现代 CMake 写法」指的是什么？

---

> 来源：原始 `cmake/day1.md`（三行 CMake 命令 + 注释），已修正 `//` 注释与 `VERSION_`、`my_poject` 拼写错误，并补全构建流程与常用命令
