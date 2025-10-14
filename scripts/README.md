# Scripts Directory

This directory contains utility scripts for the VIVID project.

## Available Scripts

### format_code.ps1

Batch format C++ source files using clang-format.

**Usage:**

```powershell
# Format all code (lib, standalone, test)
.\scripts\format_code.ps1

# Only check without modifying
.\scripts\format_code.ps1 -CheckOnly

# Format specific directory
.\scripts\format_code.ps1 -Lib
.\scripts\format_code.ps1 -Standalone
.\scripts\format_code.ps1 -Test
```

**Requirements:**

- clang-format must be installed and in PATH
- See `.vscode/SETUP_CLANG_FORMAT.md` for installation instructions

**Examples:**

```powershell
# Check if all files are formatted correctly
.\scripts\format_code.ps1 -CheckOnly

# Format only library code
.\scripts\format_code.ps1 -Lib

# Format everything
.\scripts\format_code.ps1
```

---

### check_code.ps1

Check C++ code quality using clang-tidy (naming rules, potential bugs, performance issues, etc.).

**Usage:**

```powershell
# Check all code (lib, standalone, test)
.\scripts\check_code.ps1

# Check specific directory
.\scripts\check_code.ps1 -Lib
.\scripts\check_code.ps1 -Standalone
.\scripts\check_code.ps1 -Test

# Auto-fix issues (be careful!)
.\scripts\check_code.ps1 -Fix

# Check specific categories
.\scripts\check_code.ps1 -Checks '-*,readability-identifier-naming'
.\scripts\check_code.ps1 -Checks '-*,modernize-*'
.\scripts\check_code.ps1 -Checks '-*,performance-*'

# Verbose output
.\scripts\check_code.ps1 -Verbose
```

**Requirements:**

- clang-tidy must be installed and in PATH
- `compile_commands.json` must be generated:
  ```powershell
  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
  ```
- See `.vscode/SETUP_CLANG_TIDY.md` for installation instructions

**Examples:**

```powershell
# Check naming conventions only
.\scripts\check_code.ps1 -Checks '-*,readability-identifier-naming,google-readability-naming'

# Check and auto-fix modernization issues
.\scripts\check_code.ps1 -Fix -Checks '-*,modernize-*'

# Full check with detailed output
.\scripts\check_code.ps1 -Verbose

# Check library code only
.\scripts\check_code.ps1 -Lib
```

---

## Recommended Workflow

### Before Committing Code

```powershell
# 1. Format code
.\scripts\format_code.ps1

# 2. Check code quality
.\scripts\check_code.ps1

# 3. Fix any issues and re-check
.\scripts\check_code.ps1 -Fix
```

### Focus on Specific Issues

```powershell
# Only check naming rules
.\scripts\check_code.ps1 -Checks '-*,readability-identifier-naming'

# Only check performance issues
.\scripts\check_code.ps1 -Checks '-*,performance-*'

# Only check potential bugs
.\scripts\check_code.ps1 -Checks '-*,bugprone-*'
```
