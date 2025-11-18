# Emscripten HelloSDL3 快速启动指南

## 快速开始（3 步）

### 1️⃣ 编译项目

在 VSCode 中：

- 打开 CMake 插件的 Configure 面板
- 选择 **"Emscripten WebAssembly"** 预设
- 点击 Configure 按钮

或在终端：

```powershell
.\scripts\build_emscripten.ps1
```

### 2️⃣ 运行应用

**方法 A：使用 VSCode Debug（推荐，一键启动）⭐⭐⭐**

按 `F5` 或 点击菜单 "Run" → "Start Debugging"

- 自动激活 Emscripten 环境
- 自动启动 emrun 服务器
- 浏览器自动打开 `http://localhost:6931`
- 可在 VSCode 中调试

**工作原理：**

- `.vscode/launch.json` 触发前置任务
- `.vscode/tasks.json` 先激活 Emscripten 环境，再启动 emrun
- PowerShell 执行：`. 'D:\ClineWorkSpace\emsdk\emsdk_env.ps1'; emrun ...`

**方法 B：手动启动**

在终端运行：

```powershell
cd D:\ClineWorkSpace\VIVID\build\emscripten
emrun --port 6931 hello_sdl3/index.html
```

然后在浏览器打开：`http://localhost:6931/hello_sdl3/index.html`

### 3️⃣ 调试

- 按 `F12` 打开浏览器 DevTools
- 查看 Console 标签的输出
- 设置断点进行调试

---

## 为什么需要先激活 Emscripten 环境？

直接运行 `emrun` 会找不到命令，因为：

| 情况                     | 结果                         |
| ------------------------ | ---------------------------- |
| 直接运行 `emrun`         | ❌ "emrun command not found" |
| 先激活环境再运行 `emrun` | ✅ 正常工作                  |

**激活环境做了什么：**

```powershell
. 'D:\ClineWorkSpace\emsdk\emsdk_env.ps1'
# 设置 EMSDK、PATH 等环境变量
# 使得 emrun、emcc 等工具可用
```

---

## 为什么需要 emrun？

| 方式             | file://      | HTTP (emrun)        |
| ---------------- | ------------ | ------------------- |
| WebAssembly 加载 | ❌ CORS 错误 | ✅ 正常             |
| MIME 类型        | ❌ 不正确    | ✅ application/wasm |
| 浏览器支持       | ❌ 受限      | ✅ 完整支持         |
| 调试能力         | ❌ 有限      | ✅ 完整             |

---

## VSCode 中的选项

### 选项 1：F5 快速启动（推荐）

配置文件：`.vscode/launch.json` 和 `.vscode/tasks.json`

- 一键启动：激活环境 → emrun → 浏览器 → 调试
- 最简单快捷的方式

### 选项 2：CMake 的 Run Target

在 CMake 工具栏中：

1. 点击 "Select Target" 下拉菜单
2. 选择 `hello_sdl3-run-emrun`（对于 hello_sdl3 项目）或 `editor-run-emrun`（对于 editor 项目）
3. 点击 "Run" 按钮

### 选项 3：终端命令

```powershell
# 快速重建并运行
.\scripts\build_emscripten.ps1 -BuildOnly

# 激活环境
& D:\ClineWorkSpace\emsdk\emsdk_env.ps1

# 启动 emrun
cd build\emscripten
emrun --port 6931 hello_sdl3/index.html
```

---

## 常用命令

```powershell
# 完整编译（清理 + 配置 + 编译）
.\scripts\build_emscripten.ps1 -Clean

# 仅编译（快速）
.\scripts\build_emscripten.ps1 -BuildOnly

# 仅配置
.\scripts\build_emscripten.ps1 -ConfigureOnly
```

---

## 浏览器访问

- **本地访问**：`http://localhost:6931/hello_sdl3/index.html`
- **其他计算机**：`http://<your-ip>:6931/hello_sdl3/index.html`

---

## 故障排除

### 错误：F5 后报错 "emrun command not found"

**原因：** Emscripten 环境没有被正确激活

**解决方案：** 已自动配置在 `.vscode/tasks.json` 中，检查：

```powershell
Test-Path "D:\ClineWorkSpace\emsdk\emsdk_env.ps1"
```

如果返回 `False`，说明路径不对，需要更新 `.vscode/tasks.json` 中的路径。

### 浏览器打开但页面空白

1. 打开浏览器 DevTools（F12）
2. 查看 Console 标签的错误信息
3. 检查 Network 标签中的 `.wasm` 文件是否加载成功

### 多次 F5 会启动多个 emrun 实例

这是正常的。解决方案：

1. 按 Shift+F5 停止当前调试
2. 关闭 Chrome 窗口
3. 重新按 F5 启动

---

## 下一步

- 查看 [`EMSCRIPTEN_DEBUGGING.md`](./EMSCRIPTEN_DEBUGGING.md) 了解高级调试技巧
- 查看 [`CHANGES_SUMMARY.md`](./CHANGES_SUMMARY.md) 了解所有修改
