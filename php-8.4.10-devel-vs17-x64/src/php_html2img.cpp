#include <filesystem>
#include <fstream>
#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>
#include <Zend/zend_execute.h>
#include <RmlUi/Core.h>
#include "path_utils.hpp"
#include "cairo_canvas.hpp"
#include "rml_cairo_renderer.hpp"
#include "file_interface.hpp"
#include "php_html2img_arginfo.h"

/* -------------------------------------------------------------------------
 * Helper routines
 * -------------------------------------------------------------------------*/

/** Return the directory of the executing PHP script. */
static std::filesystem::path script_base()
{
    zend_string *exec = zend_get_executed_filename_ex();
    std::filesystem::path p = exec ? html2img::from_file_uri(ZSTR_VAL(exec))
                                   : std::filesystem::current_path();
    if (!p.is_absolute()) {
        p = std::filesystem::absolute(p);
    }
    return p.parent_path();
}

/** Read the entire file at `path` into `out`. */
static bool read_file(const std::filesystem::path &path, std::string &out)
{
    std::ifstream fp(path, std::ios::binary);
    if (!fp) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(fp), std::istreambuf_iterator<char>());
    return true;
}

/** Render HTML into an image using RmlUi and Cairo. */
static bool render_html_to_image(const std::string &html,
                                 const std::filesystem::path &out_path,
                                 const std::string &format,
                                 std::string &result)
{
    static bool rml_inited = false;
    if (!rml_inited) {
        Rml::Initialise();
        rml_inited = true;
    }

    PHPFileInterface files(out_path.parent_path());
    Rml::SetFileInterface(&files);

    const int width = 800;
    const int height = 600;
    CairoCanvas canvas(width, height, true, 0xFFFFFF);
    CairoRenderInterface renderer(canvas);
    Rml::SetRenderInterface(&renderer);

    Rml::Context *ctx = Rml::CreateContext("php", Rml::Vector2i(width, height));
    Rml::ElementDocument *doc = ctx->LoadDocumentFromMemory(html);
    if (!doc) {
        Rml::RemoveRenderInterface();
        return false;
    }
    doc->Show();
    ctx->Update();
    ctx->Render();
    ctx->UnloadDocument(doc);
    Rml::RemoveRenderInterface();
    Rml::SetFileInterface(nullptr);

    if (!canvas.export_image(out_path.string(), format)) {
        return false;
    }

    return read_file(out_path, result);
}

/* -------------------------------------------------------------------------
 * PHP Extension glue
 * -------------------------------------------------------------------------*/

extern "C" {
static PHP_FUNCTION(html_file_to_image);
ZEND_TSRMLS_CACHE_DEFINE();
}

static zend_function_entry html2img_functions[] = {
    PHP_FE(html_file_to_image, arginfo_html_file_to_image)
    PHP_FE_END
};

PHP_MINIT_FUNCTION(html2img)
{
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(html2img)
{
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

/* -------------------------------------------------------------------------
 * User facing function
 * -------------------------------------------------------------------------*/

PHP_FUNCTION(html_file_to_image)
{
    zend_string *html_path, *out_path, *format;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(html_path)
        Z_PARAM_STR(out_path)
        Z_PARAM_STR(format)
    ZEND_PARSE_PARAMETERS_END();

    std::filesystem::path base = script_base();
    std::filesystem::path html_file = html2img::resolve_asset(ZSTR_VAL(html_path), base, false);
    std::filesystem::path output_file = html2img::resolve_asset(ZSTR_VAL(out_path), base, false);
    std::string fmt = ZSTR_VAL(format);

    std::string html;
    if (!read_file(html_file, html)) {
        php_error_docref(nullptr, E_WARNING, "Cannot open HTML file '%s'", ZSTR_VAL(html_path));
        RETURN_FALSE;
    }

    std::string img_data;
    if (!render_html_to_image(html, output_file, fmt, img_data)) {
        php_error_docref(nullptr, E_WARNING, "Failed to render HTML");
        RETURN_FALSE;
    }

    RETVAL_STRINGL(img_data.c_str(), img_data.size());
}

