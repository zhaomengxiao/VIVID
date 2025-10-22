# Emscripten WebAssembly 调试指南

## 为什么需要 emrun？

### 问题：直接打开 `index.html` 没有内容

当你直接打开 `index.html` 文件时（使用 `file://` 协议），会遇到以下问题：

1. **CORS（跨域资源共享）限制**

   - WebAssembly 模块需要通过 HTTP 加载
   - `file://` 协议受到浏览器同源策略的限制

2. **MIME 类型问题**

   - `.wasm` 文件需要正确的 `application/wasm` MIME 类型
   - 本地文件系统不能提供正确的 MIME 类型

3. **文件访问限制**
   - 浏览器安全策略阻止 `file://` 协议访问其他资源
   - 这阻止了 WebAssembly 模块的正确加载

### 解决方案：使用 emrun

`emrun` 是 Emscripten 提供的工具，它会：

- 启动一个本地 HTTP 服务器
- 正确设置 HTTP 响应头（包括 MIME 类型）
- 提供必要的 CORS 头配置
- 允许浏览器正确加载 WebAssembly 模块

## 在 VSCode 中运行

### 方法 1：使用 VSCode 的 Debug 配置（推荐）

**步骤：**

1. 按 `F5` 或 点击 "Debug" 菜单
2. 选择 "HelloSDL3 - Emscripten (emrun)"
3. VSCode 会自动启动 emrun 并在浏览器中打开应用

**工作原理：**

- 触发 `emrun-hellosdl3` 任务启动本地服务器
- Chrome/Edge 浏览器自动打开 `http://localhost:6931/hello_sdl3/index.html`
- 可以使用 VSCode 的 DevTools 进行调试

### 方法 2：使用 CMake 的 Run Target

**步骤：**

1. 在 CMake: Select a Target 中选择 `run-emrun`
2. 点击构建工具栏中的 "Run" 按钮

或者在 VSCode 终端中：

```powershell
# 方式 1：使用 emrun（推荐）
cmake --build --preset VE --target run-emrun

# 方式 2：使用 HTTP 服务器
cmake --build --preset VE --target run-http-server
```

**然后手动打开浏览器：**

```
http://localhost:6931/hello_sdl3/index.html  # 使用 emrun
http://localhost:8000/hello_sdl3/index.html  # 使用 HTTP 服务器
```

### 方法 3：使用自动化脚本

```powershell
# PowerShell
cd D:\ClineWorkSpace\VIVID\build\emscripten
& "D:\ClineWorkSpace\emsdk\emsdk_env.ps1" | Out-Null
emrun --port 6931 hello_sdl3/index.html
```

然后打开浏览器访问 `http://localhost:6931/hello_sdl3/index.html`

## 调试技巧

### 1. 使用 Chrome DevTools

- 按 `F12` 打开开发者工具
- 在 "Console" 标签查看 JavaScript 输出
- 在 "Application" 标签查看 WebAssembly 模块

### 2. 在 VSCode 中设置断点

- 打开源文件并设置断点
- VSCode 会自动停在断点处
- 可以查看变量值和执行堆栈

### 3. emrun 的命令行选项

```bash
# 指定端口
emrun --port 6931 index.html

# 不自动打开浏览器
emrun --no-browser --port 6931 index.html

# 查看所有选项
emrun --help
```

### 4. 性能分析

在浏览器 DevTools 中：

- 打开 "Performance" 标签
- 记录性能轨迹
- 分析帧率和 CPU 使用

## 网络文件共享

如果需要从其他计算机访问应用：

```bash
# 获取本地 IP 地址
ipconfig

# 以 0.0.0.0 监听所有接口
emrun --port 6931 --bind 0.0.0.0 index.html
```

然后在其他计算机上访问：

```
http://<your-ip>:6931/hello_sdl3/index.html
```

## 常见问题

### Q: 为什么 WebAssembly 模块加载失败？

**A:** 确保：

1. 使用 HTTP/HTTPS 协议而不是 `file://`
2. 使用 emrun 或 Web 服务器
3. 检查浏览器控制台的错误信息

### Q: 如何在生产环境中部署？

**A:** 将整个 `build/emscripten` 目录复制到你的 Web 服务器：

```bash
# 例如使用 nginx
docker run -v $(pwd)/build/emscripten:/usr/share/nginx/html -p 80:80 nginx
```

### Q: emrun 启动变慢？

**A:** 这是正常的，因为 Emscripten 需要时间初始化 WebAssembly 运行时。

### Q: 如何调试 C++ 代码？

**A:** 使用浏览器的 WASM 调试器：

1. 确保编译时包含调试符号
2. 在 DevTools 中设置 WASM 断点
3. 使用 Chrome/Edge 的内置 WASM 调试功能

## 相关文件

- `.vscode/launch.json` - VSCode 启动配置
- `.vscode/tasks.json` - 后台任务配置
- `hello_sdl3/CMakeLists.txt` - CMake 目标配置
- `scripts/build_emscripten.ps1` - 编译脚本

## 更多信息

- [Emscripten 官方文档](https://emscripten.org/)
- [emrun 工具指南](https://emscripten.org/docs/tools_reference/emrun.html)
- [WebAssembly 规范](https://webassembly.org/)
