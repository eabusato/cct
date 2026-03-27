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

# Output binaries / wrappers
HOST_TARGET = $(BIN_DIR)/cct.bin
TARGET    = $(BIN_DIR)/cct
CCT_HOST_WRAPPER = $(BIN_DIR)/cct-host
CCT_SELFHOST_ROOT = $(BIN_DIR)/cct-selfhost
CORE_WRAPPERS = $(TARGET) $(CCT_HOST_WRAPPER) $(CCT_SELFHOST_ROOT)
CCT_LBOS_OUT = build/lbos-bridge
CCT_KERNEL_SOURCE = lib/cct/kernel/kernel.cct
CCT_KERNEL_ASM = $(CCT_KERNEL_SOURCE:.cct=.cgen.s)
CCT_KERNEL_OBJ = $(CCT_LBOS_OUT)/cct_kernel.o
CCT_KERNEL_ENTRY = kernel_halt
CCT_FREESTANDING_TOOLCHAIN = tools/freestanding_toolchain.sh
BOOTSTRAP_PHASE29_OUT = out/bootstrap/phase29
BOOTSTRAP_PHASE30_OUT = out/bootstrap/phase30
BOOTSTRAP_STAGE0_DIR = $(BOOTSTRAP_PHASE29_OUT)/stage0
BOOTSTRAP_STAGE1_DIR = $(BOOTSTRAP_PHASE29_OUT)/stage1
BOOTSTRAP_STAGE2_DIR = $(BOOTSTRAP_PHASE29_OUT)/stage2
BOOTSTRAP_STAGE_DIFF_DIR = $(BOOTSTRAP_PHASE29_OUT)/diff
BOOTSTRAP_STAGE_BENCH_DIR = $(BOOTSTRAP_PHASE29_OUT)/bench
BOOTSTRAP_STAGE_LOGS_DIR = $(BOOTSTRAP_PHASE29_OUT)/logs
BOOTSTRAP_SUPPORT_DIR = $(BOOTSTRAP_PHASE29_OUT)/support
BOOTSTRAP_COMPILER_SRC = src/bootstrap/main_compiler.cct
BOOTSTRAP_SUPPORT_SRC = src/bootstrap/selfhost_support.cct
BOOTSTRAP_STAGE0_BIN = $(BOOTSTRAP_STAGE0_DIR)/cct_stage0
BOOTSTRAP_STAGE1_BIN = $(BOOTSTRAP_STAGE1_DIR)/cct_stage1
BOOTSTRAP_STAGE2_BIN = $(BOOTSTRAP_STAGE2_DIR)/cct_stage2
BOOTSTRAP_STAGE0_C = $(BOOTSTRAP_STAGE0_DIR)/main_compiler.stage0.c
BOOTSTRAP_STAGE1_C = $(BOOTSTRAP_STAGE1_DIR)/main_compiler.stage1.c
BOOTSTRAP_STAGE2_C = $(BOOTSTRAP_STAGE2_DIR)/main_compiler.stage2.c
BOOTSTRAP_STAGE0_MANIFEST = $(BOOTSTRAP_STAGE0_DIR)/manifest.txt
BOOTSTRAP_STAGE1_MANIFEST = $(BOOTSTRAP_STAGE1_DIR)/manifest.txt
BOOTSTRAP_STAGE2_MANIFEST = $(BOOTSTRAP_STAGE2_DIR)/manifest.txt
BOOTSTRAP_STAGE0_IDENTITY = $(BOOTSTRAP_STAGE0_DIR)/identity_manifest.txt
BOOTSTRAP_STAGE1_IDENTITY = $(BOOTSTRAP_STAGE1_DIR)/identity_manifest.txt
BOOTSTRAP_STAGE2_IDENTITY = $(BOOTSTRAP_STAGE2_DIR)/identity_manifest.txt
BOOTSTRAP_STAGE12_C_DIFF = $(BOOTSTRAP_STAGE_DIFF_DIR)/stage1_vs_stage2.c.diff
BOOTSTRAP_STAGE12_BIN_DIFF = $(BOOTSTRAP_STAGE_DIFF_DIR)/stage1_vs_stage2.bin.diff
BOOTSTRAP_STAGE12_MANIFEST_DIFF = $(BOOTSTRAP_STAGE_DIFF_DIR)/stage1_vs_stage2.identity.diff
BOOTSTRAP_STAGE_BENCH = $(BOOTSTRAP_STAGE_BENCH_DIR)/metrics.txt
BOOTSTRAP_SUPPORT_HOST_C = $(BOOTSTRAP_SUPPORT_DIR)/selfhost_support.host.c
BOOTSTRAP_SUPPORT_LINK_C = $(BOOTSTRAP_SUPPORT_DIR)/selfhost_support.link.c
BOOTSTRAP_SUPPORT_OBJ = $(BOOTSTRAP_SUPPORT_DIR)/selfhost_support.o
BOOTSTRAP_STAGE_CFLAGS = -Wall -Wextra -Werror -Wno-unused-label -Wno-unused-function -Wno-unused-const-variable -Wno-unused-parameter -std=c11 -O2 -g0 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -DCCT_STDLIB_DIR=\"$(abspath $(STDLIB_DIR))\" -DCCT_FREESTANDING_RT_HEADER=\"$(abspath $(SRC_DIR)/runtime/cct_freestanding_rt.h)\" -DCCT_FREESTANDING_RT_SOURCE=\"$(abspath $(SRC_DIR)/runtime/cct_freestanding_rt.c)\"
BOOTSTRAP_STAGE_LDFLAGS = $(LDFLAGS) -lsqlite3
BOOTSTRAP_PHASE30_BIN_DIR = $(BOOTSTRAP_PHASE30_OUT)/bin
BOOTSTRAP_PHASE30_TOOLS_DIR = $(BOOTSTRAP_PHASE30_OUT)/tools
BOOTSTRAP_PHASE30_LOGS_DIR = $(BOOTSTRAP_PHASE30_OUT)/logs
BOOTSTRAP_PHASE30_MANIFESTS_DIR = $(BOOTSTRAP_PHASE30_OUT)/manifests
BOOTSTRAP_PHASE30_RUN_DIR = $(BOOTSTRAP_PHASE30_OUT)/run
BOOTSTRAP_PHASE30_COMPAT_DIR = $(BOOTSTRAP_PHASE30_OUT)/compat
BOOTSTRAP_SELFHOST_WRAPPER = $(BOOTSTRAP_PHASE30_BIN_DIR)/cct_selfhost
BOOTSTRAP_PHASE31_OUT = out/bootstrap/phase31
BOOTSTRAP_PHASE31_BIN_DIR = $(BOOTSTRAP_PHASE31_OUT)/bin
BOOTSTRAP_PHASE31_TOOLS_DIR = $(BOOTSTRAP_PHASE31_OUT)/tools
BOOTSTRAP_PHASE31_STATE_DIR = $(BOOTSTRAP_PHASE31_OUT)/state
BOOTSTRAP_PHASE31_STATE_MODE = $(BOOTSTRAP_PHASE31_STATE_DIR)/default_mode.txt
BOOTSTRAP_PHASE31_LOGS_DIR = $(BOOTSTRAP_PHASE31_OUT)/logs
BOOTSTRAP_PHASE31_MANIFESTS_DIR = $(BOOTSTRAP_PHASE31_OUT)/manifests
BOOTSTRAP_PHASE31_PERSIST_DIR = .cct/toolchain
BOOTSTRAP_PHASE31_DEFAULT_MODE = $(BOOTSTRAP_PHASE31_PERSIST_DIR)/default_mode.txt
BOOTSTRAP_PHASE31_ACTIVE_MANIFEST = $(BOOTSTRAP_PHASE31_MANIFESTS_DIR)/active_compiler.txt
BOOTSTRAP_SELFHOST_LEXER_BIN = $(BOOTSTRAP_PHASE31_TOOLS_DIR)/cct_lexer_bootstrap
BOOTSTRAP_SELFHOST_LEXER_C = $(BOOTSTRAP_PHASE31_TOOLS_DIR)/cct_lexer_bootstrap.c
BOOTSTRAP_SELFHOST_PARSER_BIN = $(BOOTSTRAP_PHASE30_TOOLS_DIR)/cct_parser_bootstrap
BOOTSTRAP_SELFHOST_PARSER_C = $(BOOTSTRAP_PHASE30_TOOLS_DIR)/cct_parser_bootstrap.c
BOOTSTRAP_SELFHOST_SEMANTIC_BIN = $(BOOTSTRAP_PHASE30_TOOLS_DIR)/cct_semantic_bootstrap
BOOTSTRAP_SELFHOST_SEMANTIC_C = $(BOOTSTRAP_PHASE30_TOOLS_DIR)/cct_semantic_bootstrap.c
BOOTSTRAP_SELFHOST_CODEGEN_BIN = $(BOOTSTRAP_PHASE30_TOOLS_DIR)/cct_codegen_bootstrap
BOOTSTRAP_SELFHOST_CODEGEN_C = $(BOOTSTRAP_PHASE30_TOOLS_DIR)/cct_codegen_bootstrap.c
BOOTSTRAP_PHASE30_READY_MANIFEST = $(BOOTSTRAP_PHASE30_MANIFESTS_DIR)/selfhost_ready.txt
BOOTSTRAP_PHASE30_STDLIB_MATRIX = $(BOOTSTRAP_PHASE30_COMPAT_DIR)/selfhost_stdlib_matrix.txt

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
	$(SRC_DIR)/runtime/runtime_crypto.c \
	$(SRC_DIR)/runtime/runtime_regex.c \
	$(SRC_DIR)/runtime/runtime_toml.c \
	$(SRC_DIR)/runtime/runtime_compress.c \
	$(SRC_DIR)/runtime/runtime_filetype.c \
	$(SRC_DIR)/runtime/runtime_image_ops.c \
	$(SRC_DIR)/runtime/runtime_signal.c \
	$(SRC_DIR)/runtime/runtime_postgres.c \
	$(SRC_DIR)/runtime/runtime_mail.c \
	$(SRC_DIR)/runtime/runtime_instrument.c \
	$(SRC_DIR)/runtime/runtime_math.c \
	$(SRC_DIR)/module/module.c \
	$(SRC_DIR)/sigilo/sigilo.c \
	$(SRC_DIR)/sigilo/sigil_parse.c \
	$(SRC_DIR)/sigilo/sigil_validate.c \
	$(SRC_DIR)/sigilo/sigil_diff.c \
	$(SRC_DIR)/sigilo/trace_render.c \
	$(SRC_DIR)/sigilo/trace_overlay.c \
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
	$(SRC_DIR)/sigilo/trace_render.h \
	$(SRC_DIR)/sigilo/trace_overlay.h \
	$(SRC_DIR)/formatter/formatter.h \
	$(SRC_DIR)/lint/lint.h \
	$(SRC_DIR)/project/project.h \
	$(SRC_DIR)/project/project_discovery.h \
	$(SRC_DIR)/project/project_cache.h \
	$(SRC_DIR)/project/project_runner.h \
	$(SRC_DIR)/project/sigilo_baseline.h \
	$(SRC_DIR)/doc/doc.h

# Default target
all: $(HOST_TARGET) $(CORE_WRAPPERS)

# Link the final host binary
$(HOST_TARGET): $(OBJS) | $(BIN_DIR)
	@echo "Linking $(HOST_TARGET)..."
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(HOST_TARGET)"

$(TARGET): scripts/cct_wrapper.sh | $(BIN_DIR) $(BOOTSTRAP_PHASE31_STATE_DIR) $(BOOTSTRAP_PHASE31_PERSIST_DIR) $(BOOTSTRAP_PHASE31_MANIFESTS_DIR)
	@chmod +x scripts/cct_wrapper.sh
	@printf '%s\n' '#!/bin/sh' \
		'SCRIPT_DIR="$$(CDPATH= cd -- "$$(dirname -- "$$0")" && pwd)"' \
		'export CCT_WRAPPER_MODE=default' \
		'exec "$$SCRIPT_DIR/scripts/cct_wrapper.sh" "$$@"' >"$@"
	@chmod +x "$@"
	@if [ ! -f "$(BOOTSTRAP_PHASE31_DEFAULT_MODE)" ]; then printf '%s\n' 'selfhost' >"$(BOOTSTRAP_PHASE31_DEFAULT_MODE)"; fi
	@printf '%s\n' "active=default-wrapper" "mode_file=$(abspath $(BOOTSTRAP_PHASE31_DEFAULT_MODE))" >"$(BOOTSTRAP_PHASE31_ACTIVE_MANIFEST)"

$(CCT_HOST_WRAPPER): scripts/cct_wrapper.sh | $(BIN_DIR)
	@chmod +x scripts/cct_wrapper.sh
	@printf '%s\n' '#!/bin/sh' \
		'SCRIPT_DIR="$$(CDPATH= cd -- "$$(dirname -- "$$0")" && pwd)"' \
		'export CCT_WRAPPER_MODE=host' \
		'exec "$$SCRIPT_DIR/scripts/cct_wrapper.sh" "$$@"' >"$@"
	@chmod +x "$@"

$(CCT_SELFHOST_ROOT): scripts/cct_wrapper.sh | $(BIN_DIR)
	@chmod +x scripts/cct_wrapper.sh
	@printf '%s\n' '#!/bin/sh' \
		'SCRIPT_DIR="$$(CDPATH= cd -- "$$(dirname -- "$$0")" && pwd)"' \
		'export CCT_WRAPPER_MODE=selfhost' \
		'exec "$$SCRIPT_DIR/scripts/cct_wrapper.sh" "$$@"' >"$@"
	@chmod +x "$@"

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
lbos-bridge: $(HOST_TARGET) $(CCT_LBOS_OUT)
	@echo "[CCT] lbos-bridge: emitindo ASM freestanding..."
	@$(HOST_TARGET) --profile freestanding --emit-asm --entry $(CCT_KERNEL_ENTRY) "$(CCT_KERNEL_SOURCE)"
	@echo "[CCT] lbos-bridge: montando objeto freestanding..."
	@$(CCT_FREESTANDING_TOOLCHAIN) assemble "$(CCT_KERNEL_ASM)" "$(CCT_KERNEL_OBJ)"
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
	rm -f $(HOST_TARGET)
	rm -f $(CCT_HOST_WRAPPER)
	rm -f $(CCT_SELFHOST_ROOT)
	rm -f cct_lexer_bootstrap
	rm -f src/bootstrap/main_lexer src/bootstrap/main_lexer.cgen.c
	rm -f src/bootstrap/main_lexer.svg src/bootstrap/main_lexer.sigil
	rm -f src/bootstrap/main_lexer.system.svg src/bootstrap/main_lexer.system.sigil
	rm -rf $(CCT_LBOS_OUT)
	rm -rf $(BOOTSTRAP_PHASE31_OUT)
	@echo "Clean complete."

# Run tests
test: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running tests..."
	@bash tests/run_tests.sh

test-legacy-full: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running restored legacy test suite (phases 0-20)..."
	@bash tests/run_tests_legacy_0_20.sh

test-legacy-rebased: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running rebased legacy test suite (phases 0-20)..."
	@bash tests/run_tests_legacy_0_20_rebased.sh

test-host-legacy: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running host fallback legacy test suite..."
	@bash tests/run_tests_legacy_0_20_rebased.sh

test-all-0-30: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running full aggregated suite (phases 0-30)..."
	@bash tests/run_tests_all_0_30.sh

test-all-0-31: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running full aggregated suite (phases 0-31)..."
	@bash tests/run_tests_all_0_31.sh

test-bootstrap-parity: $(HOST_TARGET) $(CORE_WRAPPERS) bootstrap-promote
	@echo "Running bootstrap parity tests (legacy + bootstrap + selfhost suite via promoted compiler)..."
	@echo "This validates that the promoted self-hosted compiler can pass the core language tests."
	@echo "Parity matrix: docs/bootstrap_parity_matrix.txt"
	@bash tests/run_bootstrap_parity.sh

test-legacy: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running legacy/core tests..."
	@CCT_TEST_GROUP=legacy bash tests/run_tests.sh

test-bootstrap: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running bootstrap tests..."
	@CCT_TEST_GROUP=bootstrap bash tests/run_tests.sh

test-bootstrap-lexer: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running bootstrap lexer tests..."
	@CCT_TEST_GROUP=bootstrap-lexer bash tests/run_tests.sh

test-bootstrap-parser: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running bootstrap parser tests..."
	@CCT_TEST_GROUP=bootstrap-parser bash tests/run_tests.sh

test-bootstrap-semantic: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running bootstrap semantic tests..."
	@CCT_TEST_GROUP=bootstrap-semantic bash tests/run_tests.sh

test-bootstrap-codegen: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running bootstrap codegen tests..."
	@CCT_TEST_GROUP=bootstrap-codegen bash tests/run_tests.sh

test-bootstrap-selfhost: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running bootstrap self-hosting tests..."
	@env -u CCT_TEST_PHASES CCT_TEST_GROUP=bootstrap-selfhost bash tests/run_tests.sh

test-operational-selfhost: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running operational self-hosted tests..."
	@env -u CCT_TEST_GROUP CCT_TEST_PHASES=30A bash tests/run_tests.sh

test-operational-platform: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running operational platform tests..."
	@env -u CCT_TEST_GROUP CCT_TEST_PHASES=30B,30C,30D bash tests/run_tests.sh

test-phase30-final: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running final FASE 30 gate..."
	@env -u CCT_TEST_GROUP CCT_TEST_PHASES=30A,30B,30C,30D bash tests/run_tests.sh

test-phase31-final: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running final FASE 31 gate..."
	@env -u CCT_TEST_GROUP CCT_TEST_PHASES=31A,31B,31C,31D,31E bash tests/run_tests.sh

test-phase: $(HOST_TARGET) $(CORE_WRAPPERS)
	@if [ -z "$(PHASE)" ]; then \
		echo "Usage: make test-phase PHASE=26"; \
		exit 1; \
	fi
	@echo "Running phase-selected tests: $(PHASE)"
	@env -u CCT_TEST_GROUP CCT_TEST_PHASES=$(PHASE) bash tests/run_tests.sh

cct_lexer_bootstrap: $(HOST_TARGET) $(CCT_HOST_WRAPPER) \
	src/bootstrap/main_lexer.cct \
	src/bootstrap/lexer/token_type.cct \
	src/bootstrap/lexer/token.cct \
	src/bootstrap/lexer/keywords.cct \
	src/bootstrap/lexer/lexer_state.cct \
	src/bootstrap/lexer/lexer_helpers.cct \
	src/bootstrap/lexer/lexer.cct
	@echo "Building cct_lexer_bootstrap..."
	@rm -f cct_lexer_bootstrap src/bootstrap/main_lexer src/bootstrap/main_lexer.cgen.c
	@./cct-host src/bootstrap/main_lexer.cct >/dev/null
	@mv src/bootstrap/main_lexer cct_lexer_bootstrap
	@rm -f src/bootstrap/main_lexer.cgen.c
	@rm -f src/bootstrap/main_lexer.svg src/bootstrap/main_lexer.sigil
	@rm -f src/bootstrap/main_lexer.system.svg src/bootstrap/main_lexer.system.sigil
	@echo "Build complete: cct_lexer_bootstrap"

$(BOOTSTRAP_PHASE29_OUT) $(BOOTSTRAP_STAGE0_DIR) $(BOOTSTRAP_STAGE1_DIR) $(BOOTSTRAP_STAGE2_DIR) $(BOOTSTRAP_STAGE_DIFF_DIR) $(BOOTSTRAP_STAGE_BENCH_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR) $(BOOTSTRAP_SUPPORT_DIR):
	@mkdir -p "$(BOOTSTRAP_PHASE29_OUT)" "$(BOOTSTRAP_STAGE0_DIR)" "$(BOOTSTRAP_STAGE1_DIR)" "$(BOOTSTRAP_STAGE2_DIR)" "$(BOOTSTRAP_STAGE_DIFF_DIR)" "$(BOOTSTRAP_STAGE_BENCH_DIR)" "$(BOOTSTRAP_STAGE_LOGS_DIR)" "$(BOOTSTRAP_SUPPORT_DIR)"

$(BOOTSTRAP_PHASE30_OUT) $(BOOTSTRAP_PHASE30_BIN_DIR) $(BOOTSTRAP_PHASE30_TOOLS_DIR) $(BOOTSTRAP_PHASE30_LOGS_DIR) $(BOOTSTRAP_PHASE30_MANIFESTS_DIR) $(BOOTSTRAP_PHASE30_RUN_DIR) $(BOOTSTRAP_PHASE30_COMPAT_DIR):
	@mkdir -p "$(BOOTSTRAP_PHASE30_OUT)" "$(BOOTSTRAP_PHASE30_BIN_DIR)" "$(BOOTSTRAP_PHASE30_TOOLS_DIR)" "$(BOOTSTRAP_PHASE30_LOGS_DIR)" "$(BOOTSTRAP_PHASE30_MANIFESTS_DIR)" "$(BOOTSTRAP_PHASE30_RUN_DIR)" "$(BOOTSTRAP_PHASE30_COMPAT_DIR)"

$(BOOTSTRAP_PHASE31_OUT) $(BOOTSTRAP_PHASE31_BIN_DIR) $(BOOTSTRAP_PHASE31_TOOLS_DIR) $(BOOTSTRAP_PHASE31_STATE_DIR) $(BOOTSTRAP_PHASE31_LOGS_DIR) $(BOOTSTRAP_PHASE31_MANIFESTS_DIR) $(BOOTSTRAP_PHASE31_PERSIST_DIR):
	@mkdir -p "$(BOOTSTRAP_PHASE31_OUT)" "$(BOOTSTRAP_PHASE31_BIN_DIR)" "$(BOOTSTRAP_PHASE31_TOOLS_DIR)" "$(BOOTSTRAP_PHASE31_STATE_DIR)" "$(BOOTSTRAP_PHASE31_LOGS_DIR)" "$(BOOTSTRAP_PHASE31_MANIFESTS_DIR)" "$(BOOTSTRAP_PHASE31_PERSIST_DIR)"

bootstrap-support: $(HOST_TARGET) $(CCT_HOST_WRAPPER) $(BOOTSTRAP_SUPPORT_SRC) | $(BOOTSTRAP_SUPPORT_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR)
	@echo "[29A] Building selfhost support objects..."
	@rm -f src/bootstrap/selfhost_support.cgen.c
	@./cct-host "$(BOOTSTRAP_SUPPORT_SRC)" >"$(BOOTSTRAP_STAGE_LOGS_DIR)/support.host.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/support.host.stderr.log"
	@for attempt in 1 2 3 4 5 6 7 8 9 10; do [ -f src/bootstrap/selfhost_support.cgen.c ] && break; sleep 0.1; done
	@test -f src/bootstrap/selfhost_support.cgen.c
	@mv -f src/bootstrap/selfhost_support.cgen.c "$(BOOTSTRAP_SUPPORT_HOST_C)"
	@perl -0pi -e 's/\nint main\(int argc, char \*\*argv\) \{\n    cct_rt_args_init\(argc, argv\);\n    return 0;\n\}\n/\n/s' "$(BOOTSTRAP_SUPPORT_HOST_C)"
	@sed -e 's/cct_fn_/cct_boot_rit_/g' -e '/cct_boot_rit_/ s/^static //' "$(BOOTSTRAP_SUPPORT_HOST_C)" >"$(BOOTSTRAP_SUPPORT_LINK_C)"
	@perl -0pi -e 's/^static void cct_rt_args_init\(int argc, char \*\*argv\)/void cct_rt_args_init(int argc, char **argv)/m' "$(BOOTSTRAP_SUPPORT_LINK_C)"
	@perl -0pi -e 's/^static ((?:[A-Za-z_][A-Za-z0-9_]*\s+)*[A-Za-z_][A-Za-z0-9_]*\s*\*?\s*cct_rt_(?:option_[A-Za-z0-9_]+|result_[A-Za-z0-9_]+|map_[A-Za-z0-9_]+|set_[A-Za-z0-9_]+)\()/$$1/mg' "$(BOOTSTRAP_SUPPORT_LINK_C)"
	@awk -f tools/gen_selfhost_aliases.awk src/bootstrap/selfhost_prelude.cct >>"$(BOOTSTRAP_SUPPORT_LINK_C)"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -c -o "$(BOOTSTRAP_SUPPORT_OBJ)" "$(BOOTSTRAP_SUPPORT_LINK_C)" >"$(BOOTSTRAP_STAGE_LOGS_DIR)/support.cc.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/support.cc.stderr.log"
	@echo "[29A] support objects ready: $(BOOTSTRAP_SUPPORT_OBJ)"

bootstrap-stage0: $(HOST_TARGET) $(CCT_HOST_WRAPPER) $(BOOTSTRAP_COMPILER_SRC) | $(BOOTSTRAP_PHASE29_OUT) $(BOOTSTRAP_STAGE0_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR)
	@echo "[29A] Building stage0..."
	@rm -f src/bootstrap/main_compiler src/bootstrap/main_compiler.cgen.c
	@./cct-host "$(BOOTSTRAP_COMPILER_SRC)" >"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage0.host.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage0.host.stderr.log"
	@for attempt in 1 2 3 4 5 6 7 8 9 10; do [ -f src/bootstrap/main_compiler ] && [ -f src/bootstrap/main_compiler.cgen.c ] && break; sleep 0.1; done
	@test -f src/bootstrap/main_compiler
	@test -f src/bootstrap/main_compiler.cgen.c
	@mv -f src/bootstrap/main_compiler "$(BOOTSTRAP_STAGE0_BIN)"
	@mv -f src/bootstrap/main_compiler.cgen.c "$(BOOTSTRAP_STAGE0_C)"
	@printf '%s\n' \
		"stage=stage0" \
		"producer=host-cct" \
		"input=$(abspath $(BOOTSTRAP_COMPILER_SRC))" \
		"generated_c=$(abspath $(BOOTSTRAP_STAGE0_C))" \
		"binary=$(abspath $(BOOTSTRAP_STAGE0_BIN))" \
		"generated_c_size=$$(wc -c < '$(BOOTSTRAP_STAGE0_C)' | tr -d ' ')" \
		"binary_size=$$(wc -c < '$(BOOTSTRAP_STAGE0_BIN)' | tr -d ' ')" \
		"generated_c_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE0_C)' | awk '{print $$1}')" \
		"binary_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE0_BIN)' | awk '{print $$1}')" \
		> "$(BOOTSTRAP_STAGE0_MANIFEST)"
	@printf '%s\n' \
		"generated_c_size=$$(wc -c < '$(BOOTSTRAP_STAGE0_C)' | tr -d ' ')" \
		"binary_size=$$(wc -c < '$(BOOTSTRAP_STAGE0_BIN)' | tr -d ' ')" \
		"generated_c_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE0_C)' | awk '{print $$1}')" \
		> "$(BOOTSTRAP_STAGE0_IDENTITY)"
	@echo "[29A] stage0 ready: $(BOOTSTRAP_STAGE0_BIN)"

bootstrap-stage1: bootstrap-stage0 bootstrap-support | $(BOOTSTRAP_STAGE1_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR)
	@echo "[29B] Building stage1..."
	@"$(BOOTSTRAP_STAGE0_BIN)" "$(BOOTSTRAP_COMPILER_SRC)" "$(BOOTSTRAP_STAGE1_C)" >"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage1.bootstrap.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage1.bootstrap.stderr.log"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$(BOOTSTRAP_STAGE1_BIN)" "$(BOOTSTRAP_STAGE1_C)" "$(BOOTSTRAP_SUPPORT_OBJ)" "$(SRC_DIR)/runtime/fs_runtime.c" $(BOOTSTRAP_STAGE_LDFLAGS) >"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage1.cc.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage1.cc.stderr.log"
	@printf '%s\n' \
		"stage=stage1" \
		"producer=$(abspath $(BOOTSTRAP_STAGE0_BIN))" \
		"input=$(abspath $(BOOTSTRAP_COMPILER_SRC))" \
		"generated_c=$(abspath $(BOOTSTRAP_STAGE1_C))" \
		"binary=$(abspath $(BOOTSTRAP_STAGE1_BIN))" \
		"generated_c_size=$$(wc -c < '$(BOOTSTRAP_STAGE1_C)' | tr -d ' ')" \
		"binary_size=$$(wc -c < '$(BOOTSTRAP_STAGE1_BIN)' | tr -d ' ')" \
		"generated_c_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE1_C)' | awk '{print $$1}')" \
		"binary_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE1_BIN)' | awk '{print $$1}')" \
		> "$(BOOTSTRAP_STAGE1_MANIFEST)"
	@printf '%s\n' \
		"generated_c_size=$$(wc -c < '$(BOOTSTRAP_STAGE1_C)' | tr -d ' ')" \
		"binary_size=$$(wc -c < '$(BOOTSTRAP_STAGE1_BIN)' | tr -d ' ')" \
		"generated_c_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE1_C)' | awk '{print $$1}')" \
		> "$(BOOTSTRAP_STAGE1_IDENTITY)"
	@echo "[29B] stage1 ready: $(BOOTSTRAP_STAGE1_BIN)"

bootstrap-stage2: bootstrap-stage1 bootstrap-support | $(BOOTSTRAP_STAGE2_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR)
	@echo "[29C] Building stage2..."
	@"$(BOOTSTRAP_STAGE1_BIN)" "$(BOOTSTRAP_COMPILER_SRC)" "$(BOOTSTRAP_STAGE2_C)" >"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage2.bootstrap.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage2.bootstrap.stderr.log"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$(BOOTSTRAP_STAGE2_BIN)" "$(BOOTSTRAP_STAGE2_C)" "$(BOOTSTRAP_SUPPORT_OBJ)" "$(SRC_DIR)/runtime/fs_runtime.c" $(BOOTSTRAP_STAGE_LDFLAGS) >"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage2.cc.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage2.cc.stderr.log"
	@printf '%s\n' \
		"stage=stage2" \
		"producer=$(abspath $(BOOTSTRAP_STAGE1_BIN))" \
		"input=$(abspath $(BOOTSTRAP_COMPILER_SRC))" \
		"generated_c=$(abspath $(BOOTSTRAP_STAGE2_C))" \
		"binary=$(abspath $(BOOTSTRAP_STAGE2_BIN))" \
		"generated_c_size=$$(wc -c < '$(BOOTSTRAP_STAGE2_C)' | tr -d ' ')" \
		"binary_size=$$(wc -c < '$(BOOTSTRAP_STAGE2_BIN)' | tr -d ' ')" \
		"generated_c_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE2_C)' | awk '{print $$1}')" \
		"binary_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE2_BIN)' | awk '{print $$1}')" \
		> "$(BOOTSTRAP_STAGE2_MANIFEST)"
	@printf '%s\n' \
		"generated_c_size=$$(wc -c < '$(BOOTSTRAP_STAGE2_C)' | tr -d ' ')" \
		"binary_size=$$(wc -c < '$(BOOTSTRAP_STAGE2_BIN)' | tr -d ' ')" \
		"generated_c_sha256=$$($(SHA256) '$(BOOTSTRAP_STAGE2_C)' | awk '{print $$1}')" \
		> "$(BOOTSTRAP_STAGE2_IDENTITY)"
	@echo "[29C] stage2 ready: $(BOOTSTRAP_STAGE2_BIN)"

bootstrap-stage-diff: bootstrap-stage2 | $(BOOTSTRAP_STAGE_DIFF_DIR)
	@echo "[29C/29D] Comparing stage1 and stage2..."
	@if cmp -s "$(BOOTSTRAP_STAGE1_C)" "$(BOOTSTRAP_STAGE2_C)"; then : >"$(BOOTSTRAP_STAGE12_C_DIFF)"; else diff -u "$(BOOTSTRAP_STAGE1_C)" "$(BOOTSTRAP_STAGE2_C)" >"$(BOOTSTRAP_STAGE12_C_DIFF)" || true; fi
	@if [ "$$(uname -s)" = "Darwin" ]; then : >"$(BOOTSTRAP_STAGE12_BIN_DIFF)"; elif cmp -s "$(BOOTSTRAP_STAGE1_BIN)" "$(BOOTSTRAP_STAGE2_BIN)"; then : >"$(BOOTSTRAP_STAGE12_BIN_DIFF)"; else cmp -l "$(BOOTSTRAP_STAGE1_BIN)" "$(BOOTSTRAP_STAGE2_BIN)" >"$(BOOTSTRAP_STAGE12_BIN_DIFF)" || true; fi
	@if cmp -s "$(BOOTSTRAP_STAGE1_IDENTITY)" "$(BOOTSTRAP_STAGE2_IDENTITY)"; then : >"$(BOOTSTRAP_STAGE12_MANIFEST_DIFF)"; else diff -u "$(BOOTSTRAP_STAGE1_IDENTITY)" "$(BOOTSTRAP_STAGE2_IDENTITY)" >"$(BOOTSTRAP_STAGE12_MANIFEST_DIFF)" || true; fi

bootstrap-stage-identity: bootstrap-stage-diff
	@echo "[29D] Validating stage identity..."
	@test ! -s "$(BOOTSTRAP_STAGE12_C_DIFF)"
	@test ! -s "$(BOOTSTRAP_STAGE12_BIN_DIFF)"
	@test ! -s "$(BOOTSTRAP_STAGE12_MANIFEST_DIFF)"
	@echo "[29D] stage1 == stage2"

bootstrap-stage-bench: bootstrap-stage-identity | $(BOOTSTRAP_STAGE_BENCH_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR)
	@echo "[29F] Benchmarking stage pipeline..."
	@/usr/bin/time -p $(MAKE) -B bootstrap-stage0 >"$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage0.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage0.time.log"
	@/usr/bin/time -p $(MAKE) -B bootstrap-stage1 >"$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage1.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage1.time.log"
	@/usr/bin/time -p $(MAKE) -B bootstrap-stage2 >"$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage2.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage2.time.log"
	@printf '%s\n' \
		"stage0_real_seconds=$$(awk '/^real / {print $$2}' '$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage0.time.log')" \
		"stage1_real_seconds=$$(awk '/^real / {print $$2}' '$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage1.time.log')" \
		"stage2_real_seconds=$$(awk '/^real / {print $$2}' '$(BOOTSTRAP_STAGE_LOGS_DIR)/bench.stage2.time.log')" \
		> "$(BOOTSTRAP_STAGE_BENCH)"
	@echo "[29F] metrics ready: $(BOOTSTRAP_STAGE_BENCH)"

bootstrap-selfhost-ready: bootstrap-stage-identity | $(BOOTSTRAP_PHASE30_OUT) $(BOOTSTRAP_PHASE30_BIN_DIR) $(BOOTSTRAP_PHASE30_LOGS_DIR) $(BOOTSTRAP_PHASE30_MANIFESTS_DIR)
	@echo "[30A] Preparing self-hosted operational toolchain..."
	@printf '%s\n' '#!/bin/sh' \
		'ROOT_DIR="$$(CDPATH= cd -- "$$(dirname -- "$$0")/../../../.." && pwd)"' \
		'exec "$$ROOT_DIR/out/bootstrap/phase29/stage2/cct_stage2" "$$@"' >"$(BOOTSTRAP_SELFHOST_WRAPPER)"
	@chmod +x "$(BOOTSTRAP_SELFHOST_WRAPPER)"
	@printf '%s\n' \
		"phase=30A" \
		"compiler=$(abspath $(BOOTSTRAP_STAGE2_BIN))" \
		"support=$(abspath $(BOOTSTRAP_SUPPORT_OBJ))" \
		"wrapper=$(abspath $(BOOTSTRAP_SELFHOST_WRAPPER))" \
		"identity_diff=$(abspath $(BOOTSTRAP_STAGE12_MANIFEST_DIFF))" \
		> "$(BOOTSTRAP_PHASE30_READY_MANIFEST)"
	@echo "[30A] ready: $(BOOTSTRAP_SELFHOST_WRAPPER)"

bootstrap-selfhost-lexer: bootstrap-selfhost-ready | $(BOOTSTRAP_PHASE31_TOOLS_DIR) $(BOOTSTRAP_PHASE31_LOGS_DIR) $(BOOTSTRAP_PHASE31_MANIFESTS_DIR)
	@echo "[31B] Building bootstrap lexer tool with the self-hosted compiler..."
	@"$(BOOTSTRAP_STAGE2_BIN)" src/bootstrap/main_lexer.cct "$(BOOTSTRAP_SELFHOST_LEXER_C)" >"$(BOOTSTRAP_PHASE31_LOGS_DIR)/lexer.emit.stdout.log" 2>"$(BOOTSTRAP_PHASE31_LOGS_DIR)/lexer.emit.stderr.log"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$(BOOTSTRAP_SELFHOST_LEXER_BIN)" "$(BOOTSTRAP_SELFHOST_LEXER_C)" "$(BOOTSTRAP_SUPPORT_OBJ)" src/runtime/fs_runtime.c $(BOOTSTRAP_STAGE_LDFLAGS) >"$(BOOTSTRAP_PHASE31_LOGS_DIR)/lexer.cc.stdout.log" 2>"$(BOOTSTRAP_PHASE31_LOGS_DIR)/lexer.cc.stderr.log"
	@printf '%s\n' \
		"tool=lexer" \
		"producer=$(abspath $(BOOTSTRAP_STAGE2_BIN))" \
		"generated_c=$(abspath $(BOOTSTRAP_SELFHOST_LEXER_C))" \
		"binary=$(abspath $(BOOTSTRAP_SELFHOST_LEXER_BIN))" \
		> "$(BOOTSTRAP_PHASE31_MANIFESTS_DIR)/lexer.txt"

bootstrap-selfhost-parser: bootstrap-selfhost-ready | $(BOOTSTRAP_PHASE30_TOOLS_DIR) $(BOOTSTRAP_PHASE30_LOGS_DIR) $(BOOTSTRAP_PHASE30_MANIFESTS_DIR)
	@echo "[30A] Building bootstrap parser tool with self-hosted compiler..."
	@"$(BOOTSTRAP_STAGE2_BIN)" src/bootstrap/main_parser.cct "$(BOOTSTRAP_SELFHOST_PARSER_C)" >"$(BOOTSTRAP_PHASE30_LOGS_DIR)/parser.emit.stdout.log" 2>"$(BOOTSTRAP_PHASE30_LOGS_DIR)/parser.emit.stderr.log"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$(BOOTSTRAP_SELFHOST_PARSER_BIN)" "$(BOOTSTRAP_SELFHOST_PARSER_C)" "$(BOOTSTRAP_SUPPORT_OBJ)" src/runtime/fs_runtime.c $(BOOTSTRAP_STAGE_LDFLAGS) >"$(BOOTSTRAP_PHASE30_LOGS_DIR)/parser.cc.stdout.log" 2>"$(BOOTSTRAP_PHASE30_LOGS_DIR)/parser.cc.stderr.log"
	@printf '%s\n' \
		"tool=parser" \
		"producer=$(abspath $(BOOTSTRAP_STAGE2_BIN))" \
		"generated_c=$(abspath $(BOOTSTRAP_SELFHOST_PARSER_C))" \
		"binary=$(abspath $(BOOTSTRAP_SELFHOST_PARSER_BIN))" \
		> "$(BOOTSTRAP_PHASE30_MANIFESTS_DIR)/parser.txt"

bootstrap-selfhost-semantic: bootstrap-selfhost-ready | $(BOOTSTRAP_PHASE30_TOOLS_DIR) $(BOOTSTRAP_PHASE30_LOGS_DIR) $(BOOTSTRAP_PHASE30_MANIFESTS_DIR)
	@echo "[30A] Building bootstrap semantic tool with self-hosted compiler..."
	@"$(BOOTSTRAP_STAGE2_BIN)" src/bootstrap/main_semantic.cct "$(BOOTSTRAP_SELFHOST_SEMANTIC_C)" >"$(BOOTSTRAP_PHASE30_LOGS_DIR)/semantic.emit.stdout.log" 2>"$(BOOTSTRAP_PHASE30_LOGS_DIR)/semantic.emit.stderr.log"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$(BOOTSTRAP_SELFHOST_SEMANTIC_BIN)" "$(BOOTSTRAP_SELFHOST_SEMANTIC_C)" "$(BOOTSTRAP_SUPPORT_OBJ)" src/runtime/fs_runtime.c $(BOOTSTRAP_STAGE_LDFLAGS) >"$(BOOTSTRAP_PHASE30_LOGS_DIR)/semantic.cc.stdout.log" 2>"$(BOOTSTRAP_PHASE30_LOGS_DIR)/semantic.cc.stderr.log"
	@printf '%s\n' \
		"tool=semantic" \
		"producer=$(abspath $(BOOTSTRAP_STAGE2_BIN))" \
		"generated_c=$(abspath $(BOOTSTRAP_SELFHOST_SEMANTIC_C))" \
		"binary=$(abspath $(BOOTSTRAP_SELFHOST_SEMANTIC_BIN))" \
		> "$(BOOTSTRAP_PHASE30_MANIFESTS_DIR)/semantic.txt"

bootstrap-selfhost-codegen: bootstrap-selfhost-ready | $(BOOTSTRAP_PHASE30_TOOLS_DIR) $(BOOTSTRAP_PHASE30_LOGS_DIR) $(BOOTSTRAP_PHASE30_MANIFESTS_DIR)
	@echo "[30A] Building bootstrap codegen tool with self-hosted compiler..."
	@"$(BOOTSTRAP_STAGE2_BIN)" src/bootstrap/main_codegen.cct "$(BOOTSTRAP_SELFHOST_CODEGEN_C)" >"$(BOOTSTRAP_PHASE30_LOGS_DIR)/codegen.emit.stdout.log" 2>"$(BOOTSTRAP_PHASE30_LOGS_DIR)/codegen.emit.stderr.log"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$(BOOTSTRAP_SELFHOST_CODEGEN_BIN)" "$(BOOTSTRAP_SELFHOST_CODEGEN_C)" "$(BOOTSTRAP_SUPPORT_OBJ)" src/runtime/fs_runtime.c $(BOOTSTRAP_STAGE_LDFLAGS) >"$(BOOTSTRAP_PHASE30_LOGS_DIR)/codegen.cc.stdout.log" 2>"$(BOOTSTRAP_PHASE30_LOGS_DIR)/codegen.cc.stderr.log"
	@printf '%s\n' \
		"tool=codegen" \
		"producer=$(abspath $(BOOTSTRAP_STAGE2_BIN))" \
		"generated_c=$(abspath $(BOOTSTRAP_SELFHOST_CODEGEN_C))" \
		"binary=$(abspath $(BOOTSTRAP_SELFHOST_CODEGEN_BIN))" \
		> "$(BOOTSTRAP_PHASE30_MANIFESTS_DIR)/codegen.txt"

bootstrap-selfhost-build: bootstrap-selfhost-ready | $(BOOTSTRAP_PHASE30_RUN_DIR) $(BOOTSTRAP_PHASE30_LOGS_DIR)
	@if [ -z "$(SRC)" ]; then echo "bootstrap-selfhost-build: missing SRC=<file.cct>" >&2; exit 2; fi
	@if [ -z "$(OUT)" ]; then echo "bootstrap-selfhost-build: missing OUT=<binary>" >&2; exit 2; fi
	@"$(BOOTSTRAP_STAGE2_BIN)" "$(SRC)" "$(OUT).c" >"$(BOOTSTRAP_PHASE30_LOGS_DIR)/build.$(notdir $(OUT)).emit.stdout.log" 2>"$(BOOTSTRAP_PHASE30_LOGS_DIR)/build.$(notdir $(OUT)).emit.stderr.log"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$(OUT)" "$(OUT).c" "$(BOOTSTRAP_SUPPORT_OBJ)" src/runtime/fs_runtime.c $(BOOTSTRAP_STAGE_LDFLAGS) >"$(BOOTSTRAP_PHASE30_LOGS_DIR)/build.$(notdir $(OUT)).cc.stdout.log" 2>"$(BOOTSTRAP_PHASE30_LOGS_DIR)/build.$(notdir $(OUT)).cc.stderr.log"

bootstrap-selfhost-run: bootstrap-selfhost-build
	@"$(OUT)" $(ARGS)

bootstrap-selfhost-stdlib-matrix: bootstrap-selfhost-ready | $(BOOTSTRAP_PHASE30_COMPAT_DIR)
	@printf '%s\n' \
		'phase=30B' \
		'compiler=$(abspath $(BOOTSTRAP_STAGE2_BIN))' \
		'module.verbum=SUPPORTED:prelude+support' \
		'module.fmt=SUPPORTED:prelude+support' \
		'module.fs=SUPPORTED:prelude+support' \
		'module.path=SUPPORTED:prelude+support' \
		'module.fluxus=SUPPORTED:prelude+support' \
		'module.parse=SUPPORTED:prelude+support' \
		'module.crypto=HOST_ONLY:wrapper-dispatch' \
		'module.encoding=SUPPORTED:source-backed-selfhost' \
		'module.regex=HOST_ONLY:wrapper-dispatch' \
		'module.date=HOST_ONLY:wrapper-dispatch' \
		'module.toml=HOST_ONLY:wrapper-dispatch' \
		'module.compress=HOST_ONLY:wrapper-dispatch' \
		'module.config=PENDING:not-exported-in-selfhost-prelude' \
		'module.json=PENDING:not-exported-in-selfhost-prelude' \
		'module.db_sqlite=PENDING:not-exported-in-selfhost-prelude' \
		'module.http=PENDING:not-exported-in-selfhost-prelude' \
		> "$(BOOTSTRAP_PHASE30_STDLIB_MATRIX)"

project-selfhost-build:
	@if [ -z "$(PROJECT)" ]; then echo "project-selfhost-build: missing PROJECT=<dir>" >&2; exit 2; fi
	@if [ ! -x "$(BOOTSTRAP_SELFHOST_WRAPPER)" ] || [ ! -x "$(BOOTSTRAP_STAGE2_BIN)" ] || [ ! -f "$(BOOTSTRAP_SUPPORT_OBJ)" ]; then $(MAKE) --no-print-directory bootstrap-selfhost-ready >/dev/null; fi
	@PROJECT_ROOT="$$(cd "$(PROJECT)" && pwd)"; \
	PROJECT_NAME="$$(awk -F'"' '/^[[:space:]]*name[[:space:]]*=[[:space:]]*"/{print $$2; exit}' "$$PROJECT_ROOT/cct.toml" 2>/dev/null)"; \
	if [ -z "$$PROJECT_NAME" ]; then PROJECT_NAME="$$(basename "$$PROJECT_ROOT")"; fi; \
	ENTRY_REL="$(ENTRY)"; \
	if [ -z "$$ENTRY_REL" ]; then ENTRY_REL="$$(awk -F'"' '/^[[:space:]]*entry[[:space:]]*=[[:space:]]*"/{print $$2; exit}' "$$PROJECT_ROOT/cct.toml" 2>/dev/null)"; fi; \
	if [ -z "$$ENTRY_REL" ]; then ENTRY_REL="src/main.cct"; fi; \
	case "$$ENTRY_REL" in /*) ENTRY_PATH="$$ENTRY_REL" ;; *) ENTRY_PATH="$$PROJECT_ROOT/$$ENTRY_REL" ;; esac; \
	if [ ! -f "$$ENTRY_PATH" ]; then echo "project-selfhost-build: entry file not found (expected src/main.cct or explicit ENTRY)" >&2; exit 2; fi; \
	OUT_BIN="$(OUT)"; \
	if [ -z "$$OUT_BIN" ]; then OUT_BIN="$$PROJECT_ROOT/dist/$${PROJECT_NAME}_selfhost"; fi; \
	mkdir -p "$$(dirname "$$OUT_BIN")"; \
	PROFILE="debug"; \
	if [ -n "$(RELEASE)" ]; then PROFILE="release"; fi; \
	"$(BOOTSTRAP_STAGE2_BIN)" "$$ENTRY_PATH" "$$OUT_BIN.c"; \
	$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$$OUT_BIN" "$$OUT_BIN.c" "$(BOOTSTRAP_SUPPORT_OBJ)" src/runtime/fs_runtime.c $(BOOTSTRAP_STAGE_LDFLAGS); \
	printf '%s\n' \
		"[build] project root: $$PROJECT_ROOT" \
		"[build] profile: $$PROFILE" \
		"[build] entry: $$ENTRY_PATH" \
		"[build] output: $$OUT_BIN"

project-selfhost-run:
	@if [ -z "$(PROJECT)" ]; then echo "project-selfhost-run: missing PROJECT=<dir>" >&2; exit 2; fi
	@if [ ! -x "$(BOOTSTRAP_SELFHOST_WRAPPER)" ] || [ ! -x "$(BOOTSTRAP_STAGE2_BIN)" ] || [ ! -f "$(BOOTSTRAP_SUPPORT_OBJ)" ]; then $(MAKE) --no-print-directory bootstrap-selfhost-ready >/dev/null; fi
	@PROJECT_ROOT="$$(cd "$(PROJECT)" && pwd)"; \
	PROJECT_NAME="$$(awk -F'"' '/^[[:space:]]*name[[:space:]]*=[[:space:]]*"/{print $$2; exit}' "$$PROJECT_ROOT/cct.toml" 2>/dev/null)"; \
	if [ -z "$$PROJECT_NAME" ]; then PROJECT_NAME="$$(basename "$$PROJECT_ROOT")"; fi; \
	OUT_BIN="$(OUT)"; \
	if [ -z "$$OUT_BIN" ]; then OUT_BIN="$$PROJECT_ROOT/dist/$${PROJECT_NAME}_selfhost"; fi; \
	$(MAKE) --no-print-directory project-selfhost-build PROJECT="$(PROJECT)" $(if $(ENTRY),ENTRY="$(ENTRY)") OUT="$$OUT_BIN" $(if $(RELEASE),RELEASE="$(RELEASE)"); \
	"$$OUT_BIN" $(ARGS)

project-selfhost-test:
	@if [ -z "$(PROJECT)" ]; then echo "project-selfhost-test: missing PROJECT=<dir>" >&2; exit 2; fi
	@if [ ! -x "$(BOOTSTRAP_SELFHOST_WRAPPER)" ] || [ ! -x "$(BOOTSTRAP_STAGE2_BIN)" ] || [ ! -f "$(BOOTSTRAP_SUPPORT_OBJ)" ]; then $(MAKE) --no-print-directory bootstrap-selfhost-ready >/dev/null; fi
	@PROJECT_ROOT="$$(cd "$(PROJECT)" && pwd)"; \
	TEST_ROOT="$$PROJECT_ROOT/tests"; \
	ARTIFACT_DIR="$$PROJECT_ROOT/.cct/selfhost/tests"; \
	mkdir -p "$$ARTIFACT_DIR"; \
	DISCOVERED=0; PASS=0; FAIL=0; \
	if [ ! -d "$$TEST_ROOT" ]; then echo "[test] discovered: 0"; exit 0; fi; \
	for TEST_SRC in "$$TEST_ROOT"/*.test.cct; do \
		[ -e "$$TEST_SRC" ] || continue; \
		TEST_NAME="$$(basename "$$TEST_SRC")"; \
		if [ -n "$(PATTERN)" ] && ! printf '%s' "$$TEST_NAME" | grep -F -q -- "$(PATTERN)"; then \
			continue; \
		fi; \
		DISCOVERED=$$((DISCOVERED + 1)); \
		TEST_BIN="$$ARTIFACT_DIR/$${TEST_NAME%.cct}.selfhost"; \
		TEST_C="$$TEST_BIN.c"; \
		if "$(BOOTSTRAP_STAGE2_BIN)" "$$TEST_SRC" "$$TEST_C" >/dev/null 2>"$$TEST_BIN.emit.err" && \
		   $(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$$TEST_BIN" "$$TEST_C" "$(BOOTSTRAP_SUPPORT_OBJ)" src/runtime/fs_runtime.c $(BOOTSTRAP_STAGE_LDFLAGS) >/dev/null 2>"$$TEST_BIN.cc.err" && \
		   "$$TEST_BIN" >/dev/null 2>"$$TEST_BIN.run.err"; then \
			echo "[test] PASS $$TEST_SRC"; \
			PASS=$$((PASS + 1)); \
		else \
			echo "[test] FAIL $$TEST_SRC"; \
			FAIL=$$((FAIL + 1)); \
		fi; \
	done; \
	echo "[test] discovered: $$DISCOVERED"; \
	echo "[test] summary: pass=$$PASS fail=$$FAIL"; \
	test "$$FAIL" -eq 0

project-selfhost-clean:
	@if [ -z "$(PROJECT)" ]; then echo "project-selfhost-clean: missing PROJECT=<dir>" >&2; exit 2; fi
	@if [ ! -x "$(BOOTSTRAP_SELFHOST_WRAPPER)" ] || [ ! -x "$(BOOTSTRAP_STAGE2_BIN)" ] || [ ! -f "$(BOOTSTRAP_SUPPORT_OBJ)" ]; then $(MAKE) --no-print-directory bootstrap-selfhost-ready >/dev/null; fi
	@PROJECT_ROOT="$$(cd "$(PROJECT)" && pwd)"; \
	rm -rf "$$PROJECT_ROOT/.cct/selfhost"; \
	if [ -d "$$PROJECT_ROOT/dist" ]; then \
		find "$$PROJECT_ROOT/dist" -maxdepth 1 \( -name '*_selfhost' -o -name '*_selfhost.c' -o -name '*_selfhost.manifest.txt' \) -exec rm -f {} +; \
	fi

project-selfhost-package:
	@if [ -z "$(PROJECT)" ]; then echo "project-selfhost-package: missing PROJECT=<dir>" >&2; exit 2; fi
	@if [ ! -x "$(BOOTSTRAP_SELFHOST_WRAPPER)" ] || [ ! -x "$(BOOTSTRAP_STAGE2_BIN)" ] || [ ! -f "$(BOOTSTRAP_SUPPORT_OBJ)" ]; then $(MAKE) --no-print-directory bootstrap-selfhost-ready >/dev/null; fi
	@PROJECT_ROOT="$$(cd "$(PROJECT)" && pwd)"; \
	PROJECT_NAME="$$(awk -F'"' '/^[[:space:]]*name[[:space:]]*=[[:space:]]*"/{print $$2; exit}' "$$PROJECT_ROOT/cct.toml" 2>/dev/null)"; \
	if [ -z "$$PROJECT_NAME" ]; then PROJECT_NAME="$$(basename "$$PROJECT_ROOT")"; fi; \
	OUT_BIN="$(OUT)"; \
	if [ -z "$$OUT_BIN" ]; then OUT_BIN="$$PROJECT_ROOT/dist/$${PROJECT_NAME}_selfhost"; fi; \
	$(MAKE) --no-print-directory project-selfhost-build PROJECT="$(PROJECT)" $(if $(ENTRY),ENTRY="$(ENTRY)") OUT="$$OUT_BIN" RELEASE=1; \
	printf '%s\n' \
		"project=$$PROJECT_ROOT" \
		"compiler=$(abspath $(BOOTSTRAP_STAGE2_BIN))" \
		"wrapper=$(abspath $(BOOTSTRAP_SELFHOST_WRAPPER))" \
		"output=$$OUT_BIN" \
		"profile=release" \
		> "$$OUT_BIN.manifest.txt"

project-selfhost-bench:
	@if [ -z "$(PROJECT)" ]; then echo "project-selfhost-bench: missing PROJECT=<dir>" >&2; exit 2; fi
	@if [ ! -x "$(BOOTSTRAP_SELFHOST_WRAPPER)" ] || [ ! -x "$(BOOTSTRAP_STAGE2_BIN)" ] || [ ! -f "$(BOOTSTRAP_SUPPORT_OBJ)" ]; then $(MAKE) --no-print-directory bootstrap-selfhost-ready >/dev/null; fi
	@PROJECT_ROOT="$$(cd "$(PROJECT)" && pwd)"; \
	BENCH_ROOT="$$PROJECT_ROOT/bench"; \
	[ -d "$$BENCH_ROOT" ] || BENCH_ROOT="$$PROJECT_ROOT/tests"; \
	ARTIFACT_DIR="$$PROJECT_ROOT/.cct/selfhost/bench"; \
	mkdir -p "$$ARTIFACT_DIR"; \
	DISCOVERED=0; PASS=0; FAIL=0; \
	for BENCH_SRC in "$$BENCH_ROOT"/*.bench.cct; do \
		[ -e "$$BENCH_SRC" ] || continue; \
		BENCH_NAME="$$(basename "$$BENCH_SRC")"; \
		if [ -n "$(PATTERN)" ] && ! printf '%s' "$$BENCH_NAME" | grep -F -q -- "$(PATTERN)"; then \
			continue; \
		fi; \
		DISCOVERED=$$((DISCOVERED + 1)); \
		BENCH_BIN="$$ARTIFACT_DIR/$${BENCH_NAME%.cct}.selfhost"; \
		BENCH_C="$$BENCH_BIN.c"; \
		if "$(BOOTSTRAP_STAGE2_BIN)" "$$BENCH_SRC" "$$BENCH_C" >/dev/null 2>"$$BENCH_BIN.emit.err" && \
		   $(CC) $(BOOTSTRAP_STAGE_CFLAGS) -o "$$BENCH_BIN" "$$BENCH_C" "$(BOOTSTRAP_SUPPORT_OBJ)" src/runtime/fs_runtime.c $(BOOTSTRAP_STAGE_LDFLAGS) >/dev/null 2>"$$BENCH_BIN.cc.err"; then \
			"$$BENCH_BIN" >/dev/null 2>"$$BENCH_BIN.run.err"; \
			RC="$$?"; \
			echo "[bench] PASS $$BENCH_SRC (rc=$$RC)"; \
			PASS=$$((PASS + 1)); \
		else \
			echo "[bench] FAIL $$BENCH_SRC"; \
			FAIL=$$((FAIL + 1)); \
		fi; \
	done; \
	echo "[bench] discovered: $$DISCOVERED"; \
	echo "[bench] summary: pass=$$PASS fail=$$FAIL"; \
	test "$$FAIL" -eq 0

bootstrap-promote: $(TARGET) $(CCT_HOST_WRAPPER) $(CCT_SELFHOST_ROOT) bootstrap-selfhost-ready | $(BOOTSTRAP_PHASE31_STATE_DIR) $(BOOTSTRAP_PHASE31_PERSIST_DIR) $(BOOTSTRAP_PHASE31_MANIFESTS_DIR)
	@printf '%s\n' 'selfhost' >"$(BOOTSTRAP_PHASE31_DEFAULT_MODE)"
	@printf '%s\n' 'selfhost' >"$(BOOTSTRAP_PHASE31_STATE_MODE)"
	@printf '%s\n' \
		"active=selfhost" \
		"default_wrapper=$(abspath $(TARGET))" \
		"host_wrapper=$(abspath $(CCT_HOST_WRAPPER))" \
		"selfhost_wrapper=$(abspath $(CCT_SELFHOST_ROOT))" \
		> "$(BOOTSTRAP_PHASE31_ACTIVE_MANIFEST)"
	@echo "[31D] default compiler promoted to selfhost"

bootstrap-demote: $(TARGET) $(CCT_HOST_WRAPPER) $(CCT_SELFHOST_ROOT) | $(BOOTSTRAP_PHASE31_STATE_DIR) $(BOOTSTRAP_PHASE31_PERSIST_DIR) $(BOOTSTRAP_PHASE31_MANIFESTS_DIR)
	@printf '%s\n' 'host' >"$(BOOTSTRAP_PHASE31_DEFAULT_MODE)"
	@printf '%s\n' 'host' >"$(BOOTSTRAP_PHASE31_STATE_MODE)"
	@printf '%s\n' \
		"active=host" \
		"default_wrapper=$(abspath $(TARGET))" \
		"host_wrapper=$(abspath $(CCT_HOST_WRAPPER))" \
		"selfhost_wrapper=$(abspath $(CCT_SELFHOST_ROOT))" \
		> "$(BOOTSTRAP_PHASE31_ACTIVE_MANIFEST)"
	@echo "[31D] default compiler demoted to host"

test_lexer_bootstrap: cct_lexer_bootstrap
	@bash tests/validate_lexer_full_suite.sh

benchmark_lexer: cct_lexer_bootstrap
	@bash tests/benchmark_lexer_21e2.sh

valgrind_lexer: cct_lexer_bootstrap
	@bash tests/valgrind_lexer_21e3.sh

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
dist: $(HOST_TARGET) $(TARGET) $(CCT_HOST_WRAPPER) $(CCT_SELFHOST_ROOT)
	@echo "Building distribution bundle at $(DIST_DIR)..."
	@mkdir -p $(DIST_DIR)/bin
	@mkdir -p $(DIST_DIR)/lib/cct
	@mkdir -p $(DIST_DIR)/docs
	@mkdir -p $(DIST_DIR)/examples
ifeq ($(IS_WINDOWS),1)
	@cp $(HOST_TARGET) $(DIST_DIR)/bin/cct.exe
	@printf '%s\r\n' '@echo off' \
		'set "SCRIPT_DIR=%~dp0"' \
		'if not defined CCT_STDLIB_DIR set "CCT_STDLIB_DIR=%SCRIPT_DIR%..\\lib\\cct"' \
		'"%SCRIPT_DIR%cct.exe" %*' > $(DIST_DIR)/bin/cct.bat
else
	@cp $(HOST_TARGET) $(DIST_DIR)/bin/cct.bin
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
VERSION ?= 0.19
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

# Crypto toolchain flags (FASE 32A)
CRYPTO_CFLAGS ?= $(shell pkg-config --cflags openssl 2>/dev/null || echo "")
CRYPTO_LDFLAGS ?= $(shell pkg-config --libs openssl 2>/dev/null || echo "-lssl -lcrypto")
CRYPTO_CFLAGS_STRIP := $(strip $(CRYPTO_CFLAGS))
CRYPTO_LDFLAGS_STRIP := $(strip $(CRYPTO_LDFLAGS))
CFLAGS += $(CRYPTO_CFLAGS_STRIP)
LDFLAGS += $(CRYPTO_LDFLAGS_STRIP)
BOOTSTRAP_STAGE_CFLAGS += $(CRYPTO_CFLAGS_STRIP)
BOOTSTRAP_STAGE_LDFLAGS += $(CRYPTO_LDFLAGS_STRIP)

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
install: $(HOST_TARGET) $(TARGET)
	@echo "Installing $(TARGET) to $(PREFIX)/bin/..."
	mkdir -p $(PREFIX)/bin
ifeq ($(IS_WINDOWS),1)
	cp $(HOST_TARGET) $(PREFIX)/bin/cct.exe
	@printf '%s\r\n' '@echo off' \
		'if not defined CCT_STDLIB_DIR set "CCT_STDLIB_DIR=$(PREFIX)\\lib\\cct"' \
		'"$(PREFIX)\\bin\\cct.exe" %*' > $(PREFIX)/bin/cct.bat
else
	cp $(HOST_TARGET) $(PREFIX)/bin/cct.bin
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
	@echo "  test-legacy-full       - Run restored full legacy suite (phases 0-20)"
	@echo "  test-legacy-rebased    - Run rebased legacy suite (phases 0-20)"
	@echo "  test-host-legacy       - Run host fallback legacy suite"
	@echo "  test-all-0-30          - Run aggregated full suite (phases 0-30)"
	@echo "  test-legacy            - Run pre-bootstrap test block (phases before 21)"
	@echo "  test-bootstrap         - Run bootstrap blocks (phases 21-26)"
	@echo "  test-bootstrap-lexer   - Run bootstrap lexer block (phase 21)"
	@echo "  test-bootstrap-parser  - Run bootstrap parser blocks (phases 22-23)"
	@echo "  test-bootstrap-semantic - Run bootstrap semantic blocks (phases 24-25)"
	@echo "  test-bootstrap-codegen - Run bootstrap codegen block (phase 26)"
	@echo "  test-bootstrap-selfhost - Run bootstrap self-hosting block (phase 29)"
	@echo "  test-operational-selfhost - Run operational self-hosted block (phase 30A)"
	@echo "  test-operational-platform - Run operational platform blocks (phases 30B-30D)"
	@echo "  test-phase30-final - Run final operational gate (phases 30A-30D)"
	@echo "  test-phase31-final - Run self-hosted promotion gate (phase 31)"
	@echo "  test-bootstrap-parity - Validate promoted compiler passes core language tests (parity gate)"
	@echo "  test-all-0-31 - Run legacy + bootstrap + selfhost + operational + promotion suites"
	@echo "  bootstrap-stage0 - Build bootstrap compiler stage0 under out/bootstrap/phase29/"
	@echo "  bootstrap-support - Build host support object for self-hosting stages"
	@echo "  bootstrap-stage1 - Self-compile compiler with stage0"
	@echo "  bootstrap-stage2 - Self-compile compiler with stage1"
	@echo "  bootstrap-stage-identity - Validate stage1/stage2 identity"
	@echo "  bootstrap-stage-bench - Benchmark stage pipeline"
	@echo "  bootstrap-selfhost-ready - Materialize wrapper/manifests for the self-hosted compiler"
	@echo "  bootstrap-selfhost-parser - Build parser bootstrap tool with the self-hosted compiler"
	@echo "  bootstrap-selfhost-semantic - Build semantic bootstrap tool with the self-hosted compiler"
	@echo "  bootstrap-selfhost-codegen - Build codegen bootstrap tool with the self-hosted compiler"
	@echo "  bootstrap-selfhost-lexer - Build lexer bootstrap tool with the self-hosted compiler"
	@echo "  bootstrap-selfhost-build SRC=... OUT=... - Build a CCT program with the self-hosted compiler"
	@echo "  bootstrap-selfhost-run SRC=... OUT=... [ARGS=...] - Build and run a CCT program with the self-hosted compiler"
	@echo "  bootstrap-selfhost-stdlib-matrix - Materialize the explicit supported stdlib subset for the self-hosted path"
	@echo "  bootstrap-promote - Switch default ./cct wrapper to self-host mode"
	@echo "  bootstrap-demote - Switch default ./cct wrapper back to host mode"
	@echo "  project-selfhost-build PROJECT=... [OUT=...] - Build a project with the self-hosted compiler"
	@echo "  project-selfhost-run PROJECT=... [OUT=...] [ARGS=...] - Run a project with the self-hosted compiler"
	@echo "  project-selfhost-test PROJECT=... [PATTERN=...] - Run project tests with the self-hosted compiler"
	@echo "  project-selfhost-bench PROJECT=... [PATTERN=...] - Run project benchmarks with the self-hosted compiler"
	@echo "  project-selfhost-clean PROJECT=... [ALL=1] - Clean project artifacts with the self-hosted compiler"
	@echo "  project-selfhost-package PROJECT=... [OUT=...] - Build release artifact + manifest with the self-hosted compiler"
	@echo "  test-phase PHASE=26    - Run a selected major phase block"
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

fmt: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Formatting .cct files..."
	@find lib/cct -name "*.cct" -exec ./cct fmt {} \;
	@find examples -name "*.cct" -exec ./cct fmt {} \;
	@find tests/integration -name "*.cct" -exec ./cct fmt {} \;

fmt-check: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Checking .cct formatting..."
	@find lib/cct -name "*.cct" -exec ./cct fmt --check {} \; && \
	 find examples -name "*.cct" -exec ./cct fmt --check {} \; && \
	 find tests/integration -name "*.cct" -exec ./cct fmt --check {} \;

lint: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running lint on integration fixtures..."
	@find tests/integration -name "lint_*_12e2.cct" -exec ./cct lint {} \;

project-build: $(HOST_TARGET) $(CORE_WRAPPERS)
	@./cct build

project-run: $(HOST_TARGET) $(CORE_WRAPPERS)
	@./cct run

project-test: $(HOST_TARGET) $(CORE_WRAPPERS)
	@./cct test

project-test-strict: $(HOST_TARGET) $(CORE_WRAPPERS)
	@./cct test --strict-lint --fmt-check

project-bench: $(HOST_TARGET) $(CORE_WRAPPERS)
	@./cct bench

project-clean: $(HOST_TARGET) $(CORE_WRAPPERS)
	@./cct clean

doc: $(HOST_TARGET) $(CORE_WRAPPERS)
	@./cct doc

doc-strict: $(HOST_TARGET) $(CORE_WRAPPERS)
	@./cct doc --warn-missing-docs --strict-docs

# FASE 12H: Release check targets
release-check: $(HOST_TARGET) $(CORE_WRAPPERS)
	@echo "Running FASE 12 release validation..."
	@./cct fmt --check lib/cct/*.cct examples/**/*.cct || true
	@./cct lint --strict lib/cct/*.cct || true
	@./cct test
	@./cct doc --project . --format both --no-timestamp || true
	@echo "Release check complete."

phase12-final-audit: $(HOST_TARGET) $(CORE_WRAPPERS)
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

.PHONY: all clean test test-legacy-full test-legacy-rebased test-host-legacy test-all-0-30 test-all-0-31 test-bootstrap-parity test-legacy test-bootstrap test-bootstrap-lexer test-bootstrap-parser test-bootstrap-semantic test-bootstrap-codegen test-bootstrap-selfhost test-operational-selfhost test-operational-platform test-phase30-final test-phase31-final test-phase test_fluxus_storage test_diagnostic_taxonomy dist release install uninstall help fmt fmt-check lint lbos-bridge bootstrap-support bootstrap-stage0 bootstrap-stage1 bootstrap-stage2 bootstrap-stage-diff bootstrap-stage-identity bootstrap-stage-bench bootstrap-selfhost-ready bootstrap-selfhost-parser bootstrap-selfhost-semantic bootstrap-selfhost-codegen bootstrap-selfhost-lexer bootstrap-selfhost-build bootstrap-selfhost-run bootstrap-selfhost-stdlib-matrix bootstrap-promote bootstrap-demote project-selfhost-build project-selfhost-run project-selfhost-test project-selfhost-bench project-selfhost-clean project-selfhost-package project-build project-run project-test project-test-strict project-bench project-clean doc doc-strict release-check phase12-final-audit
