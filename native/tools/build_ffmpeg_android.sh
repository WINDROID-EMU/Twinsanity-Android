#!/bin/sh
set -eu

VERSION=9.0.2
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SOURCES=$ROOT/native/third_party/ffmpeg-src

NDK=/home/anthero/Android/Sdk/ndk/28.2.13676358
TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-x86_64
API=24
TARGET=aarch64-linux-android
CC=$TOOLCHAIN/bin/${TARGET}${API}-clang
CXX=$TOOLCHAIN/bin/${TARGET}${API}-clang++
AR=$TOOLCHAIN/bin/llvm-ar
RANLIB=$TOOLCHAIN/bin/llvm-ranlib
STRIP=$TOOLCHAIN/bin/llvm-strip

PREFIX=$ROOT/build/android/ffmpeg/android-arm64

# O configure do FFmpeg falha com espaço no caminho quando chamado de outro diretório (out-of-tree).
# Portanto, executamos in-tree diretamente dentro da pasta extraída do ffmpeg.
cd "$SOURCES/ffmpeg-$VERSION"

echo "Limpando build anterior..."
make distclean 2>/dev/null || true

echo "Configurando FFmpeg in-tree para Android arm64-v8a..."
./configure \
    --prefix="$PREFIX" \
    --enable-cross-compile \
    --target-os=android \
    --arch=aarch64 \
    --cpu=armv8-a \
    --cc="$CC" \
    --cxx="$CXX" \
    --ar="$AR" \
    --ranlib="$RANLIB" \
    --strip="$STRIP" \
    --extra-cflags="-fPIC -DANDROID" \
    --extra-ldflags="-Wl,-Bsymbolic" \
    --disable-everything \
    --disable-gpl --disable-nonfree --disable-version3 \
    --disable-programs --disable-doc --disable-network --disable-autodetect \
    --disable-avdevice --disable-avformat --disable-avfilter --disable-swresample --disable-swscale \
    --enable-decoder=mpeg2video --enable-parser=mpegvideo \
    --enable-shared --disable-static

echo "Compilando FFmpeg..."
make -j"$(nproc)"
make install
echo "FFmpeg compilado com sucesso em $PREFIX!"
