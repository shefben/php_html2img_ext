#!/usr/bin/env bash
# Clone RmlUi and build the extension on Linux
# Environment variables:
#   PHP_PREFIX   - path prefix to phpize/php-config if not in PATH
#   CAIRO_LIB    - path to libcairo.so (optional)
#   FREETYPE_LIB - path to libfreetype.so (optional)
#   PNG_LIB      - path to libpng.so (optional)
#   JPEG_LIB     - path to libjpeg.so (optional)
#   GIF_LIB      - path to libgif.so (optional)
set -euo pipefail

RMLUI_DIR=3rdparty/RmlUi
if [ ! -d "$RMLUI_DIR" ]; then
    git clone --depth=1 https://github.com/mikke89/RmlUi.git "$RMLUI_DIR"
    (cd "$RMLUI_DIR" && git submodule update --init --recursive)
    sed -i 's/option(BUILD_SHARED_LIBS.*/option(BUILD_SHARED_LIBS "CMake standard option. Choose whether to build shared RmlUi libraries." OFF)/' "$RMLUI_DIR"/CMakeLists.txt
fi

PHPIZE=${PHP_PREFIX:+$PHP_PREFIX/bin/}phpize
$PHPIZE
./configure --enable-html2img
make -j"$(nproc)"
