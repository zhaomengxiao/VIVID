# Emscripten 项目中加载资源文件到虚拟文件系统

> 本文将手把手教你如何在 Emscripten/WebAssembly 项目中正确加载资源文件（如图片、字体、配置文件等），让你的 C++ 代码在浏览器中也能像在本地一样访问文件。

## 📚 前置知识：什么是 Emscripten？

**Emscripten** 是一个编译器工具链，它可以把 C/C++ 代码编译成 **WebAssembly**（简称 WASM），让你的 C++ 程序能在浏览器中运行。

想象一下：
- 🖥️ **传统方式**：C++ 程序 → 编译 → `.exe` 文件 → 在 Windows/Mac/Linux 上运行
- 🌐 **Emscripten 方式**：C++ 程序 → 编译 → `.wasm` 文件 → 在浏览器中运行

## 🤔 问题：浏览器中的文件访问困境

### 为什么需要特殊处理？

在传统的 C++ 程序中，你可以这样读取文件：

```cpp
// 这在本地程序中完全没问题
std::ifstream file("assets/image.png");
```

但在浏览器中，这样做会**失败**！原因是：

1. **浏览器的沙箱限制**：出于安全考虑，浏览器中的代码不能直接访问用户的本地文件系统
2. **没有真实的文件系统**：WebAssembly 运行在浏览器的内存中，没有传统意义上的"硬盘"

### Emscripten 的解决方案：虚拟文件系统（VFS）

Emscripten 提供了一个巧妙的解决方案：**虚拟文件系统**（Virtual File System，简称 VFS）。

**虚拟文件系统**就像是在浏览器的内存中模拟了一个"假的硬盘"：
- 📁 你的资源文件被打包成一个 `.data` 文件
- 🚀 浏览器加载网页时，自动下载这个 `.data` 文件
- 💾 Emscripten 把文件内容加载到内存中的"虚拟硬盘"
- ✅ 你的 C++ 代码可以像访问真实文件一样访问这些资源

## 🛠️ 实战：如何打包资源文件

### 方法一：使用 CMake 的 `target_link_options`

这是最推荐的方法，适合使用 CMake 构建系统的项目。

#### 步骤 1：准备资源文件

假设你的项目结构如下：

```
my_project/
├── CMakeLists.txt
├── src/
│   └── main.cpp
└── assets/              # 资源文件夹
    ├── fonts/
    │   └── arial.ttf
    └── images/
        └── logo.png
```

#### 步骤 2：在 CMakeLists.txt 中配置

在你的 `CMakeLists.txt` 文件中，添加以下配置：

```cmake
# 检测是否是 Emscripten 构建
if(EMSCRIPTEN)
    # 为你的可执行文件添加链接选项
    target_link_options(MyApp PRIVATE
        # 其他 Emscripten 选项...
        "-sWASM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        
        # 关键：打包资源文件到虚拟文件系统
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
    )
endif()
```

#### 理解 `--preload-file` 参数

```cmake
"--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
```

这行代码的含义：
- `${CMAKE_CURRENT_SOURCE_DIR}/assets`：**源路径**，指向你电脑上的实际资源文件夹
- `@/assets`：**虚拟路径**，资源在虚拟文件系统中的挂载位置
- 整体意思：把本地的 `assets` 文件夹打包，并在虚拟文件系统的 `/assets` 路径下可访问

#### ⚠️ 常见错误

**错误写法**（会导致编译失败）：
```cmake
# ❌ 错误：分成两个参数
"--preload-file"
"${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
```

**正确写法**：
```cmake
# ✅ 正确：使用 = 连接成单个参数
"--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
```

### 方法二：封装成可复用函数

如果你有多个项目需要加载资源，可以创建一个 CMake 函数：

```cmake
# 在 CMakeLists.txt 中定义函数
function(add_resources_to_target TARGET_NAME RESOURCE_DIR VIRTUAL_PATH)
    if(EMSCRIPTEN)
        # 规范化路径（处理 Windows 反斜杠问题）
        file(TO_CMAKE_PATH "${RESOURCE_DIR}" NORMALIZED_PATH)
        
        # 添加链接选项
        target_link_options(${TARGET_NAME} PRIVATE
            "--preload-file=${NORMALIZED_PATH}@${VIRTUAL_PATH}"
        )
        
        message(STATUS "Resources from ${RESOURCE_DIR} will be available at ${VIRTUAL_PATH}")
    endif()
endfunction()

# 使用函数
add_resources_to_target(MyApp 
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"  # 本地路径
    "/assets"                              # 虚拟路径
)
```

## 📝 在 C++ 代码中访问资源

打包完成后，你的 C++ 代码可以直接访问虚拟文件系统中的文件：

```cpp
#include <fstream>
#include <iostream>

int main() {
    // 访问虚拟文件系统中的文件
    // 路径对应 CMake 中配置的虚拟路径
    std::ifstream file("/assets/fonts/arial.ttf", std::ios::binary);
    
    if (file.is_open()) {
        std::cout << "✅ 字体文件加载成功！" << std::endl;
        // 读取文件内容...
    } else {
        std::cout << "❌ 文件加载失败" << std::endl;
    }
    
    return 0;
}
```

## 🔍 验证资源是否正确打包

### 1. 检查生成的文件

构建完成后，检查输出目录，应该看到以下文件：

```
build/
├── index.html       # 网页入口
├── index.js         # JavaScript 胶水代码
├── index.wasm       # WebAssembly 二进制文件
└── index.data       # 📦 资源文件包（重要！）
```

**`index.data` 文件**就是打包后的资源文件，它的大小应该接近你原始资源文件的总大小。

### 2. 在浏览器中验证

打开浏览器的开发者工具（按 `F12`）：

#### 方法 A：查看网络请求

1. 切换到 **Network**（网络）标签页
2. 刷新页面
3. 查找 `index.data` 文件
   - ✅ 状态码应该是 `200`
   - ✅ 文件大小应该符合预期

#### 方法 B：检查虚拟文件系统

在浏览器的 **Console**（控制台）中运行以下 JavaScript 代码：

```javascript
// 列出虚拟文件系统根目录
FS.readdir('/')

// 列出 assets 目录
FS.readdir('/assets')

// 列出 fonts 子目录
FS.readdir('/assets/fonts')

// 检查特定文件是否存在
FS.stat('/assets/fonts/arial.ttf')
```

如果文件存在，`FS.stat()` 会返回文件信息；如果不存在，会抛出错误。

## 🎯 实际案例：加载字体文件

这是一个完整的示例，展示如何在 ImGui 项目中加载自定义字体：

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyWebApp)

# 添加可执行文件
add_executable(MyWebApp src/main.cpp)

# Emscripten 特定配置
if(EMSCRIPTEN)
    # 设置输出为 HTML
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    target_link_options(MyWebApp PRIVATE
        "-sUSE_SDL=3"
        "-sWASM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        
        # 加载资源文件
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/resources@/res"
    )
endif()
```

### main.cpp

```cpp
#include <imgui.h>
#include <fstream>

void InitializeUI() {
    ImGuiIO& io = ImGui::GetIO();
    
    // 尝试加载自定义字体
    const char* fontPath = "/res/fonts/NotoSans-Regular.ttf";
    
    // 检查文件是否存在
    std::ifstream check(fontPath);
    if (check.good()) {
        check.close();
        
        // 加载字体
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 28.0f);
        
        if (font != nullptr) {
            printf("✅ 字体加载成功！\n");
        } else {
            printf("❌ 字体加载失败\n");
        }
    } else {
        printf("❌ 字体文件不存在：%s\n", fontPath);
    }
}
```

## 🐛 常见问题排查

### 问题 1：找不到 `index.data` 文件

**症状**：构建成功，但输出目录中没有 `index.data` 文件

**可能原因**：
1. `--preload-file` 参数写法错误（使用了两个参数而不是一个）
2. 资源文件夹路径不正确
3. CMake 缓存问题

**解决方法**：
```bash
# 清理构建缓存
rm -rf build
mkdir build
cd build

# 重新配置
emcmake cmake ..

# 重新构建
cmake --build .
```

### 问题 2：浏览器控制台报错 "file not found"

**症状**：`index.data` 存在，但 C++ 代码无法读取文件

**可能原因**：虚拟路径配置错误

**检查清单**：
1. CMake 中的虚拟路径：`@/assets`
2. C++ 代码中的访问路径：`/assets/...`
3. 两者必须匹配！

### 问题 3：Windows 路径问题

**症状**：在 Windows 上构建失败，错误信息包含反斜杠 `\`

**原因**：Emscripten 需要 Unix 风格的路径（正斜杠 `/`）

**解决方法**：使用 `file(TO_CMAKE_PATH)` 规范化路径

```cmake
# 规范化路径
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets" ASSETS_PATH)

# 使用规范化后的路径
target_link_options(MyApp PRIVATE
    "--preload-file=${ASSETS_PATH}@/assets"
)
```

## 📊 性能优化建议

### 1. 只打包必要的文件

不要打包整个项目目录，只打包运行时需要的资源：

```cmake
# ❌ 不好：打包了太多不必要的文件
"--preload-file=${CMAKE_SOURCE_DIR}@/"

# ✅ 好：只打包资源文件夹
"--preload-file=${CMAKE_SOURCE_DIR}/assets@/assets"
```

### 2. 考虑使用 `--embed-file`

如果资源文件很小（< 100KB），可以考虑使用 `--embed-file` 直接嵌入到 `.wasm` 文件中：

```cmake
# 嵌入文件（适合小文件）
"--embed-file=${CMAKE_SOURCE_DIR}/config.json@/config.json"
```

**区别**：
- `--preload-file`：生成独立的 `.data` 文件，异步加载
- `--embed-file`：嵌入到 `.wasm` 文件，立即可用，但会增大 `.wasm` 体积

### 3. 压缩资源文件

对于大型资源文件，考虑先压缩再打包：

```cmake
# 启用文件压缩
target_link_options(MyApp PRIVATE
    "--preload-file=${ASSETS_PATH}@/assets"
    "-sLZ4=1"  # 使用 LZ4 压缩
)
```

## 🎓 总结

1. **虚拟文件系统**是 Emscripten 在浏览器中模拟文件访问的关键技术
2. 使用 `--preload-file=源路径@虚拟路径` 打包资源文件
3. 注意参数必须是**单个字符串**，使用 `=` 连接
4. 在 Windows 上使用 `file(TO_CMAKE_PATH)` 规范化路径
5. 通过浏览器开发者工具验证 `index.data` 文件和虚拟文件系统

## 🔗 延伸阅读

- [Emscripten 官方文档：文件系统](https://emscripten.org/docs/api_reference/Filesystem-API.html)
- [Emscripten 官方文档：打包文件](https://emscripten.org/docs/porting/files/packaging_files.html)
- [WebAssembly 入门教程](https://webassembly.org/getting-started/developers-guide/)

---

💡 **小贴士**：如果你在实践中遇到问题，记得打开浏览器的开发者工具，查看 Console 和 Network 标签页，它们会提供很多有用的调试信息！
