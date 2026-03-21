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
CCT_FREESTANDING_TOOLCHAIN = tools/freestanding_toolchain.sh
BOOTSTRAP_PHASE29_OUT = out/bootstrap/phase29
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
BOOTSTRAP_STAGE_LDFLAGS = $(LDFLAGS)

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
	rm -f cct_lexer_bootstrap
	rm -f src/bootstrap/main_lexer src/bootstrap/main_lexer.cgen.c
	rm -f src/bootstrap/main_lexer.svg src/bootstrap/main_lexer.sigil
	rm -f src/bootstrap/main_lexer.system.svg src/bootstrap/main_lexer.system.sigil
	rm -rf $(CCT_LBOS_OUT)
	@echo "Clean complete."

# Run tests
test: $(TARGET)
	@echo "Running tests..."
	@bash tests/run_tests.sh

test-legacy: $(TARGET)
	@echo "Running legacy/core tests..."
	@CCT_TEST_GROUP=legacy bash tests/run_tests.sh

test-bootstrap: $(TARGET)
	@echo "Running bootstrap tests..."
	@CCT_TEST_GROUP=bootstrap bash tests/run_tests.sh

test-bootstrap-lexer: $(TARGET)
	@echo "Running bootstrap lexer tests..."
	@CCT_TEST_GROUP=bootstrap-lexer bash tests/run_tests.sh

test-bootstrap-parser: $(TARGET)
	@echo "Running bootstrap parser tests..."
	@CCT_TEST_GROUP=bootstrap-parser bash tests/run_tests.sh

test-bootstrap-semantic: $(TARGET)
	@echo "Running bootstrap semantic tests..."
	@CCT_TEST_GROUP=bootstrap-semantic bash tests/run_tests.sh

test-bootstrap-codegen: $(TARGET)
	@echo "Running bootstrap codegen tests..."
	@CCT_TEST_GROUP=bootstrap-codegen bash tests/run_tests.sh

test-bootstrap-selfhost: $(TARGET)
	@echo "Running bootstrap self-hosting tests..."
	@CCT_TEST_GROUP=bootstrap-selfhost bash tests/run_tests.sh

test-phase: $(TARGET)
	@if [ -z "$(PHASE)" ]; then \
		echo "Usage: make test-phase PHASE=26"; \
		exit 1; \
	fi
	@echo "Running phase-selected tests: $(PHASE)"
	@CCT_TEST_PHASES=$(PHASE) bash tests/run_tests.sh

cct_lexer_bootstrap: $(TARGET) \
	src/bootstrap/main_lexer.cct \
	src/bootstrap/lexer/token_type.cct \
	src/bootstrap/lexer/token.cct \
	src/bootstrap/lexer/keywords.cct \
	src/bootstrap/lexer/lexer_state.cct \
	src/bootstrap/lexer/lexer_helpers.cct \
	src/bootstrap/lexer/lexer.cct
	@echo "Building cct_lexer_bootstrap..."
	@rm -f cct_lexer_bootstrap src/bootstrap/main_lexer src/bootstrap/main_lexer.cgen.c
	@./cct src/bootstrap/main_lexer.cct >/dev/null
	@mv src/bootstrap/main_lexer cct_lexer_bootstrap
	@rm -f src/bootstrap/main_lexer.cgen.c
	@rm -f src/bootstrap/main_lexer.svg src/bootstrap/main_lexer.sigil
	@rm -f src/bootstrap/main_lexer.system.svg src/bootstrap/main_lexer.system.sigil
	@echo "Build complete: cct_lexer_bootstrap"

$(BOOTSTRAP_PHASE29_OUT) $(BOOTSTRAP_STAGE0_DIR) $(BOOTSTRAP_STAGE1_DIR) $(BOOTSTRAP_STAGE2_DIR) $(BOOTSTRAP_STAGE_DIFF_DIR) $(BOOTSTRAP_STAGE_BENCH_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR) $(BOOTSTRAP_SUPPORT_DIR):
	@mkdir -p "$@"

bootstrap-support: $(TARGET) $(BOOTSTRAP_SUPPORT_SRC) | $(BOOTSTRAP_SUPPORT_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR)
	@echo "[29A] Building selfhost support objects..."
	@./cct "$(BOOTSTRAP_SUPPORT_SRC)" >"$(BOOTSTRAP_STAGE_LOGS_DIR)/support.host.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/support.host.stderr.log"
	@mv -f src/bootstrap/selfhost_support.cgen.c "$(BOOTSTRAP_SUPPORT_HOST_C)"
	@perl -0pi -e 's/\nint main\(int argc, char \*\*argv\) \{\n    cct_rt_args_init\(argc, argv\);\n    return 0;\n\}\n/\n/s' "$(BOOTSTRAP_SUPPORT_HOST_C)"
	@sed -e 's/cct_fn_/cct_boot_rit_/g' -e '/cct_boot_rit_/ s/^static //' "$(BOOTSTRAP_SUPPORT_HOST_C)" >"$(BOOTSTRAP_SUPPORT_LINK_C)"
	@perl -0pi -e 's/^static void cct_rt_args_init\(int argc, char \*\*argv\)/void cct_rt_args_init(int argc, char **argv)/m' "$(BOOTSTRAP_SUPPORT_LINK_C)"
	@awk -f tools/gen_selfhost_aliases.awk src/bootstrap/selfhost_prelude.cct >>"$(BOOTSTRAP_SUPPORT_LINK_C)"
	@$(CC) $(BOOTSTRAP_STAGE_CFLAGS) -c -o "$(BOOTSTRAP_SUPPORT_OBJ)" "$(BOOTSTRAP_SUPPORT_LINK_C)" >"$(BOOTSTRAP_STAGE_LOGS_DIR)/support.cc.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/support.cc.stderr.log"
	@echo "[29A] support objects ready: $(BOOTSTRAP_SUPPORT_OBJ)"

bootstrap-stage0: $(TARGET) $(BOOTSTRAP_COMPILER_SRC) | $(BOOTSTRAP_PHASE29_OUT) $(BOOTSTRAP_STAGE0_DIR) $(BOOTSTRAP_STAGE_LOGS_DIR)
	@echo "[29A] Building stage0..."
	@./cct "$(BOOTSTRAP_COMPILER_SRC)" >"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage0.host.stdout.log" 2>"$(BOOTSTRAP_STAGE_LOGS_DIR)/stage0.host.stderr.log"
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
	@echo "  test-legacy            - Run pre-bootstrap test block (phases before 21)"
	@echo "  test-bootstrap         - Run bootstrap blocks (phases 21-26)"
	@echo "  test-bootstrap-lexer   - Run bootstrap lexer block (phase 21)"
	@echo "  test-bootstrap-parser  - Run bootstrap parser blocks (phases 22-23)"
	@echo "  test-bootstrap-semantic - Run bootstrap semantic blocks (phases 24-25)"
	@echo "  test-bootstrap-codegen - Run bootstrap codegen block (phase 26)"
	@echo "  test-bootstrap-selfhost - Run bootstrap self-hosting block (phase 29)"
	@echo "  bootstrap-stage0 - Build bootstrap compiler stage0 under out/bootstrap/phase29/"
	@echo "  bootstrap-support - Build host support object for self-hosting stages"
	@echo "  bootstrap-stage1 - Self-compile compiler with stage0"
	@echo "  bootstrap-stage2 - Self-compile compiler with stage1"
	@echo "  bootstrap-stage-identity - Validate stage1/stage2 identity"
	@echo "  bootstrap-stage-bench - Benchmark stage pipeline"
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

.PHONY: all clean test test-legacy test-bootstrap test-bootstrap-lexer test-bootstrap-parser test-bootstrap-semantic test-bootstrap-codegen test-bootstrap-selfhost test-phase test_fluxus_storage test_diagnostic_taxonomy dist release install uninstall help fmt fmt-check lint lbos-bridge bootstrap-support bootstrap-stage0 bootstrap-stage1 bootstrap-stage2 bootstrap-stage-diff bootstrap-stage-identity bootstrap-stage-bench project-build project-run project-test project-test-strict project-bench project-clean doc doc-strict release-check phase12-final-audit
