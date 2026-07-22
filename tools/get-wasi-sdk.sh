#!/usr/bin/env bash
# Download a self-contained wasi-sdk (clang + wasi-sysroot + wasm builtins) into
# a repo-local .toolchains/ dir, so the custom chip can be built on the host and
# offline afterwards. No system changes; .toolchains/ is gitignored.
#
# The download runs on the host, which already trusts your corporate proxy CA
# (unlike a fresh container). Run once with a connection, then build offline:
#   ./tools/get-wasi-sdk.sh    # or: make chip-local
#
# Override the version with WASI_SDK_VER (e.g. WASI_SDK_VER=25.0 ./tools/get-wasi-sdk.sh).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$REPO_ROOT/.toolchains/wasi-sdk"
VER="${WASI_SDK_VER:-33.0}"
MAJOR="${VER%%.*}"

if [ -x "$DEST/bin/clang" ]; then
  echo "wasi-sdk already installed: $DEST"
  "$DEST/bin/clang" --version | head -1
  exit 0
fi

os="$(uname -s)"; arch="$(uname -m)"
case "$os"   in Darwin) OS=macos ;; Linux) OS=linux ;; *) echo "unsupported OS: $os"   >&2; exit 1 ;; esac
case "$arch" in arm64|aarch64) A=arm64 ;; x86_64|amd64) A=x86_64 ;; *) echo "unsupported arch: $arch" >&2; exit 1 ;; esac

URL="https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${MAJOR}/wasi-sdk-${VER}-${A}-${OS}.tar.gz"
echo "Downloading wasi-sdk ${VER} (${A}-${OS})"
echo "  $URL"
mkdir -p "$DEST"
tmp="$(mktemp)"
curl -fL --retry 3 -o "$tmp" "$URL"
tar -xzf "$tmp" -C "$DEST" --strip-components=1
rm -f "$tmp"
# Clear macOS Gatekeeper quarantine so the downloaded clang can run.
if [ "$OS" = macos ]; then xattr -dr com.apple.quarantine "$DEST" 2>/dev/null || true; fi

echo "Installed -> $DEST"
"$DEST/bin/clang" --version | head -1
echo "Build the chip with:  make chip   (auto-detects .toolchains/wasi-sdk)"
