#!/usr/bin/env bash
set -euo pipefail
ROOT="$(pwd)"
TOOLCHAIN="${LYNX_TOOLCHAIN:-$HOME/bin/lynxdev/llvm-mos}"
CXX="$TOOLCHAIN/bin/mos-lynx-bll-clang++"
LD="$TOOLCHAIN/bin/ld.lld"

SRC_MAIN="$ROOT/arduboy-lynx/examples/toolchain-test/main.cpp"
SRC_LYNX="$ROOT/arduboy-lynx/platform/lynx/Lynx.cpp"

OBJ_MAIN="$ROOT/main.o"
OBJ_LYNX="$ROOT/lynx.o"
OUT="$ROOT/toolchain-test.bll.o"

"$CXX" -Os -flto -I"$ROOT/arduboy-lynx/platform/lynx" -c "$SRC_MAIN" -o "$OBJ_MAIN"
"$CXX" -Os -flto -I"$ROOT/arduboy-lynx/platform/lynx" -c "$SRC_LYNX" -o "$OBJ_LYNX"

"$LD" --gc-sections --sort-section=alignment "$OBJ_MAIN" "$OBJ_LYNX" \
  -plugin-opt=mcpu=mos65c02 -plugin-opt=O2 \
  -L"$TOOLCHAIN/mos-platform/lynx-bll/lib" -L"$TOOLCHAIN/mos-platform/lynx/lib" -L"$TOOLCHAIN/mos-platform/common/lib" \
  -l:crt0.o -lcrt0 -lcrt -lc \
  -T"$ROOT/arduboy-lynx/build/lynx-bll/link.ld" -o "$OUT"

echo "OK: $OUT ($(stat -c%s "$OUT") bytes)"
