#!/bin/sh
# Build dist/HioudenII-Linux-x86_64.tar.gz in an Ubuntu 22.04 (glibc 2.35) x86-64 container, with SDL3 built from
# source and bundled (lib/, found through an $ORIGIN rpath). Needs: SDL3 source in build/dist-src (SDL3-<v>/),
# dist/Hiouden2_EN_v$VERSION/patches (tools/release.py). On Apple Silicon this runs emulated (slow).
# Contains the recomp, SDL3 and the English patches only; no game files.
set -e
cd "$(dirname "$0")/.."
VERSION=${VERSION:-1.0}
PATCHES=dist/Hiouden2_EN_v$VERSION/patches
[ -d "$PATCHES" ] || { echo "run: python3 tools/release.py --version $VERSION"; exit 1; }
SDLSRC=$(ls -d build/dist-src/SDL3-*/ | head -1)
[ -f "$SDLSRC/CMakeLists.txt" ] || { echo "SDL3 source not found in build/dist-src"; exit 1; }
docker run --rm --platform linux/amd64 -v "$PWD":/src -w /src ubuntu:22.04 sh -c '
set -e
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq >/dev/null
apt-get install -y -qq build-essential cmake ninja-build pkg-config libasound2-dev libpulse-dev \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
  libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libegl1-mesa-dev libdbus-1-dev libudev-dev \
  libwayland-dev wayland-protocols libdecor-0-dev patchelf >/dev/null 2>&1
OUT=build/rel-linux; mkdir -p $OUT
if [ ! -f $OUT/sdl-inst/lib/libSDL3.so ]; then
  cmake -S '"$SDLSRC"' -B $OUT/sdl -G Ninja -DCMAKE_BUILD_TYPE=Release -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
    -DCMAKE_INSTALL_PREFIX=/src/$OUT/sdl-inst >/dev/null
  cmake --build $OUT/sdl >/dev/null && cmake --install $OUT/sdl >/dev/null
fi
g++ -std=c++17 -O2 -Isrc -I$OUT/sdl-inst/include \
  src/main.cpp src/launcher.cpp src/installer.cpp src/cpu/cpu386.cpp src/hw/ega_vga.cpp src/hw/audio.cpp \
  src/hw/dos_runtime.cpp -L$OUT/sdl-inst/lib -lSDL3 -o $OUT/hiouden2 > $OUT.log 2>&1 || { grep -E "error" $OUT.log | head -30; exit 1; }
strip $OUT/hiouden2
patchelf --set-rpath "\$ORIGIN/lib" $OUT/hiouden2   # bundled SDL3 in lib/ next to the program
ldd $OUT/hiouden2 | sed "s/ (0x.*//"
objdump -T $OUT/hiouden2 | grep -o "GLIBC_[0-9.]*" | sort -V | tail -1
if [ -d hio2 ]; then  # smoke test: install the English copy from a scratch copy of the game (developer machine)
  mkdir -p /tmp/t && cp -R hio2 /tmp/t/src && HIOUDEN_HOME=/tmp/t/home LD_LIBRARY_PATH=$OUT/sdl-inst/lib \
    ./$OUT/hiouden2 --install /tmp/t/src
fi
'
STAGE=build/rel-linux/stage; rm -rf "$STAGE"; mkdir -p "$STAGE/Hiouden II/lib"
cp build/rel-linux/hiouden2 "$STAGE/Hiouden II/"
cp -P build/rel-linux/sdl-inst/lib/libSDL3.so.0* "$STAGE/Hiouden II/lib/"
cp -R "$PATCHES" "$STAGE/Hiouden II/patches"
cp packaging/HOW-TO-PLAY.txt "$STAGE/Hiouden II/"
cp font/LICENSE-spleen "$STAGE/Hiouden II/LICENSE-spleen.txt"
cp "$SDLSRC/LICENSE.txt" "$STAGE/Hiouden II/SDL-LICENSE.txt"
cp packaging/icon_1024.png "$STAGE/Hiouden II/hiouden2.png"
rm -f dist/HioudenII-Linux-x86_64.tar.gz
# neutral owner in the archive (no local user name)
tar -C "$STAGE" --uid 0 --gid 0 --uname root --gname root -czf dist/HioudenII-Linux-x86_64.tar.gz "Hiouden II"
ls -la dist/HioudenII-Linux-x86_64.tar.gz
