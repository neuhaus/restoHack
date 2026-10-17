#!/bin/sh
# wasm/build-wasm.sh - configure and build the browser port of restoHack.
#
# Needs: emsdk (emcc + emcmake on PATH), cmake, a host C compiler (cc),
# curl and network access for the pinned browser assets below.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build/wasm}"      # the servable bundle
OBJ_DIR="${OBJ_DIR:-$ROOT/build/wasm-build}"    # CMake working dir

if ! command -v emcc >/dev/null 2>&1; then
  echo "error: emcc not found. Install the Emscripten SDK first:" >&2
  echo "  git clone https://github.com/emscripten-core/emsdk.git" >&2
  echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest" >&2
  echo "  source ./emsdk_env.sh" >&2
  echo "See wasm/README.md for details." >&2
  exit 1
fi

# Pinned browser assets (@xterm/xterm + xterm-pty, both MIT): fetched once,
# checksum-verified, skipped if a good copy is already present. The bundle
# references only these local files, so the page works offline at runtime.
fetch() { # URL DEST SHA256
  if [ -f "$2" ] && [ "$(sha256sum "$2" | cut -d' ' -f1)" = "$3" ]; then
    return 0
  fi
  if ! curl -fsSL "$1" -o "$2"; then
    echo "error: failed to fetch $1" >&2
    exit 1
  fi
  if [ "$(sha256sum "$2" | cut -d' ' -f1)" != "$3" ]; then
    echo "error: checksum mismatch for $2" >&2
    exit 1
  fi
}

mkdir -p "$BUILD_DIR" "$OBJ_DIR"
XTERM=https://unpkg.com/@xterm/xterm@5.5.0
PTY=https://unpkg.com/xterm-pty@0.12.0
fetch "$XTERM/lib/xterm.js"    "$BUILD_DIR/xterm.js"           1f991ac3b4b283ebf96e60ae23a00a52765dd3a2e46fa6fdda9f1aab032f7495
fetch "$XTERM/css/xterm.css"   "$BUILD_DIR/xterm.css"          ba8e6985669488981ccf40c0cefe3aba80722cb6c92de7ad628b0bd717faf2b6
fetch "$PTY/index.mjs"         "$BUILD_DIR/xterm-pty.mjs"      9d1ab8c5f3af23ddc835f9cf61f3db7097ad338e4655ae20a7dc263adee552e2
# The link step references the glue at this gitignored source-tree path.
fetch "$PTY/emscripten-pty.js" "$ROOT/wasm/emscripten-pty.js"  93d474cb4141461145c6a01aa76bbe6f75313b562051519c8bcd21d367125315

emcmake cmake -S "$ROOT" -B "$OBJ_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$OBJ_DIR"

# Stage the servable bundle: the page, the Emscripten output and the
# fetched assets - nothing else.
cp "$OBJ_DIR/hack.mjs" "$OBJ_DIR/hack.wasm" "$OBJ_DIR/hack.data" "$BUILD_DIR/"
cp "$ROOT/wasm/index.html" "$BUILD_DIR/index.html"

echo
echo "Built: $BUILD_DIR (self-contained; serve it and open /):"
echo "  cd $BUILD_DIR && python3 -m http.server 8000"
echo "  -> http://localhost:8000/"
