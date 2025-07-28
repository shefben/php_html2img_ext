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
#include "php_html2img_arginfo.h"

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

static std::filesystem::path script_base()
{
    zend_string* exec = zend_get_executed_filename_ex();
    std::filesystem::path p = exec ? html2img::from_file_uri(ZSTR_VAL(exec)) : std::filesystem::current_path();
    if(!p.is_absolute())
        p = std::filesystem::absolute(p);
    return p.parent_path();
}

PHP_FUNCTION(html_file_to_image)
{
    zend_string *html_path, *out_path, *format;

    ZEND_PARSE_PARAMETERS_START(3,3)
        Z_PARAM_STR(html_path)
        Z_PARAM_STR(out_path)
        Z_PARAM_STR(format)
    ZEND_PARSE_PARAMETERS_END();

    std::filesystem::path base = script_base();
    std::filesystem::path html_file = html2img::resolve_asset(ZSTR_VAL(html_path), base, false);
    std::filesystem::path out_file  = html2img::resolve_asset(ZSTR_VAL(out_path), base, false);
    std::string fmt = ZSTR_VAL(format);

    std::ifstream in(html_file);
    if(!in)
    {
        php_error_docref(nullptr, E_WARNING, "Cannot open HTML file '%s'", ZSTR_VAL(html_path));
        RETURN_FALSE;
    }
    std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    static bool init = false;
    if(!init) { Rml::Initialise(); init = true; }

    CairoCanvas canvas(800,600,true,0xFFFFFF);
    CairoRenderInterface renderer(canvas);
    Rml::SetRenderInterface(&renderer);
    Rml::Context* context = Rml::CreateContext("php", Rml::Vector2i(800,600));
    Rml::ElementDocument* doc = context->LoadDocumentFromMemory(html);
    if(!doc) {
        php_error_docref(nullptr, E_WARNING, "Failed to parse HTML");
        RETURN_FALSE;
    }
    doc->Show();
    context->Update();
    context->Render();
    context->UnloadDocument(doc);
    Rml::RemoveRenderInterface();

    canvas.export_image(out_file.string(), fmt);

    std::ifstream img(out_file, std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(img)), std::istreambuf_iterator<char>());
    RETVAL_STRINGL(data.c_str(), data.size());
}
