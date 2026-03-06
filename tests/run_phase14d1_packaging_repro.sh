#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
source "$ROOT_DIR/tests/test_tmpdir.sh"
cct_setup_tmpdir "$ROOT_DIR"
TMP_DIR="$CCT_TMP_DIR/cct_phase14d1_packaging"
DIST_A="$TMP_DIR/dist_a"
DIST_B="$TMP_DIR/dist_b"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

make -C "$ROOT_DIR" dist DIST_DIR="$DIST_A" >$CCT_TMP_DIR/cct_phase14d1_dist_a.out 2>&1
make -C "$ROOT_DIR" dist DIST_DIR="$DIST_B" >$CCT_TMP_DIR/cct_phase14d1_dist_b.out 2>&1

for d in "$DIST_A" "$DIST_B"; do
  [ -f "$d/CHECKSUMS.sha256" ] || { echo "phase14d1: missing CHECKSUMS in $d" >&2; exit 1; }
  [ -f "$d/docs/spec.md" ] || { echo "phase14d1: missing docs/spec.md in $d" >&2; exit 1; }
  [ -d "$d/lib/cct" ] || { echo "phase14d1: missing stdlib tree in $d" >&2; exit 1; }
done

if [ -x "$DIST_A/bin/cct" ]; then
  "$DIST_A/bin/cct" --help >$CCT_TMP_DIR/cct_phase14d1_help.out 2>&1
elif [ -f "$DIST_A/bin/cct.bat" ]; then
  "$DIST_A/bin/cct.bat" --help >$CCT_TMP_DIR/cct_phase14d1_help.out 2>&1
else
  echo "phase14d1: missing runnable cct wrapper in dist bundle" >&2
  exit 1
fi

if ! grep -q "Usage:" $CCT_TMP_DIR/cct_phase14d1_help.out; then
  echo "phase14d1: packaged wrapper did not produce help usage" >&2
  exit 1
fi

if ! cmp -s "$DIST_A/CHECKSUMS.sha256" "$DIST_B/CHECKSUMS.sha256"; then
  echo "phase14d1: checksum manifests differ across repeated dist builds" >&2
  exit 1
fi

echo "phase14d1: reproducible packaging ok"
