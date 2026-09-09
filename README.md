# x64 PE Loader

---
##  项目简介

本项目手动实现 PE 解析与内存加载工具。主映像（目标可执行文件）由该程序独立完成解析与内存映射，不经过 Windows 加载器；导入表填充环节借助标准 API（LoadLibrary/GetProcAddress）获取系统依赖模块地址。

---

##  核心功能

###  1. PE解析与拉伸
- 手动解析 DOS头、NT头、节表
- 遍历节表，将文件中的数据按节区对齐拉伸至内存
- 处理 `VirtualSize` 与 `SizeOfRawData` 的差异，多余空间补零

### 2. 导入表填充
- 解析 INT（导入名称表）
- 区分序号导入与名称导入：
  - 判断 IMAGE_ORDINAL_FLAG64 标志位
  - 序号导入：通过 `MAKEINTRESOURCE` 调用 `GetProcAddress`
  - 名称导入：解析 `IMAGE_IMPORT_BY_NAME` 结构
- 将获取到的函数地址写入 IAT（导入地址表）

### 3. 重定位表修复
- 计算实际加载基址与首选基址的偏移（delta）
- 遍历所有重定位块
- 修复 `IMAGE_REL_BASED_DIR64` 类型项，确保地址正确性
- 支持 ASLR 场景 

### 4.入口点启动
- 获取 `AddressOfEntryPoint`（OEP）
- 通过 `CreateThread` 启动目标程序
- 等待执行完成
---

## 快速开始
### 1.准备工作
使用 Visual Studio 打开解决方案，编译 `x64 PE_tool` 项目，生成`x64 PE_tool.exe`
> 或者直接使用`Test program`文件下，编译好的`x64 PE_tool.exe`
### 2.运行程序
打开`x64 PE_tool.exe`，或者根据提示输入目标 PE 文件的**完整路径**（例如：`C:\Users\test\myapp.exe`） 

> **快捷操作** 直接将 x64 的 `.exe` 文件拖拽进控制台窗口，路径会自动填入，回车即可运行。

---

## 效果演示

#### 1.文件路径带空格
![文件路径带空格](https://github.com/Smile-axis/x64-PELoader/blob/main/screenshots/with-space.png)
#### 2.文件路径不带空格
![文件路径不带空格](https://github.com/Smile-axis/x64-PELoader/blob/main/screenshots/without-space.png)
#### 3.win11下的计算器演示
![win11下的计算器演示](https://github.com/Smile-axis/x64-PELoader/blob/main/screenshots/Win11_calc.png)
#### 4.win10下的计算器演示
![win10下的计算器演示](https://github.com/Smile-axis/x64-PELoader/blob/main/screenshots/Win10_calc.png)
#### 5.Debug + ASLR
![Debug + ASLR](https://github.com/Smile-axis/x64-PELoader/blob/main/screenshots/Debug_ASLR.png)
#### 6.Debug + NoASLR
![Debug + NoASLR](https://github.com/Smile-axis/x64-PELoader/blob/main/screenshots/Debug_NoASLR.png)
#### 7.Release + ASLR
![Release + ASLR](https://github.com/Smile-axis/x64-PELoader/blob/main/screenshots/Release_ASLR.png)
#### 8.Release + NoASLR
![Release + NoASLR](https://github.com/Smile-axis/x64-PELoader/blob/main/screenshots/Release_NoASLR.png)


## 技术栈

| 类别 | 技术 |
| :--- | :--- |
| 语言 | C / C++ |
| 核心机制 | 手动PE解析、内存节区拉伸、导入表填充（含序号/名称导入）、重定位表修复（IMAGE_REL_BASED_DIR64） |
| 开发环境 | Visual Studio 2022 |

---


## ⚠️ 注意事项
- 本加载器**未处理**以下情况，使用时需注意：
  - TLS 回调函数（`IMAGE_TLS_DIRECTORY`）  
  - 异常处理（SEH）  
  - 资源节（`.rsrc`）及其他数据目录  
  - 延迟导入（Delay-Load）  
  - .NET / CLR 混合程序集  
> **说明**：以上场景属于 PE 加载器的延伸功能，当前实现优先保证核心流程的完整性和正确性。
