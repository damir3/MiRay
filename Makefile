.PHONY: all conan configure build test run clean lsp arm arm64 intel release debug project xcode vs app bundle dmg

# Default architecture and configuration
CONFIG ?= RelWithDebInfo
ARCH ?= $(shell uname -m | sed -e 's/aarch64/arm/' -e 's/arm64/arm/')
BIN_DIR ?= $(ARCH)
PROFILE ?= conan/mac-$(ARCH)
EXE ?= MiRay.app/Contents/MacOS/MiRay

ifeq ($(OS),Windows_NT)
	WIN_ARCH := $(PROCESSOR_ARCHITECTURE)
	ifneq ($(PROCESSOR_ARCHITEW6432),)
		WIN_ARCH := $(PROCESSOR_ARCHITEW6432)
	endif
	ifeq ($(WIN_ARCH),ARM64)
		ARCH := arm64
		BIN_DIR := arm64
		VS_ARCH := ARM64
		PROFILE := conan/win-arm64
	else
		ARCH := 64
		BIN_DIR := 64
		VS_ARCH := x64
		PROFILE := conan/win-x64
	endif
	EXE := MiRay.exe
endif

# Parse MAKECMDGOALS to allow modifiers like 'make arm release build' or 'make debug test'
ifneq ($(filter arm arm64,$(MAKECMDGOALS)),)
ifeq ($(OS),Windows_NT)
	ARCH := arm64
	BIN_DIR := arm64
	VS_ARCH := ARM64
	PROFILE := conan/win-arm64
else
	ARCH := arm
	BIN_DIR := arm
	PROFILE := conan/mac-arm
endif
endif

ifneq ($(filter intel x64,$(MAKECMDGOALS)),)
ifeq ($(OS),Windows_NT)
	ARCH := 64
	BIN_DIR := 64
	VS_ARCH := x64
	PROFILE := conan/win-x64
else
	ARCH := x64
	BIN_DIR := 64
	PROFILE := conan/mac-x64
endif
endif

ifneq ($(filter debug,$(MAKECMDGOALS)),)
	CONFIG := Debug
endif

ifneq ($(filter release,$(MAKECMDGOALS)),)
	CONFIG := RelWithDebInfo
endif

# Default configuration for IDE projects (Xcode, Visual Studio) is Debug
IDE_CONFIG ?= Debug
ifneq ($(filter release,$(MAKECMDGOALS)),)
	IDE_CONFIG := RelWithDebInfo
endif
ifneq ($(filter debug,$(MAKECMDGOALS)),)
	IDE_CONFIG := Debug
endif
ifneq ($(filter both all,$(MAKECMDGOALS)),)
	IDE_CONFIG := both
endif

all: build

arm arm64 intel x64 release debug both all:
	@:

conan:
ifeq ($(OS),Windows_NT)
	@conan remote add shared conan -t local-recipes-index -f >nul 2>&1 || conan remote add shared conan -t local-recipes-index
else
	@conan remote add shared ./conan -t local-recipes-index -f >/dev/null 2>&1 || conan remote add shared ./conan -t local-recipes-index
endif
	@echo "==> Running Conan 2 for $(ARCH) ($(CONFIG))..."
	conan install . --build=missing -pr:h=$(PROFILE) -pr:b=default -s build_type=$(CONFIG) --core-conf="core.package_id:default_non_embed_mode=full_mode"

configure: conan
ifeq ($(OS),Windows_NT)
	@echo "==> Configuring CMake with Visual Studio 2022 for $(ARCH) ($(CONFIG))..."
	@if exist tmp\CMakeCache.txt (findstr /c:"CMAKE_GENERATOR:INTERNAL=Visual Studio" tmp\CMakeCache.txt >nul || (echo ==> Generator mismatch in tmp/, cleaning cache for Visual Studio... && del /f /q tmp\CMakeCache.txt && rmdir /s /q tmp\CMakeFiles))
	cmake -B tmp -G "Visual Studio 17 2022" -A $(VS_ARCH) -DCMAKE_TOOLCHAIN_FILE=tmp/conan_toolchain.cmake
else
	@echo "==> Configuring CMake with Xcode for $(ARCH) ($(CONFIG))..."
	@if [ -f tmp/CMakeCache.txt ] && ! grep -q "CMAKE_GENERATOR:INTERNAL=Xcode" tmp/CMakeCache.txt; then echo "==> Generator mismatch in tmp/ (was not Xcode), cleaning cache for Xcode..."; rm -rf tmp/CMakeCache.txt tmp/CMakeFiles; fi
	cmake -B tmp -G Xcode -DCMAKE_TOOLCHAIN_FILE=tmp/conan_toolchain.cmake
endif

build: configure
	@echo "==> Building project ($(ARCH), $(CONFIG))..."
	cmake --build tmp --config $(CONFIG) -j

test:
	@echo "==> Running tests for $(ARCH)..."
ifeq ($(OS),Windows_NT)
	bin\$(BIN_DIR)\Core.Tests.exe
else
	./bin/$(BIN_DIR)/Core.Tests
endif

run:
	bin/$(BIN_DIR)/$(EXE)

clean:
	@echo "==> Cleaning bin and tmp directories..."
	rm -rf bin tmp

lsp: conan
	@echo "==> Generating compile_commands.json for $(ARCH) ($(CONFIG))..."
	@if [ -f tmp/lsp/CMakeCache.txt ] && ! grep -q "CMAKE_GENERATOR:INTERNAL=Unix Makefiles" tmp/lsp/CMakeCache.txt; then \
		echo "==> Generator mismatch in tmp/lsp/, cleaning cache..."; \
		rm -rf tmp/lsp/CMakeCache.txt tmp/lsp/CMakeFiles; \
	fi
	cmake -B tmp/lsp -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=tmp/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$(CONFIG) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@echo "==> Generated tmp/lsp/compile_commands.json"

project:
ifeq ($(IDE_CONFIG),both)
	@$(MAKE) conan CONFIG=Debug
	@$(MAKE) conan CONFIG=RelWithDebInfo
else
	@$(MAKE) conan CONFIG=$(IDE_CONFIG)
endif
ifeq ($(OS),Windows_NT)
	@echo "==> Generating Visual Studio 2022 solution for $(ARCH)..."
	@if exist tmp\CMakeCache.txt (findstr /c:"CMAKE_GENERATOR:INTERNAL=Visual Studio" tmp\CMakeCache.txt >nul || (echo ==> Generator mismatch in tmp/, cleaning cache for Visual Studio... && del /f /q tmp\CMakeCache.txt && rmdir /s /q tmp\CMakeFiles))
	cmake -B tmp -G "Visual Studio 17 2022" -A $(VS_ARCH) -DCMAKE_TOOLCHAIN_FILE=tmp/conan_toolchain.cmake
	@echo ""
	@echo "========================================================="
	@echo "  Visual Studio solution ($(ARCH)) generated at: tmp/MiRay.sln"
	@echo "  Open in Visual Studio: start tmp\MiRay.sln"
	@echo "========================================================="
else
	@echo "==> Generating Xcode project for $(ARCH)..."
	@if [ -f tmp/CMakeCache.txt ] && ! grep -q "CMAKE_GENERATOR:INTERNAL=Xcode" tmp/CMakeCache.txt; then echo "==> Generator mismatch in tmp/ (was not Xcode), cleaning cache for Xcode..."; rm -rf tmp/CMakeCache.txt tmp/CMakeFiles; fi
	cmake -B tmp -G Xcode -DCMAKE_TOOLCHAIN_FILE=tmp/conan_toolchain.cmake
	@echo ""
	@echo "========================================================="
	@echo "  Xcode project ($(ARCH)) generated at: tmp/MiRay.xcodeproj"
	@echo "  Open in Xcode: open tmp/MiRay.xcodeproj"
	@echo "========================================================="
endif

app: build
ifeq ($(OS),Windows_NT)
	@cmd /c tools\win\bundle.cmd $(ARCH)
else
	@bash tools/mac/bundle.sh $(ARCH)
endif

bundle: app

dmg: app
ifeq ($(OS),Windows_NT)
	@echo "==> DMG creation is only applicable for macOS"
else
	@bash tools/mac/installer/create_dmg.sh $(ARCH)
endif

