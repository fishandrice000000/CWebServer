# 前端重构 — 总结

## 一、背景

原有前端仅 15 行 HTML + 10 行 CSS + 6 行 JS，功能单一。
在 V1.5 完成后，对 `www/` 进行全面重构，打造正式的项目展示页面。

## 二、前端设计

### 页面结构

| 区域 | 内容 |
|------|------|
| 导航栏 | 固定顶部，深色背景，包含功能特性/API 路由/在线演示/GitHub 链接，移动端汉堡菜单 |
| Hero 区 | 渐变深色背景，技术栈统计（C11/epoll/LT/8KB），V1.5 徽章 |
| 功能特性 | 6 张卡片：JSON 配置驱动、路由分发、Basic 认证、静态文件、动态查询、访问日志 |
| 技术架构 | ASCII 风格流程图：Client → epoll_wait → HTTP 解析 → 路由分发 → Handler → 日志 |
| API 路由表 | 8 条路由的方法/路径/说明/认证要求，可排序表格 |
| 在线演示 | 7 个交互式演示区块（见下） |
| Footer | 项目信息 + 技术栈标签 |

### 7 个演示区块

| # | 演示内容 | 技术实现 |
|---|---------|---------|
| 1 | 静态资源 200 | fetch GET，显示状态码 + Content-Type |
| 2 | GET 动态查询 | 表单输入 class/keyword → GET /search，DOMParser 解析 HTML `<table>` 渲染为真实表格 |
| 3 | POST 动态查询 | 同上，POST 方法 + application/x-www-form-urlencoded 请求体 |
| 4 | 路由配置生效 | 内置路由 /hello、JSON 配置路由 /users、POST /echo 自定义 body |
| 5 | 404 错误处理 | 请求不存在路径，验证 404 返回 |
| 6 | Basic 认证测试 | 专用端点 /api/auth-check（永无 WWW-Authenticate），单按钮三种场景 |
| 7 | 日志查看 | /api/logs 实时拉取服务器日志，刷新按钮 |

### 技术特点

- 纯静态 HTML/CSS/JS，零框架依赖
- CSS 自定义属性 + 响应式布局（768px / 480px 断点）
- DOMParser 解析 HTML 响应并渲染为页面元素
- 移动端适配：汉堡菜单、单列布局

## 三、服务端配套改动

### 新增 Handler

| Handler | 路由 | 功能 |
|---------|------|------|
| `log_viewer` | `GET /api/logs` | 读取日志文件返回 text/plain，限制 64KB |
| `auth_checker` | `GET /api/auth-check` | 与 /secured 相同认证逻辑，返回 JSON，永不发 WWW-Authenticate |

### 改进

| 改动 | 说明 |
|------|------|
| `users_get` 改为真实遍历 | 不再返回占位 HTML，遍历 CSV 链表生成用户表格（序号/用户名/手机号） |
| 401 优化 | 请求已携带 Authorization 头时，401 响应不再发送 WWW-Authenticate |
| `log_path` 参数传递 | request_handler.h → epoll_server.h → main.c 沿调用链传入日志路径 |

## 四、修改文件

| 文件 | 改动 |
|------|------|
| `www/index.html` | 全新：导航栏、Hero、功能卡片、架构图、API 表、7 个演示区块 |
| `www/css/style.css` | 全新：响应式样式系统，CSS 变量，深色/浅色主题 |
| `www/js/app.js` | 全新：fetch/XHR 请求、HTML 解析渲染、交互式演示逻辑 |
| `src/request_handler.c` | 新增 log_viewer、auth_checker、users_get 真实遍历、401 优化、前置声明 |
| `config/server.json` | 新增 /api/logs、/api/auth-check 路由 |
| `include/request_handler.h` | log_path 参数 |
| `include/epoll_server.h` | log_path 参数 |
| `src/epoll_server.c` | log_path 沿调用链传递 |
| `src/main.c` | 传入 config.log_path |
