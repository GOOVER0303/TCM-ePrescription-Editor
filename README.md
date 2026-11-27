# 中医电子处方编辑系统

基于 **C++17** 的中文命令行中医电子处方编辑系统，使用自行实现的**双向链栈**保存 Undo/Redo 操作命令，并通过命令对象统一处理常用方、处方、中药和药量控制等业务修改。

> 本项目仅用于数据结构课程演示，预置药物、方剂和剂量数据不构成临床用药建议。

## 功能

- **常用方管理**：新增、修改、删除、查看个人常用方，并应用到草稿处方。
- **处方开具**：新增草稿、增删改药味、修改剂数、规范检测和提交。
- **超量签名**：按“药物名称 + 规格”匹配药量上限，超量时要求医生确认并自动写入签名。
- **处方状态流转**：草稿 -> 已提交 -> 已撤销 -> 草稿，仅本人可撤销，恢复后需重新提交。
- **中药管理**：维护编号、名称、产地、规格，并保护被引用数据。
- **药量控制**：维护药物名称、规格和最大用量。
- **Undo/Redo**：所有业务修改均进入命令历史，最多保留 100 步。
- **数据持久化**：UTF-8 文本自动保存，支持从种子数据恢复。

## 环境要求

- Windows 10/11
- 支持 C++17 的 g++（已在 MSYS2 UCRT64 g++ 16.2.0 验证）

项目零第三方运行时依赖。

## 编译

在项目根目录打开 PowerShell：

```powershell
New-Item -ItemType Directory -Force bin | Out-Null
$sources = Get-ChildItem -Path src -Filter *.cpp | ForEach-Object { $_.FullName }
g++ -std=c++17 -Wall -Wextra -pedantic `
    -finput-charset=UTF-8 -fexec-charset=UTF-8 `
    -Isrc $sources -o bin\tcm_prescription.exe
```

## 运行

必须从项目根目录运行，以便程序找到 `data/`：

```powershell
.\bin\tcm_prescription.exe
```

重置为预置演示数据：

```powershell
.\bin\tcm_prescription.exe --reset-demo
```

指定其他数据目录：

```powershell
.\bin\tcm_prescription.exe --data-dir .\my_data
```

## 菜单

```text
1. 常用方管理
2. 处方管理
3. 中药信息管理
4. 药量控制
5. 撤销/重做与历史
6. 系统与演示数据
7. 快速撤销上一步
8. 快速重做上一步
0. 退出
```

处方状态：

```text
DRAFT --提交--> SUBMITTED --业务撤销--> WITHDRAWN
  ^                                         |
  +---------------恢复为草稿----------------+
```

“处方业务撤销”与编辑器式“撤销上一步”是两个不同操作。

## 预置数据

- 当前医生：`D001 / 演示医生 / 中医内科`
- 其他医生：`D002 / 协作医生`
- 26 味教学中药
- 4 项药量控制
- 3 个个人常用方
- 4 张不同状态处方

其中草稿 `P0001` 含炙甘草 12.00 g，超过演示上限 10.00 g，可用于验证超量确认和签名流程。

## 目录结构

```text
src/      C++17 源码
data/     当前业务数据和 data/seed 种子数据
README.md 说明文档
```

## 核心实现

- `DoublyLinkedStack<T>`：节点维护 `prev/next`，支持 O(1) 入栈、出栈和最旧命令淘汰。
- `ICommand`：定义 `execute`、`undo` 和 `description`。
- `CommandManager`：执行命令、保存数据、管理 Undo/Redo、清空 Redo 和处理保存失败回滚。
- `Storage`：负责七个 UTF-8 文本文件的加载、保存和种子恢复。

## 已知限制

- 单医生演示，不实现登录和用户管理；
- 仅支持个人常用方；
- 药物名称采用精确匹配；
- 用量统一使用克，最多两位小数；
- Undo/Redo 历史不跨程序启动保存；
- 不包含患者资料、真实电子签名、打印和网络数据库。
