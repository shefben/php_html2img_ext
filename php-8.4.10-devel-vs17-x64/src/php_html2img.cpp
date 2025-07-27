#define LITEHTML_STD_VARIANT 1
#define LITEHTML_STD_OPTIONAL 1
#define LITEHTML_STD_FILESYSTEM 1
#include <variant>
#include <optional>
#include <filesystem>
#include <regex>
#include <litehtml.h>   // or whatever header you use next

#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>
#include <chrono>
#include "gd_canvas.hpp"
#include "gd_container.hpp"
#include "cache.hpp"
#include "php_html2img_arginfo.h"   /* add after other #includes */


extern "C" {
    static PHP_FUNCTION(html_css_to_image);
    /* Required when building ZTS + static‑TLS extensions */
    ZEND_TSRMLS_CACHE_DEFINE();
}
ZEND_BEGIN_MODULE_GLOBALS(html2img)
    char *cache_dir;
    zend_long ttl;
    char *font_path;
    zend_bool allow_remote;
ZEND_END_MODULE_GLOBALS(html2img)

ZEND_DECLARE_MODULE_GLOBALS(html2img)
#define HTML2IMG_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(html2img, v)

static void php_html2img_init_globals(zend_html2img_globals *g)
{
    g->cache_dir = nullptr;
    g->ttl = 0;
    g->font_path = nullptr;
    g->allow_remote = 0;
}

zend_function_entry html2img_functions[] = {
    /* replace ‘nullptr’ with the arginfo symbol */
    PHP_FE(html_css_to_image, arginfo_html_css_to_image)
    PHP_FE_END
};
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("html2img.cache_dir", "", PHP_INI_ALL, OnUpdateString, cache_dir, zend_html2img_globals, html2img_globals)
    STD_PHP_INI_ENTRY("html2img.ttl", "0", PHP_INI_ALL, OnUpdateLong, ttl, zend_html2img_globals, html2img_globals)
    STD_PHP_INI_ENTRY("html2img.font_path", "./", PHP_INI_ALL, OnUpdateString, font_path, zend_html2img_globals, html2img_globals)
    STD_PHP_INI_ENTRY("html2img.allow_remote", "0", PHP_INI_ALL, OnUpdateBool, allow_remote, zend_html2img_globals, html2img_globals)
PHP_INI_END()

PHP_MINIT_FUNCTION(html2img)
{
    ZEND_INIT_MODULE_GLOBALS(html2img, php_html2img_init_globals, nullptr);
    REGISTER_INI_ENTRIES();
    if(!HTML2IMG_G(cache_dir) || HTML2IMG_G(cache_dir)[0] == '\0') {
        std::string d = default_cache_dir().string();
        HTML2IMG_G(cache_dir) = strdup(d.c_str());
    }
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(html2img)
{
    UNREGISTER_INI_ENTRIES();
    if(HTML2IMG_G(cache_dir)) free(HTML2IMG_G(cache_dir));
    return SUCCESS;
}

PHP_MINFO_FUNCTION(html2img)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "html2img support", "enabled");
    php_info_print_table_end();
}

zend_module_entry html2img_module_entry = {
    STANDARD_MODULE_HEADER,
    "html2img",
    html2img_functions,
    PHP_MINIT(html2img),
    PHP_MSHUTDOWN(html2img),
    nullptr,
    nullptr,
    PHP_MINFO(html2img),
    PHP_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_HTML2IMG
extern "C" ZEND_GET_MODULE(html2img)
#endif

PHP_FUNCTION(html_css_to_image)
{
    zend_string *html;
    zend_string *format = nullptr;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(html)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(format)
    ZEND_PARSE_PARAMETERS_END();

    std::string fmt = format ? ZSTR_VAL(format) : "png";
    std::string html_str(ZSTR_VAL(html), ZSTR_LEN(html));

    std::string key = cache_key(html_str, fmt);
    std::filesystem::path dir = HTML2IMG_G(cache_dir);
    ensure_directory(dir);
    std::filesystem::path file = dir / (key + "." + fmt);

    auto now = std::chrono::system_clock::now();
    if(std::filesystem::exists(file)) {
        if(HTML2IMG_G(ttl) == 0) {
            RETURN_STRING(file.string().c_str());
        } else {
            auto ft = std::filesystem::last_write_time(file);
            auto exp = ft + std::chrono::seconds(HTML2IMG_G(ttl));
            if(exp > std::filesystem::file_time_type::clock::now()) {
                RETURN_STRING(file.string().c_str());
            }
        }
    }

    FileLock lock(dir / (key + ".lock"));
    if(std::filesystem::exists(file)) {
        RETURN_STRING(file.string().c_str());
    }

    GDCanvas dummy(1,1,false);
    std::filesystem::path cwd = std::filesystem::current_path();
    GDContainer cont(dummy, cwd, HTML2IMG_G(font_path), HTML2IMG_G(allow_remote));

    static const std::regex font_face_re("@font-face\\s*\\{[^}]*font-family:\\s*['\"]?([^;\"']+)['\"]?;[^}]*src:\\s*url\\(['\"]?([^\"')]+)['\"]?\)", std::regex::icase);
    for(std::sregex_iterator it(html_str.begin(), html_str.end(), font_face_re), end; it!=end; ++it) {
        std::string fam = (*it)[1].str();
        std::string url = (*it)[2].str();
        if(url.rfind("file://",0)==0) url = url.substr(7);
        cont.register_font(fam, url);
    }

    auto doc = litehtml::document::createFromString(html_str.c_str(), &cont);
    doc->render(800);
    int w = doc->width();
    int h = doc->height();

    GDCanvas canvas(w? w:1, h? h:1, false);
    GDContainer cont2(canvas, cwd, HTML2IMG_G(font_path), HTML2IMG_G(allow_remote));
    for(std::sregex_iterator it(html_str.begin(), html_str.end(), font_face_re), end; it!=end; ++it) {
        std::string fam = (*it)[1].str();
        std::string url = (*it)[2].str();
        if(url.rfind("file://",0)==0) url = url.substr(7);
        cont2.register_font(fam, url);
    }
    doc = litehtml::document::createFromString(html_str.c_str(), &cont2);
    doc->render(w);
    litehtml::position clip(0,0,w,h);
    doc->draw(0,0,0,&clip);
    canvas.export_image(file.string(), fmt);

    RETURN_STRING(file.string().c_str());
}
