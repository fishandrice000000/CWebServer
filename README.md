# CWebServer - 基于C的Linux平台的高并发Web服务器

## 01 简介

本仓库是电子科技大学信软学院2024级大二下学期暑期工程实训班课程所用仓库.

项目目标：使用 C 语言在 Linux 平台上实现一个高并发 Web 服务器.

## 02 项目架构

```text
CWebServer/
├── src/                    # 主项目源代码
├── include/                # 公共头文件
├── test/                   # 单元测试
├── build/                  # 构建输出（不纳入版本控制）
├── lib/                    # 第三方依赖库
├── scripts/                # 辅助脚本（构建、测试等）
├── config/                 # 服务器配置文件
│
├── training/               # 课堂训练（与主项目独立）
│
├── docs/                   # 项目文档与课件
│   └── course/             # 课程 PDF 课件
│
├── CMakeLists.txt          # CMake 构建配置
├── .gitignore
└── README.md
```

### 模块分层

```text
┌─────────────────────────┐
│       main.c            │  ← 入口层
├─────────────────────────┤
│       server.c          │  ← 核心调度层
├──────────┬──────────────┤
│ http     │ thread_pool  │  ← 业务层
│ _parser  │              │
├──────────┴──────────────┤
│  io.c (epoll)           │  ← I/O 层
├─────────────────────────┤
│  timer.c  │  log.c      │  ← 基础设施层
├─────────────────────────┤
│  utils.c  │  buffer.c   │  ← 通用工具层
└─────────────────────────┘
```

## 03 技术栈

- **语言**: C (C11)
- **构建**: CMake + Make
- **调试**: GDB
- **平台**: Linux (Ubuntu)
- **I/O 模型**: epoll (I/O 多路复用)
- **并发模型**: 线程池 + Reactor 模式

## 04 环境要求

- Ubuntu 20.04+ 或其他 Linux 发行版
- GCC 9.0+
- CMake 3.10+
- GDB（调试用）

## 05 快速开始

```bash
# 构建
mkdir -p build && cd build
cmake ..
make

# 运行
./CWebServer

# 调试
gdb ./CWebServer
```
