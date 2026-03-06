#!/usr/bin/env bash

# Configures a writable temp workspace for test scripts.
# Defaults to project-local tests/.tmp to avoid /tmp permission issues.
cct_setup_tmpdir() {
  local project_root="$1"
  if [ -z "${project_root}" ]; then
    echo "cct_setup_tmpdir: missing project root" >&2
    return 1
  fi

  if [ -z "${CCT_TMP_DIR:-}" ]; then
    CCT_TMP_DIR="${project_root}/tests/.tmp"
  fi

  mkdir -p "${CCT_TMP_DIR}"
  export CCT_TMP_DIR
  export TMPDIR="${CCT_TMP_DIR}"
}
