# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Structure

VIVID is being restructured from VIVID_D into a modern CMake template with engine/editor separation:
- **engine/**: Core library (VIVID engine) containing all non-editor code
- **editor/**: Standalone application referencing the engine library
- Uses CPM.cmake for dependency management (replacing pkgconfig)

## Build Commands

### Development Workflow
```bash
# Build everything (recommended for development)
cmake -S all -B build
cmake --build build

# Run specific components
./build/test/VIVIDTests          # Run tests
./build/standalone/VIVIDEditor   # Run editor

# Build engine library only
cmake -S . -B build/engine
cmake --build build/engine

# Build editor standalone
cmake -S standalone -B build/editor
cmake --build build/editor

# Run tests
cmake -S test -B build/test
cmake --build build/test
CTEST_OUTPUT_ON_FAILURE=1 cmake --build build/test --target test

# Format code
cmake --build build --target format    # View changes
cmake --build build --target fix-format # Apply changes

# Build documentation
cmake -S documentation -B build/doc
cmake --build build/doc --target GenerateDocs
```

### Configuration Options
```bash
# Enable test coverage
cmake -DENABLE_TEST_COVERAGE=1 -B build/test

# Enable sanitizers
cmake -DUSE_SANITIZER="Address;Undefined" -B build

# Enable static analysis
cmake -DUSE_STATIC_ANALYZER="clang-tidy" -B build
```

## Architecture Migration Plan

### Current VIVID_D Structure → New Structure
**VIVID_D/src/** → **source/** (engine library)
- `app/` → Core engine systems (App, Plugin, ECS)
- `rendering/` → Rendering systems and components
- `opengl/` → OpenGL wrappers and utilities
- `physics/` → Physics components and systems
- `input/` → Input handling systems
- Exclude: `editor/` (moved to standalone)

**VIVID_D/src/editor/** → **standalone/source/** (editor application)
- SceneHierarchyPanel, InspectorPanel → Editor UI
- ComponentRegistry → Editor-specific component management
- editor_plugin.cpp → Editor plugin implementation

### Dependency Management Changes
**From VIVID_D (pkgconfig/FetchContent) → VIVID (CPM.cmake)**
- `find_package(glfw3)` → `CPMAddPackage("gh:glfw/glfw@3.3.8")`
- `find_package(GLEW)` → `CPMAddPackage("gh:Perlmint/glew-cmake@2.2.0")`
- `FetchContent(glm)` → `CPMAddPackage("gh:g-truc/glm@0.9.9.8")`
- `FetchContent(EnTT)` → `CPMAddPackage("gh:skypjack/entt@3.12.2")`
- PhysX/Optick → Keep as-is (pre-compiled binaries)

### Key Files to Update

**CMakeLists.txt** (root): Convert to library target
```cmake
# Replace Greeter with VIVID
add_library(VIVID ${headers} ${sources})
target_link_libraries(VIVID PRIVATE fmt::glm::glm EnTT::EnTT)
```

**standalone/CMakeLists.txt**: Editor executable
```cmake
CPMAddPackage(NAME VIVID SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(VIVIDEditor VIVID::VIVID cxxopts)
```

**test/CMakeLists.txt**: Test suite for engine
```cmake
CPMAddPackage(NAME VIVID SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(VIVIDTests doctest::doctest VIVID::VIVID)
```

## Migration Checklist

### Phase 1: Setup Engine Library
1. Copy VIVID_D/src/* (except editor/) to source/
2. Copy VIVID_D/include/* to include/vivid/
3. Update CMakeLists.txt for library target
4. Set up proper include paths and namespace

### Phase 2: Create Editor Standalone
1. Copy VIVID_D/src/editor/* to standalone/source/
2. Update CMakeLists.txt for executable target
3. Link against VIVID engine library
4. Update include paths for new structure

### Phase 3: Dependency Migration
1. Replace FetchContent with CPMAddPackage
2. Update all find_package calls to CPM
3. Ensure PhysX/Optick integration works
4. Test build with new dependency system

### Phase 4: Component Registration
1. Move component registration to engine
2. Update editor to use engine's registration system
3. Ensure ImGui integration works correctly

## Key Architecture Decisions

**Engine Library (VIVID)**: Core systems, rendering, physics, ECS
- Header-only components in include/vivid/
- Implementation in source/
- Installable target with proper versioning

**Editor Application**: Standalone GUI application
- Uses VIVID engine as dependency
- Implements editor-specific UI and tools
- Links against cxxopts for CLI arguments

**Testing**: Unit tests for engine functionality
- Separate from editor tests
- Uses doctest framework
- Tests core engine systems and components