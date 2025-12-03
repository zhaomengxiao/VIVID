@echo off
REM Build script for Emscripten WebAssembly target
REM This script activates the Emscripten environment before building

setlocal enabledelayedexpansion

set EMSDK_PATH=D:\ClineWorkSpace\emsdk
set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build\emscripten

echo ==== VIVID Emscripten Build Script ====
echo Project Root: %PROJECT_ROOT%
echo Emsdk Path: %EMSDK_PATH%
echo Build Directory: %BUILD_DIR%
echo.

REM Check if emsdk exists
if not exist "%EMSDK_PATH%\emsdk_env.bat" (
    echo Error: Emscripten SDK not found at %EMSDK_PATH%
    echo Please update the EMSDK_PATH variable in this script.
    exit /b 1
)

REM Parse command line arguments
set CLEAN=0
if "%1"=="-clean" set CLEAN=1
if "%1"=="--clean" set CLEAN=1

REM Clean build directory if requested
if %CLEAN%==1 (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" (
        rmdir /s /q "%BUILD_DIR%"
        echo [32m✓ Build directory cleaned[0m
    ) else (
        echo [32m✓ Build directory doesn't exist, nothing to clean[0m
    )
    echo.
)

REM Change to project root
cd /d "%PROJECT_ROOT%"

REM Activate Emscripten environment
echo Activating Emscripten environment...
call "%EMSDK_PATH%\emsdk_env.bat"
if errorlevel 1 (
    echo [31mError: Failed to activate Emscripten environment[0m
    exit /b 1
)
echo [32m✓ Emscripten environment activated[0m
echo.

REM Configure with CMake
echo Configuring project with CMake...
cmake --preset VE
if errorlevel 1 (
    echo [31mError: CMake configuration failed[0m
    exit /b 1
)
echo [32m✓ CMake configuration successful[0m
echo.

REM Build with CMake
echo Building project...
cmake --build --preset VE
if errorlevel 1 (
    echo [31mError: Build failed[0m
    exit /b 1
)
echo [32m✓ Build successful[0m
echo.

REM Show output files
if exist "%BUILD_DIR%\hello_sdl3\index.html" (
    echo ==== Build Output ====
    echo Output files:
    dir /b "%BUILD_DIR%\hello_sdl3\index.*"
    echo.
    echo To run the WebAssembly application:
    echo   1. Start a local web server in the build directory
    echo   2. Open http://localhost:8000/hello_sdl3/index.html in your browser
    echo.
    echo Example using Python:
    echo   cd %BUILD_DIR%
    echo   python -m http.server 8000
    echo.
)

echo ==== Done ====

endlocal


