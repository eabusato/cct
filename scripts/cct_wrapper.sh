#!/bin/sh

set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

MODE="${CCT_WRAPPER_MODE:-default}"
HOST_BIN="$ROOT_DIR/cct.bin"
MODE_FILE="$ROOT_DIR/.cct/toolchain/default_mode.txt"
PHASE30_STAGE2="$ROOT_DIR/out/bootstrap/phase29/stage2/cct_stage2"
PHASE30_SUPPORT="$ROOT_DIR/out/bootstrap/phase29/support/selfhost_support.o"
PHASE30_PARSER="$ROOT_DIR/out/bootstrap/phase30/tools/cct_parser_bootstrap"
PHASE30_SEMANTIC="$ROOT_DIR/out/bootstrap/phase30/tools/cct_semantic_bootstrap"
PHASE30_CODEGEN="$ROOT_DIR/out/bootstrap/phase30/tools/cct_codegen_bootstrap"
PHASE31_LEXER="$ROOT_DIR/out/bootstrap/phase31/tools/cct_lexer_bootstrap"

ensure_make_target() {
  target="$1"
  if make --no-print-directory "$target" >/dev/null 2>&1; then
    return 0
  fi
  make --no-print-directory "$target"
}

ensure_bootstrap_artifact() {
  target="$1"
  artifact="$2"
  if [ -e "$artifact" ]; then
    return 0
  fi
  ensure_make_target "$target"
}

parse_project_dir() {
  shift
  project="."
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project)
        [ "$#" -ge 2 ] || {
          echo "error: missing argument for --project" >&2
          return 2
        }
        project="$2"
        shift 2
        ;;
      *)
        shift
        ;;
    esac
  done
  printf '%s\n' "$project"
}

run_project_make() {
  target="$1"
  shift
  project="."
  pattern=""
  entry=""
  output=""
  release_flag=0
  run_args_started=0
  run_arg_count=0

  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project)
        [ "$#" -ge 2 ] || {
          echo "error: missing argument for --project" >&2
          return 2
        }
        project="$2"
        shift 2
        ;;
      --pattern)
        [ "$#" -ge 2 ] || {
          echo "error: missing argument for --pattern" >&2
          return 2
        }
        pattern="$2"
        shift 2
        ;;
      --entry)
        [ "$#" -ge 2 ] || {
          echo "error: missing argument for --entry" >&2
          return 2
        }
        entry="$2"
        shift 2
        ;;
      -o|--output)
        [ "$#" -ge 2 ] || {
          echo "error: missing argument for $1" >&2
          return 2
        }
        output="$2"
        shift 2
        ;;
      --release)
        release_flag=1
        shift
        ;;
      --)
        shift
        run_args_started=1
        break
        ;;
      *)
        if [ "$run_args_started" -eq 1 ]; then
          break
        fi
        echo "error: unsupported project argument: $1" >&2
        return 64
        ;;
    esac
  done

  run_args_value=""
  if [ "$run_args_started" -eq 1 ]; then
    while [ "$#" -gt 0 ]; do
      if [ "$run_arg_count" -eq 0 ]; then
        run_args_value="$1"
      else
        run_args_value="$run_args_value $1"
      fi
      run_arg_count=$((run_arg_count + 1))
      shift
    done
  fi

  set -- "PROJECT=$project"
  [ -n "$pattern" ] && set -- "$@" "PATTERN=$pattern"
  [ -n "$entry" ] && set -- "$@" "ENTRY=$entry"
  [ -n "$output" ] && set -- "$@" "OUT=$output"
  [ "$release_flag" -eq 1 ] && set -- "$@" "RELEASE=1"
  [ -n "$run_args_value" ] && set -- "$@" "ARGS=$run_args_value"

  exec make --no-print-directory "$target" "$@"
}

run_project_command() {
  command_name="$1"
  shift
  case "$command_name" in
    build)
      run_project_make project-selfhost-build "$@"
      ;;
    run)
      run_project_make project-selfhost-run "$@"
      ;;
    test)
      run_project_make project-selfhost-test "$@"
      ;;
    bench)
      run_project_make project-selfhost-bench "$@"
      ;;
    clean)
      run_project_make project-selfhost-clean "$@"
      ;;
    package)
      run_project_make project-selfhost-package "$@"
      ;;
    *)
      echo "error: unsupported project command: $command_name" >&2
      exit 64
      ;;
  esac
}

print_compile_artifacts() {
  input="$1"
  output="$2"
  c_file="$3"
  base_input="${input%.cct}"
  if [ -f "${base_input}.svg" ]; then
    echo "Sigil SVG: ${base_input}.svg"
  fi
  if [ -f "${base_input}.sigil" ]; then
    echo "Sigil Meta: ${base_input}.sigil"
  fi
  if [ -f "${base_input}.system.svg" ]; then
    echo "Sigil SVG: ${base_input}.system.svg"
  fi
  if [ -f "${base_input}.system.sigil" ]; then
    echo "Sigil Meta: ${base_input}.system.sigil"
  fi
  echo "Compiled: $input -> $output"
  echo "Intermediate C: $c_file"
}

compile_via_selfhost() {
  [ "$#" -ge 1 ] || {
    echo "Usage: cct-selfhost <input.cct> [output]" >&2
    exit 64
  }

  input="$1"
  shift

  output=""
  if [ "$#" -eq 0 ]; then
    output="${input%.cct}"
  elif [ "$#" -eq 2 ] && [ "$1" = "-o" ]; then
    output="$2"
  elif [ "$#" -eq 1 ]; then
    output="$1"
  else
    echo "error: unsupported self-host compile arguments" >&2
    exit 64
  fi

  # Ensure bootstrap is ready only when artifacts are absent.
  if [ ! -x "$PHASE30_STAGE2" ] || [ ! -f "$PHASE30_SUPPORT" ]; then
    echo "[selfhost] bootstrap artifacts missing; preparing stage2 toolchain..." >&2
    make --no-print-directory bootstrap-selfhost-ready || {
      echo "error: failed to prepare self-hosted compiler" >&2
      exit 1
    }
  fi

  c_file="${output}.cgen.c"
  "$PHASE30_STAGE2" "$input" "$c_file" || {
    echo "error: compilation failed" >&2
    exit 1
  }

  cc_bin="${CC:-gcc}"
  "$cc_bin" -O2 -o "$output" "$c_file" "$PHASE30_SUPPORT" "$ROOT_DIR/src/runtime/fs_runtime.c" -lsqlite3 || {
    echo "error: C compilation failed" >&2
    exit 1
  }
  print_compile_artifacts "$input" "$output" "$c_file"
}

should_dispatch_compile_to_host() {
  [ "$#" -ge 1 ] || return 0

  input="$1"
  shift

  case "$input" in
    -*)
      return 1
      ;;
  esac

  case "$input" in
    *.cct)
      ;;
    *)
      return 0
      ;;
  esac

  [ -f "$input" ] || return 0
  return 1
}

dispatch_host() {
  exec "$HOST_BIN" "$@"
}

dispatch_default() {
  active="selfhost"
  if [ -f "$MODE_FILE" ]; then
    active="$(tr -d '[:space:]' < "$MODE_FILE")"
  fi

  if [ "${1:-}" = "--which-compiler" ]; then
    if [ "$active" = "selfhost" ]; then
      printf 'selfhost\n'
    else
      printf 'host\n'
    fi
    exit 0
  fi

  if [ "$active" = "selfhost" ]; then
    export CCT_WRAPPER_MODE=selfhost
    exec "$SCRIPT_DIR/cct_wrapper.sh" "$@"
  fi

  dispatch_host "$@"
}

dispatch_selfhost() {
  if [ "${1:-}" = "--which-compiler" ]; then
    printf 'selfhost\n'
    exit 0
  fi

  case "${1:-}" in
    "")
      dispatch_host "$@"
      ;;
    build|run|test|bench|clean|package)
      ensure_make_target bootstrap-selfhost-ready
      run_project_command "$@"
      ;;
    --check)
      [ "$#" -eq 2 ] || dispatch_host "$@"
      ensure_bootstrap_artifact bootstrap-selfhost-semantic "$PHASE30_SEMANTIC"
      exec "$PHASE30_SEMANTIC" "$2"
      ;;
    --ast)
      [ "$#" -eq 2 ] || dispatch_host "$@"
      ensure_bootstrap_artifact bootstrap-selfhost-parser "$PHASE30_PARSER"
      printf 'Parsing: %s\n' "$2"
      printf '========================================\n\n'
      exec "$PHASE30_PARSER" "$2"
      ;;
    --tokens)
      [ "$#" -eq 2 ] || dispatch_host "$@"
      ensure_bootstrap_artifact bootstrap-selfhost-lexer "$PHASE31_LEXER"
      exec "$PHASE31_LEXER" "$2"
      ;;
    --sigilo-only|fmt|lint|doc|sigilo|--version|--help|-h)
      dispatch_host "$@"
      ;;
    --*)
      dispatch_host "$@"
      ;;
    *)
      if should_dispatch_compile_to_host "$@"; then
        dispatch_host "$@"
      fi
      compile_via_selfhost "$@"
      ;;
  esac
}

case "$MODE" in
  host)
    dispatch_host "$@"
    ;;
  selfhost)
    dispatch_selfhost "$@"
    ;;
  default)
    dispatch_default "$@"
    ;;
  *)
    echo "error: invalid wrapper mode: $MODE" >&2
    exit 64
    ;;
esac
