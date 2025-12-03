# Build script for Emscripten WebAssembly target
# This script activates the Emscripten environment before building

param(
    [switch]$Clean,
    [switch]$ConfigureOnly,
    [switch]$BuildOnly
)

$ErrorActionPreference = "Stop"

# Project root directory (parent of scripts folder)
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$EmsdkPath = "D:\ClineWorkSpace\emsdk"
$BuildDir = Join-Path $ProjectRoot "build\emscripten"

Write-Host "==== VIVID Emscripten Build Script ====" -ForegroundColor Cyan
Write-Host "Project Root: $ProjectRoot" -ForegroundColor Gray
Write-Host "Emsdk Path: $EmsdkPath" -ForegroundColor Gray
Write-Host "Build Directory: $BuildDir" -ForegroundColor Gray
Write-Host ""

# Check if emsdk exists
if (-not (Test-Path "$EmsdkPath\emsdk_env.ps1")) {
    Write-Host "Error: Emscripten SDK not found at $EmsdkPath" -ForegroundColor Red
    Write-Host "Please update the `$EmsdkPath variable in this script." -ForegroundColor Yellow
    exit 1
}

# Clean build directory if requested
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Host "✓ Build directory cleaned" -ForegroundColor Green
    }
    else {
        Write-Host "✓ Build directory doesn't exist, nothing to clean" -ForegroundColor Green
    }
    Write-Host ""
}

# Change to project root
Push-Location $ProjectRoot

try {
    # Activate Emscripten environment
    Write-Host "Activating Emscripten environment..." -ForegroundColor Yellow
    & "$EmsdkPath\emsdk_env.ps1"
    
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to activate Emscripten environment"
    }
    Write-Host "✓ Emscripten environment activated" -ForegroundColor Green
    Write-Host ""

    # Configure with CMake if not BuildOnly
    if (-not $BuildOnly) {
        Write-Host "Configuring project with CMake..." -ForegroundColor Yellow
        cmake --preset VE
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
        Write-Host "✓ CMake configuration successful" -ForegroundColor Green
        Write-Host ""
    }

    # Build with CMake if not ConfigureOnly
    if (-not $ConfigureOnly) {
        Write-Host "Building project..." -ForegroundColor Yellow
        cmake --build --preset VE
        
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
        Write-Host "✓ Build successful" -ForegroundColor Green
        Write-Host ""
        
        # Show output files
        if (Test-Path "$BuildDir\hello_sdl3\index.html") {
            Write-Host "==== Build Output ====" -ForegroundColor Cyan
            Write-Host "Output files:" -ForegroundColor Gray
            Get-ChildItem "$BuildDir\hello_sdl3" -Filter "index.*" | ForEach-Object {
                Write-Host "  - $($_.FullName)" -ForegroundColor Gray
            }
            Write-Host ""
            Write-Host "To run the WebAssembly application:" -ForegroundColor Yellow
            Write-Host "  1. Start a local web server in the build directory" -ForegroundColor Gray
            Write-Host "  2. Open http://localhost:8000/hello_sdl3/index.html in your browser" -ForegroundColor Gray
            Write-Host ""
            Write-Host "Example using Python:" -ForegroundColor Yellow
            Write-Host "  cd $BuildDir" -ForegroundColor Gray
            Write-Host "  python -m http.server 8000" -ForegroundColor Gray
        }
    }
    
    Write-Host "==== Done ====" -ForegroundColor Cyan
    
}
catch {
    Write-Host ""
    Write-Host "Error: $_" -ForegroundColor Red
    Pop-Location
    exit 1
}
finally {
    Pop-Location
}


