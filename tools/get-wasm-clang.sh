#!/usr/bin/env bash
# Vendor an in-browser C++ -> WASM toolchain (clang + lld compiled to wasm, plus a
# wasi sysroot) into docs/app/vendor/wasm-clang/ so the Studio's C++ tab can
# compile the student's controller entirely client-side — no server (G3).
#
# Source: binji/wasm-clang (Apache-2.0) — the canonical client-side clang demo
# (Ben Smith, CppCon 2019). We self-host the binaries (rather than hotlink raw
# GitHub) to control MIME types and avoid rate limits. Large + machine-agnostic,
# so it is gitignored and fetched on demand, exactly like the wasi-sdk toolchain.
#
# Usage:  ./tools/get-wasm-clang.sh          # fetch if missing
#         ./tools/get-wasm-clang.sh --force  # re-fetch
#
# NOTE: ~60 MB. Requires network (the host trusts the corp proxy CA, so this works
# even where the dev container's TLS is intercepted — see docs/references notes).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$REPO_ROOT/docs/app/vendor/wasm-clang"
BASE="https://raw.githubusercontent.com/binji/wasm-clang/master"

# name  : upstream file (no extension on the wasm blobs, by design)
FILES=(clang lld memfs sysroot.tar shared.js)

force=0
[[ "${1:-}" == "--force" ]] && force=1

if [[ $force -eq 0 && -f "$DEST/clang" && -f "$DEST/lld" && -f "$DEST/sysroot.tar" ]]; then
  echo "wasm-clang already present in $DEST (use --force to re-fetch)"
  exit 0
fi

mkdir -p "$DEST"
for f in "${FILES[@]}"; do
  echo "fetching $f ..."
  curl -fSL --retry 3 -o "$DEST/$f" "$BASE/$f"
done

echo "wasm-clang vendored -> $DEST"
ls -lh "$DEST"
