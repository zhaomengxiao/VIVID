# Check C++ code quality using clang-tidy
# Usage: .\scripts\check_code.ps1

param(
    [switch]$Fix,         # Automatically fix issues
    [switch]$Lib,         # Only check lib directory
    [switch]$Standalone,  # Only check standalone directory
    [switch]$Test,        # Only check test directory
    [string]$Checks = "", # Custom checks (e.g., '-*,readability-identifier-naming')
    [switch]$Verbose      # Show detailed output
)

$ErrorActionPreference = "Stop"

Write-Host "=== Clang-Tidy Code Quality Check ===" -ForegroundColor Cyan
Write-Host ""

# Check if clang-tidy is available
try {
    $version = clang-tidy --version 2>&1 | Select-String "LLVM version" | Select-Object -First 1
    Write-Host "✓ Found clang-tidy: $version" -ForegroundColor Green
}
catch {
    Write-Host "✗ clang-tidy not found in PATH!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install LLVM first:" -ForegroundColor Yellow
    Write-Host "  1. Visit: https://github.com/llvm/llvm-project/releases" -ForegroundColor Yellow
    Write-Host "  2. Download and install LLVM for Windows" -ForegroundColor Yellow
    Write-Host "  3. Make sure 'Add LLVM to PATH' is checked" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Or see .vscode/SETUP_CLANG_TIDY.md for detailed instructions" -ForegroundColor Yellow
    exit 1
}

# Check for compile_commands.json
$compileCommandsPath = "build/compile_commands.json"
if (-not (Test-Path $compileCommandsPath)) {
    Write-Host "✗ compile_commands.json not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please generate it first:" -ForegroundColor Yellow
    Write-Host "  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build" -ForegroundColor Cyan
    Write-Host ""
    exit 1
}

Write-Host "✓ Found compile_commands.json" -ForegroundColor Green
Write-Host ""

# Determine which directories to check
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
    # Default: check lib, standalone, and test
    $directories += "lib", "standalone", "test"
}

Write-Host "Checking directories: $($directories -join ', ')" -ForegroundColor Cyan
Write-Host ""

# Build clang-tidy command arguments
$tidyArgs = @("-p", "build")

if ($Fix) {
    $tidyArgs += "--fix"
    Write-Host "⚠ Auto-fix mode enabled - files will be modified!" -ForegroundColor Yellow
    Write-Host ""
}

if ($Checks) {
    $tidyArgs += "--checks=$Checks"
    Write-Host "Using custom checks: $Checks" -ForegroundColor Cyan
}

# Collect files to check
$filesToCheck = @()
foreach ($dir in $directories) {
    if (-not (Test-Path $dir)) {
        Write-Host "Warning: Directory '$dir' not found, skipping..." -ForegroundColor Yellow
        continue
    }
    
    $files = Get-ChildItem -Path $dir -Recurse -Include *.cpp, *.h, *.hpp -File
    $filesToCheck += $files
}

if ($filesToCheck.Count -eq 0) {
    Write-Host "✗ No files found to check!" -ForegroundColor Red
    exit 1
}

Write-Host "Found $($filesToCheck.Count) file(s) to check" -ForegroundColor Cyan
Write-Host ""

# Check each file
$issueCount = 0
$fileCount = 0

foreach ($file in $filesToCheck) {
    $relativePath = Resolve-Path -Relative $file.FullName
    
    Write-Host "Checking: $relativePath" -ForegroundColor Gray
    
    # Run clang-tidy
    $output = & clang-tidy $file.FullName @tidyArgs 2>&1
    $exitCode = $LASTEXITCODE
    
    if ($Verbose -or $exitCode -ne 0) {
        # Show output if verbose or if there are issues
        $warningLines = $output | Select-String "warning:|error:" 
        if ($warningLines) {
            Write-Host $output -ForegroundColor Yellow
            $issueCount += $warningLines.Count
        }
    }
    
    $fileCount++
}

# Summary
Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host "Files checked: $fileCount" -ForegroundColor White
Write-Host "Issues found: $issueCount" -ForegroundColor $(if ($issueCount -eq 0) { "Green" } else { "Yellow" })

if ($issueCount -eq 0) {
    Write-Host ""
    Write-Host "✓ All checks passed!" -ForegroundColor Green
    exit 0
}
else {
    Write-Host ""
    if ($Fix) {
        Write-Host "⚠ Issues found and attempted to fix. Please review the changes." -ForegroundColor Yellow
    }
    else {
        Write-Host "⚠ Issues found. Run with -Fix flag to auto-fix some of them." -ForegroundColor Yellow
    }
    Write-Host ""
    Write-Host "To check specific issues:" -ForegroundColor Cyan
    Write-Host "  .\scripts\check_code.ps1 -Checks '-*,readability-identifier-naming'" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Common check categories:" -ForegroundColor Cyan
    Write-Host "  -Checks '-*,readability-*'        # Readability issues" -ForegroundColor Gray
    Write-Host "  -Checks '-*,modernize-*'          # Modernization suggestions" -ForegroundColor Gray
    Write-Host "  -Checks '-*,performance-*'        # Performance issues" -ForegroundColor Gray
    Write-Host "  -Checks '-*,bugprone-*'           # Potential bugs" -ForegroundColor Gray
    
    exit 1
}

