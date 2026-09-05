# MiRay2 Build & Distribution Guide

Comprehensive guide for configuring, building, testing, and packaging the **MiRay2** rendering engine across macOS and Windows.

---

## Table of Contents

- [Prerequisites & Environment Setup](#prerequisites--environment-setup)
- [Quick Start via Makefile](#quick-start-via-makefile)
  - [Common Makefile Targets](#common-makefile-targets)
  - [Build Flavors & Modifiers](#build-flavors--modifiers)
- [IDE Project Generation](#ide-project-generation)
  - [macOS: Xcode Project](#macos-xcode-project)
  - [Windows: Visual Studio 2022 Solution](#windows-visual-studio-2022-solution)
- [Packaging Standalone Applications (`make app`)](#packaging-standalone-applications-make-app)
  - [macOS: `tools/mac/bundle.sh`](#macos-toolsmacbundlesh)
  - [Windows: `tools/win/bundle.cmd`](#windows-toolswinbundlecmd)

---

## Prerequisites & Environment Setup

Before building or running scripts, ensure your workstation satisfies the following requirements:

| Tool | Version / Requirement | Installation / Notes |
|---|---|---|
| **OS (macOS)** | macOS 11.0+ (`arm64` or `x86_64`) | Supported on Apple Silicon and Intel |
| **OS (Windows)** | Windows 10/11 (64-bit) | Supported on `x64` |
| **C++ Compiler (macOS)** | AppleClang 15+ / Xcode 15+ | `xcode-select --install` |
| **C++ Compiler (Windows)** | MSVC v143+ (Visual Studio 2022) | "Desktop development with C++" workload |
| **CMake** | 3.28 or later | `brew install cmake` or [cmake.org](https://cmake.org) |
| **Python** | 3.10 or later | Required for Conan 2 |
| **Conan** | 2.0 or later | `pip install "conan>=2.0"` |

Verify tools from your terminal:
```bash
conan --version    # Should print: Conan version 2.x.x
cmake --version    # Should print: cmake version 3.28.x or newer
python3 --version  # Should print: Python 3.10.x or newer
```

---

## Quick Start via Makefile

A root [Makefile](../Makefile) wraps the most common workflows into clean, portable commands.

### Common Makefile Targets

```bash
# Install Conan dependencies, configure CMake, and build
make build

# Run unit tests
make test

# Launch the compiled MiRay application executable
make run

# Package standalone application (tools/mac/bundle.sh or tools/win/bundle.cmd)
make app

# Generate symlink to tmp/compile_commands.json for clangd / IDEs
make lsp

# Generate native IDE project (Xcode on macOS, Visual Studio on Windows)
make project

# Clean build output (bin/, tmp/, and compile_commands.json)
make clean
```

### Build Flavors & Modifiers

You can prepend modifier targets to choose the architecture and build configuration:

```bash
# Build RelWithDebInfo (default) for Apple Silicon
make arm release build

# Build Debug configuration for Intel x86_64
make intel debug build

# Build and immediately run tests with Debug symbols
make debug build test

# Custom variable overrides:
make build CONFIG=Release ARCH=arm
```

Supported modifier targets:
- `release`: Sets `CONFIG=RelWithDebInfo`
- `debug`: Sets `CONFIG=Debug`
- `arm`: Targets Apple Silicon `arm64` (uses `conan/mac-arm` profile, outputs to `bin/arm`)
- `intel` / `x64`: Targets Intel `x86_64` (uses `conan/mac-x64` profile on macOS or `conan/win-x64` on Windows, outputs to `bin/64`)

---

## IDE Project Generation

To develop with full IDE debugging, breakpoints, and symbol navigation, generate native project solutions using `make project`:

### macOS: Xcode Project

Automatically configures Conan 2 for macOS, installs packages, invalidates any mismatched CMake caches, and invokes the native `Xcode` CMake generator.

#### Usage:
```bash
make project                # Generate Xcode project in Debug (auto-detects host architecture)
make release project        # Generate Xcode project in RelWithDebInfo
make both project           # Install both Debug and RelWithDebInfo dependencies
make intel project          # Target Intel x64 architecture
make arm project            # Target Apple Silicon arm64 architecture
```

#### Open in Xcode:
```bash
open tmp/MiRay.xcodeproj
```

---

### Windows: Visual Studio 2022 Solution

Configures Conan 2 for Windows (x64), installs dependencies, cleans previous CMake cache conflicts, and generates a multi-configuration Visual Studio 2022 solution.

#### Usage:
```bash
make project                # Generate Visual Studio 2022 solution in Debug (x64)
make release project        # Generate with RelWithDebInfo dependencies
make both project           # Install both Debug and RelWithDebInfo dependencies
make intel project          # Target Windows x64 architecture
```

#### Open in Visual Studio:
```cmd
start tmp\MiRay.sln
```

---

## Packaging Standalone Applications (`make app`)

Creates a standalone, distributable application directory with all Qt runtime libraries, plugins, and scene assets deployed alongside the binary.

```
tools/
├── mac/
│   └── bundle.sh               # macOS: bundles Qt runtime and plugins into MiRay.app
└── win/
    ├── bundle.cmd              # Windows: packages runtime DLLs, qt.conf, and resources
    ├── version.rc.in           # Windows: MSVC resource version template
    └── qt.conf                 # Windows: Qt library deployment configuration
```

---

### macOS: `tools/mac/bundle.sh`

Deploys dynamic libraries, Qt plugins, and QML modules into `bin/<ARCH>/MiRay.app` using `macdeployqt`.

**Usage:**
```bash
# Via Makefile (recommended):
make app

# Or directly:
bash tools/mac/bundle.sh [ARCH]
```
- `ARCH` *(optional, default: `arm`)*: Target architecture (`arm` or `intel`).

---

### Windows: `tools/win/bundle.cmd`

Prepares `bin\<ARCH>\MiRay.exe` for standalone execution by deploying `qt.conf`, syncing preview and library resources, and cleaning temporary cache.

**Usage:**
```cmd
:: Via Makefile (recommended):
make app

:: Or directly:
tools\win\bundle.cmd [PLATFORM]
```
- `PLATFORM` *(optional, default: `64`)*: Target architecture (`64`).

