#!/bin/sh
# Build dist/HioudenII-Windows.zip (64-bit Windows 10+) by cross-compiling with MinGW-w64 in Docker.
# Needs: the SDL3 MinGW development package unpacked in build/dist-src (SDL3-<v>/x86_64-w64-mingw32),
# packaging/hiouden2.ico (tools/mkicon.py), and dist/Hiouden2_EN_v$VERSION/patches (tools/release.py).
# Contains the recomp, SDL3 and the English patches only; no game files.
set -e
cd "$(dirname "$0")/.."
VERSION=${VERSION:-1.0}
PATCHES=dist/Hiouden2_EN_v$VERSION/patches
[ -d "$PATCHES" ] || { echo "run: python3 tools/release.py --version $VERSION"; exit 1; }
SDLW=$(ls -d build/dist-src/SDL3-*/x86_64-w64-mingw32 | head -1)
[ -d "$SDLW" ] || { echo "SDL3 MinGW package not found in build/dist-src"; exit 1; }
docker run --rm -v "$PWD":/src -w /src debian:trixie sh -c '
set -e
apt-get update -qq >/dev/null && apt-get install -y -qq g++-mingw-w64-x86-64-posix binutils-mingw-w64-x86-64 >/dev/null 2>&1
OUT=build/rel-win; mkdir -p $OUT
printf "1 ICON \"packaging/hiouden2.ico\"\n" > $OUT/icon.rc
x86_64-w64-mingw32-windres $OUT/icon.rc -O coff -o $OUT/icon.res
x86_64-w64-mingw32-g++-posix -std=c++17 -O2 -Isrc -I'"$SDLW"'/include \
  src/main.cpp src/launcher.cpp src/installer.cpp src/cpu/cpu386.cpp src/hw/ega_vga.cpp src/hw/audio.cpp \
  src/hw/dos_runtime.cpp $OUT/icon.res -L'"$SDLW"'/lib -l:libSDL3.dll.a -mwindows -static -o $OUT/HioudenII.exe \
  > $OUT.log 2>&1 || { grep -E "error" $OUT.log | head -30; exit 1; }
x86_64-w64-mingw32-strip $OUT/HioudenII.exe
x86_64-w64-mingw32-objdump -p $OUT/HioudenII.exe | grep "DLL Name" | sort -u
'
STAGE=build/rel-win/stage; rm -rf "$STAGE"; mkdir -p "$STAGE/Hiouden II"
cp build/rel-win/HioudenII.exe "$STAGE/Hiouden II/"
cp "$SDLW/bin/SDL3.dll" "$STAGE/Hiouden II/"
cp -R "$PATCHES" "$STAGE/Hiouden II/patches"
# Windows readers expect CRLF line endings
sed 's/$/\r/' packaging/HOW-TO-PLAY.txt > "$STAGE/Hiouden II/HOW-TO-PLAY.txt"
sed 's/$/\r/' font/LICENSE-spleen > "$STAGE/Hiouden II/LICENSE-spleen.txt"
sed 's/$/\r/' "$(dirname "$SDLW")/LICENSE.txt" > "$STAGE/Hiouden II/SDL-LICENSE.txt"
rm -f dist/HioudenII-Windows.zip
(cd "$STAGE" && zip -qrX ../../../dist/HioudenII-Windows.zip "Hiouden II")
ls -la dist/HioudenII-Windows.zip
