PHP_ARG_ENABLE(html2img, whether to enable html2img, [  --enable-html2img   Enable html2img extension], no, yes)

if test "$PHP_HTML2IMG" != "no"; then
  PHP_REQUIRE_CXX()
  PHP_ADD_INCLUDE([3rdparty/litehtml/include])
  PHP_ADD_LIBRARY_WITH_PATH([gd], [], HTML2IMG_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH([freetype], [], HTML2IMG_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH([png], [], HTML2IMG_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH([jpeg], [], HTML2IMG_SHARED_LIBADD)
  PHP_NEW_EXTENSION(html2img, src/php_html2img.cpp src/gd_canvas.cpp src/gd_container.cpp src/cache.cpp src/ft_cache.cpp, $ext_shared,, [-std=c++17])
  PHP_SUBST(HTML2IMG_SHARED_LIBADD)
fi
