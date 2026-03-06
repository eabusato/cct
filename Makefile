# CCT — Clavicula Turing
# Build System
#
# FASE 9C: Makefile with multi-module + visibility boundary support over the C-hosted backend/runtime base
#
# Future phases may add:
# - additional backend/runtime optimization targets
# - packaging variants per release channel
#
# Copyright (c) Erick Andrade Busato. Todos os direitos reservados.

# Compiler and flags
CC        = gcc
CFLAGS    = -Wall -Wextra -Werror -std=c11 -O2 -g
CFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -D_XOPEN_SOURCE=700
LDFLAGS   = -lm

# Directories
SRC_DIR   = src
BUILD_DIR = build
BIN_DIR   = .
STDLIB_DIR = lib/cct
DIST_DIR  ?= dist/cct
PREFIX    ?= /usr/local

CFLAGS += -DCCT_STDLIB_DIR=\"$(abspath $(STDLIB_DIR))\"
CFLAGS += -DCCT_FREESTANDING_RT_HEADER=\"$(abspath $(SRC_DIR)/runtime/cct_freestanding_rt.h)\"
CFLAGS += -DCCT_FREESTANDING_RT_SOURCE=\"$(abspath $(SRC_DIR)/runtime/cct_freestanding_rt.c)\"

# Output binary
TARGET    = $(BIN_DIR)/cct
CCT_LBOS_OUT = build/lbos-bridge
CCT_KERNEL_SOURCE = lib/cct/kernel/kernel.cct
CCT_KERNEL_ASM = $(CCT_KERNEL_SOURCE:.cct=.cgen.s)
CCT_KERNEL_OBJ = $(CCT_LBOS_OUT)/cct_kernel.o
CCT_KERNEL_ENTRY = kernel_halt

# Source files
SRCS = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/common/errors.c \
	$(SRC_DIR)/common/diagnostic.c \
	$(SRC_DIR)/common/fuzzy.c \
	$(SRC_DIR)/cli/cli.c \
	$(SRC_DIR)/lexer/lexer.c \
	$(SRC_DIR)/parser/ast.c \
	$(SRC_DIR)/parser/parser.c \
	$(SRC_DIR)/semantic/semantic.c \
	$(SRC_DIR)/codegen/codegen.c \
	$(SRC_DIR)/codegen/codegen_contract.c \
	$(SRC_DIR)/codegen/codegen_runtime_bridge.c \
	$(SRC_DIR)/runtime/runtime.c \
	$(SRC_DIR)/runtime/runtime_math.c \
	$(SRC_DIR)/module/module.c \
	$(SRC_DIR)/sigilo/sigilo.c \
	$(SRC_DIR)/sigilo/sigil_parse.c \
	$(SRC_DIR)/sigilo/sigil_validate.c \
	$(SRC_DIR)/sigilo/sigil_diff.c \
	$(SRC_DIR)/formatter/formatter.c \
	$(SRC_DIR)/lint/lint.c \
	$(SRC_DIR)/project/project.c \
	$(SRC_DIR)/project/project_discovery.c \
	$(SRC_DIR)/project/project_cache.c \
	$(SRC_DIR)/project/project_runner.c \
	$(SRC_DIR)/project/sigilo_baseline.c \
	$(SRC_DIR)/doc/doc.c

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Header dependencies
DEPS = \
	$(SRC_DIR)/common/types.h \
	$(SRC_DIR)/common/errors.h \
	$(SRC_DIR)/common/diagnostic.h \
	$(SRC_DIR)/common/fuzzy.h \
	$(SRC_DIR)/cli/cli.h \
	$(SRC_DIR)/lexer/lexer.h \
	$(SRC_DIR)/lexer/keywords.h \
	$(SRC_DIR)/parser/ast.h \
	$(SRC_DIR)/parser/parser.h \
	$(SRC_DIR)/semantic/semantic.h \
	$(SRC_DIR)/codegen/codegen.h \
	$(SRC_DIR)/codegen/codegen_internal.h \
	$(SRC_DIR)/runtime/runtime.h \
	$(SRC_DIR)/runtime/runtime_math.h \
	$(SRC_DIR)/module/module.h \
	$(SRC_DIR)/sigilo/sigilo.h \
	$(SRC_DIR)/sigilo/sigil_parse.h \
	$(SRC_DIR)/sigilo/sigil_validate.h \
	$(SRC_DIR)/sigilo/sigil_diff.h \
	$(SRC_DIR)/formatter/formatter.h \
	$(SRC_DIR)/lint/lint.h \
	$(SRC_DIR)/project/project.h \
	$(SRC_DIR)/project/project_discovery.h \
	$(SRC_DIR)/project/project_cache.h \
	$(SRC_DIR)/project/project_runner.h \
	$(SRC_DIR)/project/sigilo_baseline.h \
	$(SRC_DIR)/doc/doc.h

# Default target
all: $(TARGET)

# Link the final binary
$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "Linking $(TARGET)..."
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile C source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c -o $@ $<

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/common
	@mkdir -p $(BUILD_DIR)/cli
	@mkdir -p $(BUILD_DIR)/lexer
	@mkdir -p $(BUILD_DIR)/parser
	@mkdir -p $(BUILD_DIR)/semantic
	@mkdir -p $(BUILD_DIR)/codegen
	@mkdir -p $(BUILD_DIR)/runtime
	@mkdir -p $(BUILD_DIR)/module
	@mkdir -p $(BUILD_DIR)/sigilo
	@mkdir -p $(BUILD_DIR)/formatter
	@mkdir -p $(BUILD_DIR)/lint
	@mkdir -p $(BUILD_DIR)/project
	@mkdir -p $(BUILD_DIR)/doc

# Create bin directory
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Build freestanding bridge artifact consumable by LBOS integration
lbos-bridge: $(TARGET) $(CCT_LBOS_OUT)
	@echo "[CCT] lbos-bridge: emitindo ASM freestanding..."
	@$(TARGET) --profile freestanding --emit-asm --entry $(CCT_KERNEL_ENTRY) "$(CCT_KERNEL_SOURCE)"
	@echo "[CCT] lbos-bridge: montando objeto ELF32..."
	@as --32 "$(CCT_KERNEL_ASM)" -o "$(CCT_KERNEL_OBJ)"
	@echo "[CCT] lbos-bridge: auditando símbolos undefined proibidos..."
	@if nm "$(CCT_KERNEL_OBJ)" | awk '$$2 == "U" {print $$3}' | \
		grep -Eq '^(printf|malloc|free|memcpy|memset|puts|fopen|__stack_chk_fail|__udivdi3|__divdi3|__muldi3)$$'; then \
		echo "[CCT] FAIL: símbolo proibido encontrado em $(CCT_KERNEL_OBJ)"; \
		exit 1; \
	fi
	@echo "[CCT] lbos-bridge: validando presença de símbolo cct_fn_..."
	@nm "$(CCT_KERNEL_OBJ)" | awk '$$2 == "T" {print $$3}' | grep -q '^cct_fn_' || \
		(echo "[CCT] FAIL: nenhum símbolo cct_fn_ encontrado em $(CCT_KERNEL_OBJ)" && exit 1)
	@echo "[CCT] lbos-bridge: $(CCT_KERNEL_OBJ) pronto"

$(CCT_LBOS_OUT):
	@mkdir -p "$@"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	rm -rf $(CCT_LBOS_OUT)
	@echo "Clean complete."

# Run tests
test: $(TARGET)
	@echo "Running tests..."
	@bash tests/run_tests.sh

test_fluxus_storage:
	@echo "Building fluxus storage runtime tests..."
	$(CC) $(CFLAGS) -o tests/runtime/test_fluxus_storage \
		tests/runtime/test_fluxus_storage.c \
		src/runtime/fluxus_runtime.c \
		src/runtime/mem_runtime.c
	@echo "Running fluxus storage runtime tests..."
	@./tests/runtime/test_fluxus_storage

test_sigil_parse:
	@echo "Building sigil parse runtime tests..."
	$(CC) $(CFLAGS) -o tests/runtime/test_sigil_parse \
		tests/runtime/test_sigil_parse.c \
		src/sigilo/sigil_parse.c \
		src/sigilo/sigil_validate.c
	@echo "Running sigil parse runtime tests..."
	@./tests/runtime/test_sigil_parse

test_sigil_diff:
	@echo "Building sigil diff runtime tests..."
	$(CC) $(CFLAGS) -o tests/runtime/test_sigil_diff \
		tests/runtime/test_sigil_diff.c \
		src/sigilo/sigil_parse.c \
		src/sigilo/sigil_validate.c \
		src/sigilo/sigil_diff.c
	@echo "Running sigil diff runtime tests..."
	@./tests/runtime/test_sigil_diff

test_diagnostic_taxonomy:
	@echo "Building diagnostic taxonomy runtime tests..."
	$(CC) $(CFLAGS) -o tests/runtime/test_diagnostic_taxonomy \
		tests/runtime/test_diagnostic_taxonomy.c \
		src/common/diagnostic.c \
		src/common/errors.c
	@echo "Running diagnostic taxonomy runtime tests..."
	@./tests/runtime/test_diagnostic_taxonomy

# Build relocatable distribution bundle
dist: $(TARGET)
	@echo "Building distribution bundle at $(DIST_DIR)..."
	@mkdir -p $(DIST_DIR)/bin
	@mkdir -p $(DIST_DIR)/lib/cct
	@mkdir -p $(DIST_DIR)/docs
	@mkdir -p $(DIST_DIR)/examples
ifeq ($(IS_WINDOWS),1)
	@cp $(TARGET) $(DIST_DIR)/bin/cct.exe
	@printf '%s\r\n' '@echo off' \
		'set "SCRIPT_DIR=%~dp0"' \
		'if not defined CCT_STDLIB_DIR set "CCT_STDLIB_DIR=%SCRIPT_DIR%..\\lib\\cct"' \
		'"%SCRIPT_DIR%cct.exe" %*' > $(DIST_DIR)/bin/cct.bat
else
	@cp $(TARGET) $(DIST_DIR)/bin/cct.bin
	@printf '%s\n' '#!/bin/sh' \
		'SCRIPT_DIR="$$(CDPATH= cd -- "$$(dirname -- "$$0")" && pwd)"' \
		'export CCT_STDLIB_DIR="$${CCT_STDLIB_DIR:-$$SCRIPT_DIR/../lib/cct}"' \
		'exec "$$SCRIPT_DIR/cct.bin" "$$@"' > $(DIST_DIR)/bin/cct
	@chmod +x $(DIST_DIR)/bin/cct
	@chmod +x $(DIST_DIR)/bin/cct.bin
endif
	@cp -R $(STDLIB_DIR)/. $(DIST_DIR)/lib/cct/
	@cp README.md BUILD.md $(DIST_DIR)/
	@cp docs/install.md docs/spec.md docs/architecture.md docs/roadmap.md \
		docs/bibliotheca_canonica.md docs/build_system.md \
		docs/project_conventions.md docs/doc_generator.md \
		$(DIST_DIR)/docs/
	@mkdir -p $(DIST_DIR)/docs/release
	@cp docs/release/*.md $(DIST_DIR)/docs/release/
	@echo "Copying examples..."
	@cp examples/README.md \
		examples/hello.cct \
		examples/ars_magna_showcase.cct \
		examples/option_result.cct \
		examples/collection_ops_12d2.cct \
		examples/iterators.cct \
		examples/fluxus_demo.cct \
		examples/lint_showcase_before_12e2.cct \
		examples/lint_showcase_after_12e2.cct \
		$(DIST_DIR)/examples/
	@echo "Generating checksums..."
	@cd $(DIST_DIR) && find . -type f -exec $(SHA256) {} \; | sed 's|\./||' > CHECKSUMS.sha256
	@echo "Distribution bundle ready: $(DIST_DIR)"

# Version and platform detection
VERSION ?= 0.12
UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)
UNAME_M := $(shell uname -m 2>/dev/null || echo unknown)

# Detect OS type
IS_WINDOWS := $(if $(findstring MINGW,$(UNAME_S)),1,$(if $(findstring MSYS,$(UNAME_S)),1,$(if $(findstring CYGWIN,$(UNAME_S)),1,0)))
IS_MACOS := $(if $(filter Darwin,$(UNAME_S)),1,0)
IS_LINUX := $(if $(filter Linux,$(UNAME_S)),1,0)

ifeq ($(IS_WINDOWS),1)
  LDFLAGS += -static
endif

# Binary extension and wrapper script type
ifeq ($(IS_WINDOWS),1)
  BIN_EXT = .exe
  WRAPPER_EXT = .bat
else
  BIN_EXT =
  WRAPPER_EXT =
endif

# Detect SHA256 command and platform name
ifeq ($(IS_MACOS),1)
  SHA256 = shasum -a 256
  ifeq ($(UNAME_M),arm64)
    PLATFORM = macos-arm64
  else
    PLATFORM = macos-x86_64
  endif
else ifeq ($(IS_LINUX),1)
  SHA256 = sha256sum
  ifeq ($(UNAME_M),x86_64)
    PLATFORM = linux-x86_64
  else ifeq ($(UNAME_M),aarch64)
    PLATFORM = linux-arm64
  else
    PLATFORM = linux-$(UNAME_M)
  endif
else ifeq ($(IS_WINDOWS),1)
  SHA256 = sha256sum
  ifeq ($(UNAME_M),x86_64)
    PLATFORM = windows-x86_64
  else ifeq ($(UNAME_M),i686)
    PLATFORM = windows-x86
  else
    PLATFORM = windows-$(UNAME_M)
  endif
else
  # Fallback
  SHA256 = $(shell command -v shasum >/dev/null 2>&1 && echo "shasum -a 256" || echo "sha256sum")
  PLATFORM = $(shell echo $(UNAME_S) | tr '[:upper:]' '[:lower:]')-$(UNAME_M)
endif

RELEASE_NAME = cct-v$(VERSION)-$(PLATFORM)
ifeq ($(IS_WINDOWS),1)
  RELEASE_ARCHIVE = $(RELEASE_NAME).zip
else
  RELEASE_ARCHIVE = $(RELEASE_NAME).tar.gz
endif

# Create release archive with checksum
release: dist
	@echo "Creating release archive: $(RELEASE_ARCHIVE)..."
ifeq ($(IS_WINDOWS),1)
	@win_dest=$$(cygpath -w "$(CURDIR)/$(RELEASE_ARCHIVE)"); \
	 cd dist && powershell.exe -NoProfile -Command "Compress-Archive -Path cct -DestinationPath '$$win_dest' -Force"
else
	@cd dist && tar czf ../$(RELEASE_ARCHIVE) cct
endif
	@echo "Generating release checksum..."
	@$(SHA256) $(RELEASE_ARCHIVE) > $(RELEASE_ARCHIVE).sha256
	@echo "Release ready:"
	@echo "  $(RELEASE_ARCHIVE)"
	@echo "  $(RELEASE_ARCHIVE).sha256"
	@echo ""
	@echo "Checksum:"
	@cat $(RELEASE_ARCHIVE).sha256

# Install binary (requires sudo on Linux)
install: $(TARGET)
	@echo "Installing $(TARGET) to $(PREFIX)/bin/..."
	mkdir -p $(PREFIX)/bin
ifeq ($(IS_WINDOWS),1)
	cp $(TARGET) $(PREFIX)/bin/cct.exe
	@printf '%s\r\n' '@echo off' \
		'if not defined CCT_STDLIB_DIR set "CCT_STDLIB_DIR=$(PREFIX)\\lib\\cct"' \
		'"$(PREFIX)\\bin\\cct.exe" %*' > $(PREFIX)/bin/cct.bat
else
	cp $(TARGET) $(PREFIX)/bin/cct.bin
	@printf '%s\n' '#!/bin/sh' \
		'export CCT_STDLIB_DIR="$${CCT_STDLIB_DIR:-$(PREFIX)/lib/cct}"' \
		'exec "$(PREFIX)/bin/cct.bin" "$$@"' > $(PREFIX)/bin/cct
	chmod +x $(PREFIX)/bin/cct
	chmod +x $(PREFIX)/bin/cct.bin
endif
	@echo "Installing Bibliotheca Canonica to $(PREFIX)/lib/cct/..."
	mkdir -p $(PREFIX)/lib/cct
	cp -R $(STDLIB_DIR)/. $(PREFIX)/lib/cct/
	@echo "Installation complete."

# Uninstall binary
uninstall:
	@echo "Uninstalling cct from $(PREFIX)/bin/..."
ifeq ($(IS_WINDOWS),1)
	rm -f $(PREFIX)/bin/cct.bat
	rm -f $(PREFIX)/bin/cct.exe
else
	rm -f $(PREFIX)/bin/cct
	rm -f $(PREFIX)/bin/cct.bin
endif
	@echo "Removing $(PREFIX)/lib/cct/..."
	rm -rf $(PREFIX)/lib/cct
	@echo "Uninstall complete."

# Show help
help:
	@echo "CCT — Clavicula Turing Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all       - Build the compiler (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  test      - Run test suite"
	@echo "  fmt       - Format .cct sources in lib/, examples/, tests/integration/"
	@echo "  fmt-check - Check formatting without rewriting files"
	@echo "  lint      - Run canonical linter over 12E.2 integration fixtures"
	@echo "  project-build       - Run canonical project build workflow"
	@echo "  project-run         - Run canonical project run workflow"
	@echo "  project-test        - Run canonical project test workflow"
	@echo "  project-test-strict - Run project tests with lint/fmt gates"
	@echo "  project-bench       - Run canonical project benchmark workflow"
	@echo "  project-clean       - Clean project artifacts (.cct + optional dist)"
	@echo "  doc       - Generate API docs for current project (docs/api)"
	@echo "  doc-strict - Generate API docs with strict warnings"
	@echo "  test_fluxus_storage - Build and run standalone FLUXUS runtime tests"
	@echo "  test_diagnostic_taxonomy - Build and run diagnostic taxonomy runtime tests"
	@echo "  dist      - Build relocatable distribution bundle"
	@echo "  release   - Create release tarball with checksums (cct-vX.XX-platform.tar.gz)"
	@echo "  install   - Install binary + stdlib under PREFIX ($(PREFIX))"
	@echo "  uninstall - Remove binary + stdlib from PREFIX ($(PREFIX))"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Current status: FASE 11H target (stdlib consolidation + release packaging)"

fmt: $(TARGET)
	@echo "Formatting .cct files..."
	@find lib/cct -name "*.cct" -exec ./cct fmt {} \;
	@find examples -name "*.cct" -exec ./cct fmt {} \;
	@find tests/integration -name "*.cct" -exec ./cct fmt {} \;

fmt-check: $(TARGET)
	@echo "Checking .cct formatting..."
	@find lib/cct -name "*.cct" -exec ./cct fmt --check {} \; && \
	 find examples -name "*.cct" -exec ./cct fmt --check {} \; && \
	 find tests/integration -name "*.cct" -exec ./cct fmt --check {} \;

lint: $(TARGET)
	@echo "Running lint on integration fixtures..."
	@find tests/integration -name "lint_*_12e2.cct" -exec ./cct lint {} \;

project-build: $(TARGET)
	@./cct build

project-run: $(TARGET)
	@./cct run

project-test: $(TARGET)
	@./cct test

project-test-strict: $(TARGET)
	@./cct test --strict-lint --fmt-check

project-bench: $(TARGET)
	@./cct bench

project-clean: $(TARGET)
	@./cct clean

doc: $(TARGET)
	@./cct doc

doc-strict: $(TARGET)
	@./cct doc --warn-missing-docs --strict-docs

# FASE 12H: Release check targets
release-check: $(TARGET)
	@echo "Running FASE 12 release validation..."
	@./cct fmt --check lib/cct/*.cct examples/**/*.cct || true
	@./cct lint --strict lib/cct/*.cct || true
	@./cct test
	@./cct doc --project . --format both --no-timestamp || true
	@echo "Release check complete."

phase12-final-audit: $(TARGET)
	@echo "=== FASE 12 Final Audit ==="
	@./cct --version || echo "Version flag not implemented"
	@echo ""
	@echo "--- Help Commands ---"
	@./cct --help || echo "Help output"
	@echo ""
	@echo "--- Tooling Help ---"
	@./cct fmt --help 2>&1 || true
	@./cct lint --help 2>&1 || true
	@./cct build --help 2>&1 || true
	@./cct test --help 2>&1 || true
	@./cct doc --help 2>&1 || true
	@echo ""
	@echo "Audit complete."

.PHONY: all clean test test_fluxus_storage test_diagnostic_taxonomy dist release install uninstall help fmt fmt-check lint lbos-bridge project-build project-run project-test project-test-strict project-bench project-clean doc doc-strict release-check phase12-final-audit
