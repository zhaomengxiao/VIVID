# Format all C++ source files in the project using clang-format
# Usage: .\scripts\format_code.ps1

param(
    [switch]$CheckOnly,  # Only check, don't modify files
    [switch]$Lib,        # Only format lib directory
    [switch]$Standalone, # Only format standalone directory
    [switch]$Test,       # Only format test directory
    [switch]$All         # Format all directories (default)
)

$ErrorActionPreference = "Stop"

# Check if clang-format is available
try {
    $version = clang-format --version
    Write-Host "✓ Found clang-format: $version" -ForegroundColor Green
}
catch {
    Write-Host "✗ clang-format not found in PATH!" -ForegroundColor Red
    Write-Host "Please install LLVM or add clang-format to PATH" -ForegroundColor Yellow
    Write-Host "See .vscode/SETUP_CLANG_FORMAT.md for installation instructions" -ForegroundColor Yellow
    exit 1
}

# Determine which directories to format
$directories = @()
if ($Lib) {
    $directories += "lib"
}
elseif ($Standalone) {
    $directories += "standalone"
}
elseif ($Test) {
    $directories += "test"
}
else {
    # Default: format lib, standalone, and test
    $directories += "lib", "standalone", "test"
}

Write-Host "`nFormatting C++ files in: $($directories -join ', ')" -ForegroundColor Cyan

$fileCount = 0
$errorCount = 0

foreach ($dir in $directories) {
    if (-not (Test-Path $dir)) {
        Write-Host "Warning: Directory '$dir' not found, skipping..." -ForegroundColor Yellow
        continue
    }

    Write-Host "`nProcessing $dir directory..." -ForegroundColor Cyan
    
    $files = Get-ChildItem -Path $dir -Recurse -Include *.cpp, *.h, *.hpp -File
    
    foreach ($file in $files) {
        $relativePath = Resolve-Path -Relative $file.FullName
        
        if ($CheckOnly) {
            # Check only mode
            $result = clang-format --dry-run --Werror $file.FullName 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Host "  ✗ $relativePath needs formatting" -ForegroundColor Red
                $errorCount++
            }
            else {
                Write-Host "  ✓ $relativePath" -ForegroundColor Green
            }
        }
        else {
            # Format mode
            Write-Host "  Formatting: $relativePath" -ForegroundColor Gray
            clang-format -i $file.FullName
            if ($LASTEXITCODE -eq 0) {
                $fileCount++
            }
            else {
                Write-Host "  ✗ Failed to format: $relativePath" -ForegroundColor Red
                $errorCount++
            }
        }
    }
}

Write-Host ""
if ($CheckOnly) {
    if ($errorCount -eq 0) {
        Write-Host "✓ All files are properly formatted!" -ForegroundColor Green
        exit 0
    }
    else {
        Write-Host "✗ $errorCount file(s) need formatting" -ForegroundColor Red
        Write-Host "Run without -CheckOnly flag to format them" -ForegroundColor Yellow
        exit 1
    }
}
else {
    if ($errorCount -eq 0) {
        Write-Host "✓ Successfully formatted $fileCount file(s)" -ForegroundColor Green
        exit 0
    }
    else {
        Write-Host "✗ Formatted $fileCount file(s), $errorCount error(s)" -ForegroundColor Red
        exit 1
    }
}



