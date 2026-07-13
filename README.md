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
│   ├── http_response.c           # HTTP 响应构建
│   ├── user_store.c              # 用户存储
│   ├── user_index.c              # BST 索引
│   ├── request_handler.c         # 请求处理
│   ├── process_server.c          # 多进程服务
│   ├── thread_server.c           # 多线程服务
│   └── tcp_server.c              # TCP 网络服务
├── include/                      # 公共头文件
│   ├── config.h
│   ├── log.h
│   ├── http_response.h
│   ├── user_store.h
│   ├── user_index.h
│   ├── request_handler.h
│   ├── process_server.h
│   ├── thread_server.h
│   └── tcp_server.h
├── data/                         # 数据文件
│   ├── users.csv
│   └── users_large.csv
├── config/                       # 配置文件
│   └── server.conf
├── www/                          # 静态资源目录
│   └── index.html
├── test/                         # 测试脚本
│   ├── test_day01.sh
│   ├── test_day02.sh
│   ├── test_day03.sh
│   ├── test_day04.sh
│   ├── test_day05.sh
│   └── test_day06.sh
├── requests/                     # 请求文件 (.req)
├── outputs/                      # 输出文件 (gitignore)
├── build/                        # 构建产物（gitignore）
├── logs/                         # 运行日志（gitignore）
├── training/                     # 课堂训练（与主项目独立）
│   ├── w1d1/
│   │   ├── linked_list/          # 单向链表
│   │   └── fmt/                  # 文本格式化
│   ├── w1d2/
│   │   ├── train1/               # CSV + 链表 CRUD
│   │   └── train2/               # 有序链表
│   ├── w1d3/
│   │   ├── train1/               # BST 查找与新增
│   │   └── train2/               # BST 删除与修改
│   ├── w1d4/
│   │   ├── train1/               # 简单 Shell
│   │   └── train2/               # 生产者-消费者
│   ├── w1d5/
│   │   └── philosopher/          # 哲学家进餐
│   └── w2d1/
│       └── chat/                 # TCP 文本聊天
├── docs/                         # 文档
│   ├── course/                   # 课程 PDF 课件
│   └── project/                  # 项目文档
│       ├── debug-logs/           # 调试记录
│       └── what_we_have_done/    # 各课时完成总结
├── Makefile
├── .gitignore
└── README.md
```

### V0.6 模块关系

```text
┌──────────┐
│  main.c  │  入口：配置加载 → 日志初始化 → 用户管理 / 多进程 / 多线程 / TCP 服务
└──┬───┬───┴───┬───┬───┬───┬───┐
   │   │       │   │   │   │   │
   ▼   ▼       ▼   ▼   ▼   ▼   ▼
┌──────┐ ┌──────┐ ┌────────────┐ ┌────────────────┐ ┌────────────┐ ┌──────────────────┐ ┌──────────────────┐ ┌────────────┐
│config│ │ log  │ │user_store  │ │ http_response  │ │user_index  │ │process_server    │ │thread_server     │ │tcp_server  │
│ .c   │ │ .c   │ │    .c      │ │     .c         │ │    .c      │ │ +request_handler │ │ +request_handler │ │+req_handler│
└──────┘ └──────┘ └────────────┘ └────────────────┘ └────────────┘ └──────────────────┘ └──────────────────┘ └────────────┘
 解析配置  写日志   CSV 链表存储    HTTP 响应构建    BST 索引 (指针)   多进程请求分发      多线程请求分发      TCP socket 服务
                  增删查                          查找/compare       fork/exec/waitpid   mutex+cond+队列     bind/listen/accept
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

# 测试（Day01）
make test01

# 测试（Day02）
make test02

# 测试（Day03）
make test03

# 测试（Day04）
make test04

# 测试（Day05）
make test05

# 测试（Day06）
make test06

# 清理
make clean
```

## 06 课程进度

| 课时 | 内容 | 状态 |
|------|------|------|
| W1D1 | Linux 开发环境、GNU 工具链、V0.1 工程骨架、链表/fmt 训练 | ✅ 完成 |
| W1D2 | 指针与链表、CSV 读写、V0.2 用户存储与查询 | ✅ 完成 |
| W1D3 | 数组与结构、BST、V0.3 用户索引与查找对比 | ✅ 完成 |
| W1D4 | 多进程并发、XSI IPC、V0.4 多进程请求处理 | ✅ 完成 |
| W1D5 | 多线程并发、互斥量/条件变量、V0.5 多线程请求处理 | ✅ 完成 |
| W2D1 | 网络编程基础、TCP 协议、Socket API、V0.6 TCP 网络服务 | ✅ 完成 |

详见：
- [`docs/project/what_we_have_done/W1D1.md`](docs/project/what_we_have_done/W1D1.md)
- [`docs/project/what_we_have_done/W1D2.md`](docs/project/what_we_have_done/W1D2.md)
- [`docs/project/what_we_have_done/W1D3.md`](docs/project/what_we_have_done/W1D3.md)
- [`docs/project/what_we_have_done/W1D4.md`](docs/project/what_we_have_done/W1D4.md)
- [`docs/project/what_we_have_done/W1D5.md`](docs/project/what_we_have_done/W1D5.md)
- [`docs/project/what_we_have_done/W2D1.md`](docs/project/what_we_have_done/W2D1.md)
