#!/usr/bin/env bash
# Build the extension using the system PHP
# Environment variables:
#   PHP_PREFIX   - prefix to phpize/php-config if not in PATH
set -euo pipefail
PHPIZE=${PHP_PREFIX:+$PHP_PREFIX/bin/}phpize
$PHPIZE
./configure --enable-html2img
make -j"$(nproc)"
