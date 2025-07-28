#include "cairo_container.hpp"
#include <cstring>
#include <cctype>
#include <cairo.h>
#include <iostream>
#include <unordered_map>
#include <fstream>
#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

// For FreeType font loading
#include <cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// For image loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std::string_literals;

/* Global FreeType library instance */
static FT_Library ft_library = nullptr;
static std::unordered_map<std::string, cairo_font_face_t*> font_cache;
static std::unordered_map<std::string, cairo_surface_t*> image_cache;

/* -------------------------------------------------------------- */

CairoContainer::CairoContainer(CairoCanvas& c,
                               const std::filesystem::path& base,
                               const std::filesystem::path& fonts,
                               bool allow_remote)
  : canvas_(c), base_path_(base),
    font_dir_(fonts), allow_remote_(allow_remote)
{
    if (!ft_library) {
        FT_Error error = FT_Init_FreeType(&ft_library);
        if (error) {
            std::cout << "Failed to initialize FreeType library\n";
        } else {
            std::cout << "FreeType library initialized successfully\n";
        }
    }
    std::cout << "CairoContainer created with font_dir: " << fonts
              << "  base_path: " << base << "\n";
}

/* helper – search for font files with various extensions */
static std::filesystem::path find_font_file(const std::filesystem::path& base_dir,
                                            const std::string& family)
{
    static const char* exts[] = { ".otf", ".ttf", ".ttc" };
    for (const char* ext : exts) {
        auto p = base_dir / (family + ext);
        if (std::filesystem::exists(p)) return p;
    }
    std::string lower = family;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (auto& ent : std::filesystem::directory_iterator(base_dir)) {
        if (!ent.is_regular_file()) continue;
        auto fn = ent.path().filename().string();
        auto lfn = fn; std::transform(lfn.begin(), lfn.end(), lfn.begin(), ::tolower);
        if (lfn.find(lower) != std::string::npos) {
            for (const char* ext : exts) {
                if (lfn.size() >= strlen(ext) && lfn.rfind(ext) == lfn.size() - strlen(ext))
                    return ent.path();
            }
        }
    }
    return {};
}

cairo_font_face_t* load_font_from_file(const std::filesystem::path& font_path) {
    if (!ft_library) return nullptr;
    auto key = font_path.string();
    if (auto it = font_cache.find(key); it != font_cache.end()) return it->second;

    FT_Face ft_face;
    if (FT_New_Face(ft_library, key.c_str(), 0, &ft_face)) {
        std::cout << "Failed to load font: " << key << "\n";
        return nullptr;
    }
    auto face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    if (cairo_font_face_status(face) != CAIRO_STATUS_SUCCESS) {
        cairo_font_face_destroy(face);
        FT_Done_Face(ft_face);
        std::cout << "Failed to create Cairo face: " << key << "\n";
        return nullptr;
    }
    font_cache[key] = face;
    return face;
}

litehtml::uint_ptr CairoContainer::create_font(const litehtml::font_description& d,
                                               const litehtml::document*,
                                               litehtml::font_metrics* fm)
{

    ToyFont* F = new ToyFont;
    F->size_px = d.size ? d.size : 16;

    std::string family = d.family.empty() ? "Arial" : d.family;
    auto path = find_font_file(base_path_, family);
    if (path.empty()) path = find_font_file(font_dir_, family);
    if (!path.empty()) {
        F->face = load_font_from_file(path);
    }
    if (!F->face) {
        cairo_font_weight_t w = d.weight >= 700
            ? CAIRO_FONT_WEIGHT_BOLD
            : CAIRO_FONT_WEIGHT_NORMAL;
        F->face = cairo_toy_font_face_create(family.c_str(),
                                             CAIRO_FONT_SLANT_NORMAL, w);
    }

    if (!F->face) {
        delete F;
        return 0;
    }

    if (fm) {
        auto surf = cairo_image_surface_create(CAIRO_FORMAT_A8,1,1);
        auto cr   = cairo_create(surf);
        cairo_set_font_face(cr, F->face);
        cairo_set_font_size(cr, F->size_px);
        cairo_font_extents_t ext;
        cairo_font_extents(cr, &ext);
        fm->height   = int(ext.height);
        fm->ascent   = int(ext.ascent);
        fm->descent  = int(ext.descent);
        fm->x_height = int(ext.height * 0.5);
        cairo_destroy(cr);
        cairo_surface_destroy(surf);
    }

    pool_.push_back(F);
    return reinterpret_cast<litehtml::uint_ptr>(F);
}

void CairoContainer::delete_font(litehtml::uint_ptr f) {
    auto F = reinterpret_cast<decltype(pool_)::value_type>(f);
    delete F;
}

int CairoContainer::text_width(const char* txt, litehtml::uint_ptr f) {
    if (!txt || !f) return 0;
    auto F = reinterpret_cast<decltype(pool_)::value_type>(f);
    auto surf = cairo_image_surface_create(CAIRO_FORMAT_A8,1,1);
    auto cr   = cairo_create(surf);
    cairo_set_font_face(cr, F->face);
    cairo_set_font_size(cr, F->size_px);
    cairo_text_extents_t ext;
    cairo_text_extents(cr, txt, &ext);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    return int(ext.x_advance + 0.5);
}

void CairoContainer::draw_text(litehtml::uint_ptr, const char* txt,
                               litehtml::uint_ptr fnt,
                               litehtml::web_color col,
                               const litehtml::position& pos)
{
    if (!txt || !fnt || !*txt) return;
    auto F = reinterpret_cast<decltype(pool_)::value_type>(fnt);
    cairo_t* cr = cairo_create(canvas_.surface());
    cairo_set_font_face(cr, F->face);
    cairo_set_font_size(cr, F->size_px);

    cairo_font_extents_t fe;
    cairo_font_extents(cr, &fe);
    double x = pos.x;
    double y = pos.y + fe.ascent;

    // ** SINGLE DRAW ** — LiteHTML will call this for each CSS text-shadow
    cairo_set_source_rgba(cr,
        col.red   / 255.0,
        col.green / 255.0,
        col.blue  / 255.0,
        col.alpha / 255.0);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, txt);
    cairo_destroy(cr);
}

void CairoContainer::draw_solid_fill(litehtml::uint_ptr,
                                     const litehtml::background_layer& lay,
                                     const litehtml::web_color& c)
{
    cairo_t* cr = cairo_create(canvas_.surface());
    cairo_set_source_rgba(cr,
         c.red/255.0, c.green/255.0, c.blue/255.0, c.alpha/255.0);
    auto& b = lay.clip_box;
    cairo_rectangle(cr, b.left(), b.top(), b.width, b.height);
    cairo_fill(cr);
    cairo_destroy(cr);
}

int CairoContainer::pt_to_px(int pt) const {
    return int((pt * 96.0/72.0) + 0.5);
}

void CairoContainer::transform_text(litehtml::string& t,
                                    litehtml::text_transform tt)
{
    switch(tt) {
        case litehtml::text_transform_uppercase:
            for (auto& ch : t) ch = char(std::toupper(ch)); break;
        case litehtml::text_transform_lowercase:
            for (auto& ch : t) ch = char(std::tolower(ch)); break;
        case litehtml::text_transform_capitalize: {
            bool cap = true;
            for (auto& ch : t) {
                if (std::isspace(ch)) cap = true;
                else if (cap) { ch = char(std::toupper(ch)); cap = false; }
            }
            break;
        }
        default: break;
    }
}

void CairoContainer::get_viewport(litehtml::position& v) const {
    auto s = canvas_.surface();
    v.x = v.y = 0;
    v.width  = cairo_image_surface_get_width(s);
    v.height = cairo_image_surface_get_height(s);
}

void CairoContainer::get_media_features(litehtml::media_features& m) const {
    auto s = canvas_.surface();
    m.type        = litehtml::media_type_screen;
    m.width       = cairo_image_surface_get_width(s);
    m.height      = cairo_image_surface_get_height(s);
    m.color       = 8;
    m.monochrome  = 0;
    m.color_index = 0;
    m.resolution  = 96;
}

void CairoContainer::draw_image(litehtml::uint_ptr,
                                const litehtml::background_layer& layer,
                                const std::string& url,
                                const std::string& base_url)
{
    cairo_surface_t* img = nullptr;
    auto it = image_cache.find(url);
    if (it != image_cache.end()) img = it->second;
    else {
        std::string path = (url.find("://")==std::string::npos && url[0]!='/')
                         ? (base_path_/url).string() : url;
        it = image_cache.find(path);
        if (it != image_cache.end()) img = it->second;
        else load_image(url.c_str(), base_url.c_str(), false),
             img = image_cache[url];
    }
    if (!img) return;

    cairo_t* cr = cairo_create(canvas_.surface());
    auto& b = layer.clip_box;
    cairo_save(cr);
    cairo_translate(cr, b.left(), b.top());

    int iw = cairo_image_surface_get_width(img);
    int ih = cairo_image_surface_get_height(img);
    if (iw != b.width || ih != b.height) {
        cairo_scale(cr,
            double(b.width)/iw,
            double(b.height)/ih);
    }

    cairo_set_source_surface(cr, img, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    cairo_destroy(cr);
}

void CairoContainer::load_image(const char* src,
                                const char* baseurl,
                                bool)
{
    if (!src) return;
    std::string path = (strstr(src, "://")==nullptr && src[0]!='/')
                     ? (base_path_/src).string()
                     : src;
    if (image_cache.count(src) || image_cache.count(path)) return;

    int w,h,chn;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &chn, 4);
    if (!data) return;
    for(int i=0;i<w*h;i++){
        auto p = data + i*4;
        std::swap(p[0], p[2]);
    }
    auto surf = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, w,h, w*4);
    image_cache[src]  = surf;
    if (path != src) image_cache[path] = surf;
}

void CairoContainer::get_image_size(const char* src,
                                    const char* baseurl,
                                    litehtml::size& sz)
{
    if (!src) return;
    if (auto it = image_cache.find(src); it!=image_cache.end()){
        sz.width  = cairo_image_surface_get_width(it->second);
        sz.height = cairo_image_surface_get_height(it->second);
    } else {
        int w,h,chn;
        if (stbi_info(src, &w,&h,&chn)){
            sz.width=w; sz.height=h;
        } else sz.width=sz.height=0;
    }
}
