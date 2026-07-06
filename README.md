# CWebServer - 基于C的Linux平台的高并发Web服务器

## 01 简介

本仓库是电子科技大学信软学院2024级大二下学期暑期工程实训班课程所用仓库。

项目目标：使用 C 语言在 Linux 平台上逐步实现一个高并发 Web 服务器。

## 02 项目架构

```text
CWebServer/
├── src/                          # 主项目源代码
│   ├── main.c                    # 入口
│   ├── config.c                  # 配置解析
│   ├── log.c                     # 日志系统
│   └── http_response.c           # HTTP 响应构建
├── include/                      # 公共头文件
│   ├── config.h
│   ├── log.h
│   └── http_response.h
├── config/                       # 配置文件
│   └── server.conf
├── www/                          # 静态资源目录
│   └── index.html
├── test/                         # 测试脚本
│   └── test_day01.sh
├── build/                        # 构建产物（gitignore）
├── logs/                         # 运行日志（gitignore）
├── training/                     # 课堂训练（与主项目独立）
│   └── w1d1/
│       ├── linked_list/          # 训练一：单向链表
│       └── fmt/                  # 训练二：文本格式化
├── docs/                         # 文档
│   ├── course/                   # 课程 PDF 课件
│   └── project/                  # 项目文档
│       ├── debug-log.txt         # 调试记录
│       └── what_we_have_done/    # 各课时完成总结
├── Makefile
├── .gitignore
└── README.md
```

### V0.1 模块关系

```text
┌──────────┐
│  main.c  │  入口：加载配置 → 初始化日志 → 输出 HTTP 响应
└──┬───┬───┘
   │   │
   ▼   ▼
┌──────┐ ┌────────┐ ┌────────────────┐
│config│ │  log   │ │ http_response  │
│ .c   │ │  .c    │ │     .c         │
└──────┘ └────────┘ └────────────────┘
 解析      写日志     构建 HTTP 响应
server.conf
```

后续课程将陆续加入：server、http_parser、thread_pool、io (epoll)、timer 等模块。

## 03 技术栈

- **语言**: C (C11)
- **构建**: Make
- **调试**: GDB
- **平台**: Linux (Ubuntu)
- **I/O 模型**: epoll (I/O 多路复用)
- **并发模型**: 线程池 + Reactor 模式

## 04 环境要求

- Ubuntu 20.04+ 或其他 Linux 发行版
- GCC 9.0+
- GDB（调试用）

## 05 快速开始

```bash
# 构建
make

# 运行
make run

# 测试
make test

# 清理
make clean
```

## 06 课程进度

| 课时 | 内容 | 状态 |
|------|------|------|
| W1D1 | Linux 开发环境、GNU 工具链、V0.1 工程骨架、链表/fmt 训练 | ✅ 完成 |

详见 [`docs/project/what_we_have_done/W1D1.md`](docs/project/what_we_have_done/W1D1.md)
