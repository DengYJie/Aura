# Aura

Aura 是一个基于 Qt 6 的桌面端 AI 智能聊天助手项目，包含一个原生 `QWidget` 客户端和一个独立的 TCP + AI 网关服务端。

当前项目已经打通以下能力：

- 基于 FluentQt 的桌面界面
- 本地聊天记录存储
- 人与人 TCP 聊天
- OpenAI-compatible 模型的流式 AI 对话
- 基于本地解密上下文的情感分析与回复建议

## 项目架构

### 客户端

客户端代码位于 [`src/`](F:\source\homework\Aura\src)，按层次分为：

- `ui/`：窗口、导航、通用控件、聊天页面
- `domain/`：用例与仓储接口
- `data/local/`：本地 SQLite 与聊天数据源
- `data/remote/`：TCP 客户端与远程 AI 任务传输
- `data/repository/`：组合本地与远程数据源
- `utils/`：加密与图片处理工具

当前关键链路：

- 人聊消息在发送前于客户端本地加密。
- 对方消息在客户端本地解密后再落本地库。
- AI 聊天使用服务端维护的结构化上下文。
- 情感分析与回复建议使用**客户端本地解密后的最近几轮人聊消息**组装临时上下文，再发送给服务端做推理。

### 服务端

服务端代码位于 [`server/`](F:\source\homework\Aura\server)，当前承担三类职责：

- TCP 聊天路由
- 离线消息暂存与回放
- AI 任务网关

核心模块：

- `core/ChatServer`：连接接入与消息路由
- `core/ClientSession`：单连接协议处理
- `core/ServerDatabase`：离线消息存储
- `core/ServerConfig`：读取 `server_config.json`
- `ai/OpenAICompatibleClient`：DeepSeek / OpenAI / Ollama 兼容模型客户端
- `ai/AiModelRouter`：AI 路由、聊天上下文维护、任务 prompt 组织

### 共享协议

[`shared/Protocol.h`](F:\source\homework\Aura\shared\Protocol.h) 定义了前后端共用的 JSON-over-TCP 协议。

当前主要消息类型包括：

- `Auth` / `AuthAck`
- `Chat` / `ChatAck`
- `IncomingChat`
- `StreamChunk`
- `StreamEnd`
- `Error`

协议已支持：

- `taskType`
- `reqId`
- `messageId`

用于区分普通聊天、翻译、情感分析、回复建议等 AI 任务。

## 当前功能

- 基于 FluentQt 的桌面主界面与自定义标题栏
- 登录窗口与按用户划分的本地数据库
- 人与人文本聊天
- 离线消息存储与上线后同步
- AI 流式对话
- Markdown 消息渲染
- 翻译侧边栏
- 基于本地上下文的人聊情感分析
- 基于本地上下文的快捷回复建议

## 仓库结构

```text
Aura/
├─ src/                  # 桌面客户端
├─ server/               # TCP + AI 网关服务端
├─ shared/               # 前后端共享协议
├─ third_party/FluentQt/ # UI 框架
├─ server_config.json    # 当前服务端配置
└─ CMakeLists.txt        # 根构建入口
```

## 环境要求

- Qt 6.11.1
- CMake
- FluentQt
- `qwindowkit`

## 运行

### 启动服务端

```powershell
.\AuraServer.exe
```


### 启动客户端

```powershell
.\Aura.exe
```

当前登录窗口支持输入简单用户 ID，例如：

- `01`
- `02`

## 配置说明

服务端配置模板见 `server_config.json.example`

主要配置段：

- `server`：监听地址、端口、数据库路径
- `context`：服务端 AI 聊天上下文窗口
- `providers`：模型提供商配置
- `routing`：任务到模型的映射
- `prompts`：各类 AI 任务的默认 prompt

当前默认路由大致为：

- `chat`：DeepSeek
- `sentiment`：DeepSeek
- `suggest`：DeepSeek
- `translate`：OpenAI
- `ollama`：本地 OpenAI-compatible 接口
