#!/usr/bin/env bash

set -euo pipefail

supports_gnu_m32() {
    local cc="$1"
    /bin/sh -c "printf 'int __cct_probe = 0;\n' | \"$cc\" \
        -m32 -ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-builtin -fwrapv \
        -fno-asynchronous-unwind-tables -fno-unwind-tables \
        -x c -c -o /dev/null - >/dev/null 2>&1 && \
        printf '' | \"$cc\" -m32 -dM -E -x c - 2>/dev/null | grep -q '__i386__'"
}

supports_clang_i386_target() {
    local cc="$1"
    /bin/sh -c "printf 'int __cct_probe = 0;\n' | \"$cc\" \
        -target i386-unknown-none-elf \
        -ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-builtin -fwrapv \
        -fno-asynchronous-unwind-tables -fno-unwind-tables \
        -x c -c -o /dev/null - >/dev/null 2>&1 && \
        printf '' | \"$cc\" -target i386-unknown-none-elf -dM -E -x c - 2>/dev/null | grep -q '__i386__'"
}

resolve_toolchain() {
    local cc=""
    local mode=""
    local candidates=()

    if [ -n "${CCT_CROSS_CC:-}" ]; then
        candidates+=("$CCT_CROSS_CC")
    else
        candidates+=("i686-elf-gcc" "clang" "gcc")
    fi

    for cc in "${candidates[@]}"; do
        if supports_gnu_m32 "$cc"; then
            printf '%s\t%s\n' "$cc" "gnu-m32"
            return 0
        fi
        if supports_clang_i386_target "$cc"; then
            printf '%s\t%s\n' "$cc" "clang-i386-elf"
            return 0
        fi
    done

    return 1
}

run_with_mode() {
    local cc="$1"
    local mode="$2"
    shift 2

    case "$mode" in
        gnu-m32)
            exec "$cc" -m32 "$@"
            ;;
        clang-i386-elf)
            exec "$cc" -target i386-unknown-none-elf "$@"
            ;;
        *)
            echo "unknown freestanding toolchain mode: $mode" >&2
            exit 2
            ;;
    esac
}

usage() {
    cat <<'EOF' >&2
usage:
  tools/freestanding_toolchain.sh resolve
  tools/freestanding_toolchain.sh show-cc
  tools/freestanding_toolchain.sh show-mode
  tools/freestanding_toolchain.sh compile-c <src.c> <out.o>
  tools/freestanding_toolchain.sh assemble <src.s> <out.o>
EOF
    exit 2
}

if [ "$#" -lt 1 ]; then
    usage
fi

cmd="$1"
shift

case "$cmd" in
    resolve)
        resolve_toolchain
        ;;
    show-cc)
        IFS=$'\t' read -r cc mode < <(resolve_toolchain)
        printf '%s\n' "$cc"
        ;;
    show-mode)
        IFS=$'\t' read -r cc mode < <(resolve_toolchain)
        printf '%s\n' "$mode"
        ;;
    compile-c)
        if [ "$#" -ne 2 ]; then
            usage
        fi
        IFS=$'\t' read -r cc mode < <(resolve_toolchain)
        run_with_mode "$cc" "$mode" \
            -ffreestanding -nostdlib -fno-pic -fwrapv \
            -fno-stack-protector -fno-builtin \
            -fno-asynchronous-unwind-tables -fno-unwind-tables \
            -c "$1" -o "$2"
        ;;
    assemble)
        if [ "$#" -ne 2 ]; then
            usage
        fi
        IFS=$'\t' read -r cc mode < <(resolve_toolchain)
        run_with_mode "$cc" "$mode" -c "$1" -o "$2"
        ;;
    *)
        usage
        ;;
esac
