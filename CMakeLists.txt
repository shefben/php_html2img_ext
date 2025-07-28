cmake_minimum_required(VERSION 3.10)
project(html2img CXX)

option(BUILD_TESTS "Build unit tests" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

execute_process(COMMAND php-config --includes OUTPUT_VARIABLE PHP_INCLUDES OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "-I" "" PHP_INCLUDES "${PHP_INCLUDES}")
separate_arguments(PHP_INCLUDES)

find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(PNG REQUIRED libpng)
pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(GIF REQUIRED libgif)
find_package(OpenSSL REQUIRED)

add_subdirectory(3rdparty/RmlUi)
set_property(TARGET rmlui_core PROPERTY POSITION_INDEPENDENT_CODE ON)

if(PHP_INCLUDES)
    include_directories(${PHP_INCLUDES} 3rdparty/RmlUi/Include ${CAIRO_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS} ${GIF_INCLUDE_DIRS})
    add_library(html2img SHARED
        php-8.4.10-devel-vs17-x64/src/php_html2img.cpp
        php-8.4.10-devel-vs17-x64/src/cairo_canvas.cpp
        php-8.4.10-devel-vs17-x64/src/rml_cairo_renderer.cpp
        php-8.4.10-devel-vs17-x64/src/path_utils.cpp)
    target_link_libraries(html2img rmlui_core ${CAIRO_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${GIF_LIBRARIES} Freetype::Freetype OpenSSL::Crypto)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_executable(cairo_backend_test tests/cairo_backend_test.cpp
        php-8.4.10-devel-vs17-x64/src/cairo_canvas.cpp
        php-8.4.10-devel-vs17-x64/src/rml_cairo_renderer.cpp
        php-8.4.10-devel-vs17-x64/src/path_utils.cpp)
    target_link_libraries(cairo_backend_test rmlui_core ${CAIRO_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${GIF_LIBRARIES} Freetype::Freetype OpenSSL::Crypto gtest gtest_main)
    add_test(NAME cairo_backend_test COMMAND cairo_backend_test)
endif()
