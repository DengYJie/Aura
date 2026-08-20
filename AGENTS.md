# AGENTS.md

## Project Overview

Aura is a Windows-focused Qt 6 desktop AI chat assistant with:

- a FluentQt `QWidget` client in [`src/`](F:\source\homework\Aura\src)
- a separate TCP + AI gateway server in [`server/`](F:\source\homework\Aura\server)
- a shared JSON line protocol in [`shared/`](F:\source\homework\Aura\shared)

The codebase follows a lightweight layered structure:

- `src/ui`: views, windows, navigation, widgets, page composition
- `src/domain`: use cases and repository interfaces
- `src/data`: local storage, remote transport, repository implementations
- `server/src/core`: TCP server, sessions, offline storage, config
- `server/src/ai`: AI model client and task router

## Setup Commands

### Visual Studio environment

Run all Windows builds from a VS developer environment:

```powershell
cmd /c "call ""F:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"""
```

### Configure desktop client

```powershell
"F:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=F:/Qt/6.11.1/msvc2022_64
```

### Build desktop client

```powershell
"F:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build
```

### Configure server

```powershell
"F:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S server -B server/build -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=F:/Qt/6.11.1/msvc2022_64
```

### Build server

```powershell
"F:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build server/build
```

## Development Workflow

- The root `CMakeLists.txt` builds the desktop app and includes `server/` as a subdirectory.
- `server/CMakeLists.txt` can also be configured independently.
- The desktop app starts from [`src/main.cpp`](F:\source\homework\Aura\src\main.cpp).
- The server starts from [`server/src/main.cpp`](F:\source\homework\Aura\server\src\main.cpp).
- The login flow initializes `AppContainer` after a user ID is entered.
- The desktop client uses `TcpChatClient` to connect to `127.0.0.1:8080` by default.

## Testing and Validation

There is no dedicated automated test suite yet. Validation is currently build- and workflow-driven.

Minimum validation after code changes:

- Rebuild the desktop app:
  - `cmake --build build`
- Rebuild the server:
  - `cmake --build server/build`
- For protocol or AI task changes, manually validate:
  - login/auth handshake
  - human-to-human message delivery
  - offline message replay
  - AI streaming responses
  - translation pane behavior
  - sentiment and suggestion tasks in human chat

When touching chat protocol or AI task routing, validate both binaries in the same round.

## Code Style and Conventions

- Language: C++23
- UI framework: Qt 6 Widgets + FluentQt
- Prefer ASCII unless the file already uses Chinese or another required Unicode string.
- Keep business logic out of raw widget callbacks when a use case or repository boundary already exists.
- Visible custom widgets should inherit `fluent::FluentElement` (and `fluent::QMLPlus` when appropriate).
- Prefer Fluent design tokens over hard-coded styling:
  - `themeColors()/themeColorsRef()`
  - `themeFont(...)`
  - `themeRadius()`
- Use `apply_patch` for manual code edits.
- Prefer `rg` for search and `rg --files` for file discovery.

## Architecture Notes

### Client chat pipeline

The current desktop chat flow is:

1. `ChatPage`
2. `ChatViewModel`
3. `SendMessageUseCase` / `GetMessagesUseCase` / `TranslateUseCase`
4. `IChatRepository`
5. `ChatRepositoryImpl`
6. `LocalChatDataSource` + `RemoteAiDataSource` + `TcpChatClient`

### AI task split

The current AI model behavior is intentionally split:

- `chat`
  - server-side structured context
  - role-aware history: `system / user / assistant`
  - turn-count and character-budget trimming

- `translate`
  - one-shot request

- `sentiment` and `suggest`
  - built from **client-local decrypted human dialogue**
  - sent to the server as transient task input
  - not persisted server-side as AI context

If you change this boundary, update both:

- [`src/data/repository/ChatRepositoryImpl.cpp`](F:\source\homework\Aura\src\data\repository\ChatRepositoryImpl.cpp)
- [`server/src/ai/AiModelRouter.cpp`](F:\source\homework\Aura\server\src\ai\AiModelRouter.cpp)

### Storage

Client local DB:

- `aura_chat_<userId>.db`

Server DB:

- `aura_server.db`

Do not move decrypted human-chat assist context into server-side long-lived storage unless the requirements explicitly change.

## Build and Deployment Notes

- Qt path currently assumed by local builds:
  - `F:/Qt/6.11.1/msvc2022_64`
- Visual Studio path currently assumed by local builds:
  - `F:\Program Files\Microsoft Visual Studio\18\Community`
- Root build prefers a local `qwindowkit` checkout at:
  - `F:/source/homework/AIChatDraw/3rdparty/qwindowkit`
- If that path is unavailable, the build falls back to `FetchContent`.

The server copies `server_config.json` (or the example file) into the output directory after build.

## Security and Data Handling

- Human-to-human messages are encrypted before network transport.
- Incoming peer messages are decrypted locally on the client.
- Sentiment analysis and reply suggestion must use locally decrypted history only.
- The server may route transient assist requests but should not retain those decrypted local conversation windows as AI history.

## Troubleshooting

- Qt `AutoMoc` may emit license service warnings in this environment.
  - In the current setup these warnings did not prevent successful builds.
- If client build succeeds but link fails after signal signature changes, search for stale connections with:

```powershell
rg -n "errorOccurred|streamChunk|streamEnd|sendMessage\(" src
```

- If the server and client disagree on payload shape, inspect:
  - [`shared/Protocol.h`](F:\source\homework\Aura\shared\Protocol.h)
  - [`src/data/remote/TcpChatClient.cpp`](F:\source\homework\Aura\src\data\remote\TcpChatClient.cpp)
  - [`server/src/core/ClientSession.cpp`](F:\source\homework\Aura\server\src\core\ClientSession.cpp)

## PR / Change Guidelines

- Keep client and server protocol changes in the same patch set.
- For AI task changes, document whether the context is:
  - server-managed chat context
  - client-local decrypted transient context
- Before finishing, rebuild both:
  - `build`
  - `server/build`

## High-Risk Files

Changes here often have cross-layer effects:

- [`shared/Protocol.h`](F:\source\homework\Aura\shared\Protocol.h)
- [`src/data/remote/TcpChatClient.cpp`](F:\source\homework\Aura\src\data\remote\TcpChatClient.cpp)
- [`src/data/repository/ChatRepositoryImpl.cpp`](F:\source\homework\Aura\src\data\repository\ChatRepositoryImpl.cpp)
- [`server/src/core/ClientSession.cpp`](F:\source\homework\Aura\server\src\core\ClientSession.cpp)
- [`server/src/ai/AiModelRouter.cpp`](F:\source\homework\Aura\server\src\ai\AiModelRouter.cpp)
