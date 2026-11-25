#!/bin/sh
# Build dist/HioudenII-macOS.zip: a universal (Apple Silicon + Intel) "Hiouden II.app", macOS 11+.
# Needs: build/dist-src/sdl-mac-inst (universal SDL3), packaging/HioudenII.icns (tools/mkicon.py), and the English
# patches from `python3 tools/release.py --version $VERSION` (dist/Hiouden2_EN_v$VERSION/patches).
# The app contains the recomp, SDL3 and the patches only; no game files.
set -e
cd "$(dirname "$0")/.."
VERSION=${VERSION:-1.0}
SDL=build/dist-src/sdl-mac-inst
PATCHES=dist/Hiouden2_EN_v$VERSION/patches
[ -d "$PATCHES" ] || { echo "run: python3 tools/release.py --version $VERSION"; exit 1; }
OUT=build/rel-mac; mkdir -p $OUT
echo "compiling (universal)..."
clang++ -std=c++17 -O2 -arch arm64 -arch x86_64 -mmacosx-version-min=11.0 -Isrc -I$SDL/include \
  src/main.cpp src/launcher.cpp src/installer.cpp src/cpu/cpu386.cpp src/hw/ega_vga.cpp src/hw/audio.cpp \
  src/hw/dos_runtime.cpp -L$SDL/lib -lSDL3 -Wl,-rpath,@executable_path/../Frameworks -o $OUT/HioudenII

APP="dist/Hiouden II.app"
rm -rf "$APP"; mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks" "$APP/Contents/Resources"
cp $OUT/HioudenII "$APP/Contents/MacOS/HioudenII"
strip -x "$APP/Contents/MacOS/HioudenII"   # no local symbols or build paths
cp $SDL/lib/libSDL3.0.dylib "$APP/Contents/Frameworks/"
cp packaging/HioudenII.icns "$APP/Contents/Resources/"
cp -R "$PATCHES" "$APP/Contents/Resources/patches"
cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>Hiouden II</string>
  <key>CFBundleDisplayName</key><string>Hiouden II</string>
  <key>CFBundleIdentifier</key><string>com.secondsave.hiouden2</string>
  <key>CFBundleExecutable</key><string>HioudenII</string>
  <key>CFBundleIconFile</key><string>HioudenII</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>$VERSION</string>
  <key>CFBundleVersion</key><string>$VERSION</string>
  <key>LSMinimumSystemVersion</key><string>11.0</string>
  <key>LSApplicationCategoryType</key><string>public.app-category.strategy-games</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>NSHumanReadableCopyright</key><string>Recomp and English translation by Second Save. Requires your own copy of Hiouden II (Taiwanese DOS release).</string>
</dict></plist>
PLIST
codesign --force --deep -s - "$APP"

STAGE=$OUT/stage; rm -rf "$STAGE"; mkdir -p "$STAGE/Hiouden II"
cp -R "$APP" "$STAGE/Hiouden II/"
cp packaging/HOW-TO-PLAY.txt "$STAGE/Hiouden II/"
cp font/LICENSE-spleen "$STAGE/Hiouden II/LICENSE-spleen.txt"
cp build/dist-src/SDL3-*/LICENSE.txt "$STAGE/Hiouden II/SDL-LICENSE.txt"
rm -f dist/HioudenII-macOS.zip
(cd "$STAGE" && ditto -c -k --norsrc --noextattr --noqtn --keepParent "Hiouden II" ../../../dist/HioudenII-macOS.zip)
lipo -info "$APP/Contents/MacOS/HioudenII"
ls -la dist/HioudenII-macOS.zip
