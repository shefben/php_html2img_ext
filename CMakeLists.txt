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
pkg_check_modules(GD REQUIRED gdlib)
find_package(OpenSSL REQUIRED)

add_subdirectory(3rdparty/litehtml)
set_property(TARGET litehtml PROPERTY POSITION_INDEPENDENT_CODE ON)

if(PHP_INCLUDES)
    include_directories(${PHP_INCLUDES} 3rdparty/litehtml/include)
    add_library(html2img SHARED src/php_html2img.cpp src/gd_canvas.cpp src/gd_container.cpp src/ft_cache.cpp src/cache.cpp)
    target_link_libraries(html2img litehtml ${GD_LIBRARIES} Freetype::Freetype OpenSSL::Crypto)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_executable(gd_backend_test tests/gd_backend_test.cpp src/gd_canvas.cpp src/gd_container.cpp src/ft_cache.cpp src/cache.cpp)
    target_link_libraries(gd_backend_test litehtml ${GD_LIBRARIES} Freetype::Freetype OpenSSL::Crypto gtest gtest_main)
    add_test(NAME gd_backend_test COMMAND gd_backend_test)

    add_test(NAME ConcurrentCacheTest
             COMMAND php -dextension=$<TARGET_FILE:html2img> ${CMAKE_CURRENT_SOURCE_DIR}/tests/concurrent_cache.php)
    add_test(NAME RelativePathFontTest
             COMMAND php -dextension=$<TARGET_FILE:html2img> ${CMAKE_CURRENT_SOURCE_DIR}/tests/relative_font.php)
endif()
