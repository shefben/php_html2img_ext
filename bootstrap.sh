#!/usr/bin/env bash
# Clone litehtml and build the extension on Linux
# Environment variables:
#   PHP_PREFIX   - path prefix to phpize/php-config if not in PATH
#   CAIRO_LIB    - path to libcairo.so (optional)
#   FREETYPE_LIB - path to libfreetype.so (optional)
#   PNG_LIB      - path to libpng.so (optional)
#   JPEG_LIB     - path to libjpeg.so (optional)
#   GIF_LIB      - path to libgif.so (optional)
set -euo pipefail

LITEHTML_DIR=3rdparty/litehtml
if [ ! -d "$LITEHTML_DIR" ]; then
    git clone --depth=1 https://github.com/litehtml/litehtml.git "$LITEHTML_DIR"
    (cd "$LITEHTML_DIR" && git submodule update --init --recursive)
    patch -d "$LITEHTML_DIR" -p1 < patches/litehtml-static.patch
fi

PHPIZE=${PHP_PREFIX:+$PHP_PREFIX/bin/}phpize
$PHPIZE
./configure --enable-html2img
make -j"$(nproc)"
