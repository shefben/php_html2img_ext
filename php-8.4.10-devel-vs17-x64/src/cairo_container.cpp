#define LITEHTML_STD_VARIANT 1
#define LITEHTML_STD_OPTIONAL 1
#define LITEHTML_STD_FILESYSTEM 1

#include "cairo_container.hpp"
#include <cairo-ft.h>
#include <cstring>
#include <fstream>
#include <filesystem>
#include "path_utils.hpp"

CairoContainer::CairoContainer(CairoCanvas& canvas,
                               const std::filesystem::path& base,
                               const std::filesystem::path& font_dir,
                               bool allow_remote)
    : canvas_(canvas), base_path_(base), font_dir_(font_dir), allow_remote_(allow_remote) {}

void CairoContainer::register_font(const std::string& family, const std::filesystem::path& path)
{
    font_map_[family] = path;
}

litehtml::uint_ptr CairoContainer::create_font(const litehtml::font_description& descr, const litehtml::document*, litehtml::font_metrics* fm)
{
    FT_Face face = nullptr;
    auto it = font_map_.find(descr.family);
    std::filesystem::path path;
    if(it != font_map_.end()) {
        path = it->second;
    } else {
        path = font_dir_ / descr.family;
        if(!std::filesystem::exists(path)) {
            static const char* exts[] = {".ttf", ".otf", ".ttc"};
            for(const char* ext : exts) {
                auto candidate = path;
                candidate += ext;
                if(std::filesystem::exists(candidate)) {
                    path = candidate;
                    break;
                }
            }
        }
    }
    if(std::filesystem::exists(path)) {
        face = ft_.load(path);
    }
    if(!face && descr.family != "Arial") {
        auto fallback = font_dir_ / "Arial.ttf";
        if(std::filesystem::exists(fallback)) {
            face = ft_.load(fallback);
        }
    }
    if(!face) return 0;
    FT_Set_Pixel_Sizes(face, 0, descr.size);
    cairo_font_face_t* cf = cairo_ft_font_face_create_for_ft_face(face, 0);
    cairo_matrix_t font_matrix, ctm;
    cairo_matrix_init_scale(&font_matrix, descr.size, descr.size);
    cairo_matrix_init_identity(&ctm);
    cairo_font_options_t* opts = cairo_font_options_create();
    cairo_scaled_font_t* sf = cairo_scaled_font_create(cf, &font_matrix, &ctm, opts);
    cairo_font_options_destroy(opts);
    cairo_font_face_destroy(cf);
    if(fm) {
        cairo_font_extents_t ext;
        cairo_scaled_font_extents(sf, &ext);
        fm->ascent = ext.ascent;
        fm->descent = ext.descent;
        fm->height = ext.height;
    }
    return reinterpret_cast<litehtml::uint_ptr>(sf);
}

void CairoContainer::delete_font(litehtml::uint_ptr hFont)
{
    if(hFont) {
        cairo_scaled_font_destroy(reinterpret_cast<cairo_scaled_font_t*>(hFont));
    }
}

int CairoContainer::text_width(const char* text, litehtml::uint_ptr hFont)
{
    cairo_scaled_font_t* sf = reinterpret_cast<cairo_scaled_font_t*>(hFont);
    if(!sf) return strlen(text)*6;
    cairo_text_extents_t ext;
    cairo_scaled_font_text_extents(sf, text, &ext);
    return (int)ext.x_advance;
}

void CairoContainer::draw_text(litehtml::uint_ptr, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos)
{
    cairo_scaled_font_t* sf = reinterpret_cast<cairo_scaled_font_t*>(hFont);
    if(!sf) return;
    cairo_t* cr = canvas_.cr();
    cairo_set_scaled_font(cr, sf);
    cairo_set_source_rgba(cr, color.red/255.0, color.green/255.0, color.blue/255.0, (255-color.alpha)/255.0);
    cairo_move_to(cr, pos.x, pos.y);
    cairo_show_text(cr, text);
}

int CairoContainer::pt_to_px(int pt) const
{
    return pt * 96 / 72;
}

int CairoContainer::get_default_font_size() const
{
    return 16;
}

const char* CairoContainer::get_default_font_name() const
{
    return "Arial";
}

void CairoContainer::draw_solid_fill(litehtml::uint_ptr, const litehtml::background_layer& layer, const litehtml::web_color& color)
{
    cairo_t* cr = canvas_.cr();
    cairo_set_source_rgba(cr, color.red/255.0, color.green/255.0, color.blue/255.0, 1.0);
    cairo_rectangle(cr, layer.clip_box.left(), layer.clip_box.top(), layer.clip_box.width, layer.clip_box.height);
    cairo_fill(cr);
}

cairo_surface_t* CairoContainer::load_image_internal(const std::string& src, const std::string& base)
{
    auto it = image_cache_.find(src);
    if(it != image_cache_.end()) return it->second.surf;
    auto path = resolve_path(src, base);
    if(path.empty() || !std::filesystem::exists(path)) return nullptr;
    cairo_surface_t* surf = cairo_image_surface_create_from_png(path.string().c_str());
    if(cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surf);
        surf = nullptr;
    }
    if(surf) {
        image_cache_[src] = {surf, cairo_image_surface_get_width(surf), cairo_image_surface_get_height(surf)};
    }
    return surf;
}

void CairoContainer::load_image(const char* src, const char* baseurl, bool)
{
    load_image_internal(src ? src : "", baseurl ? baseurl : "");
}

void CairoContainer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz)
{
    auto surf = load_image_internal(src ? src : "", baseurl ? baseurl : "");
    if(surf) {
        sz.width = cairo_image_surface_get_width(surf);
        sz.height = cairo_image_surface_get_height(surf);
    } else {
        sz.width = sz.height = 0;
    }
}

void CairoContainer::draw_image(litehtml::uint_ptr, const litehtml::background_layer& layer, const std::string& url, const std::string& base_url)
{
    auto surf = load_image_internal(url, base_url);
    if(!surf) return;
    cairo_t* cr = canvas_.cr();
    cairo_save(cr);
    cairo_rectangle(cr, layer.clip_box.left(), layer.clip_box.top(), layer.clip_box.width, layer.clip_box.height);
    cairo_clip(cr);
    cairo_set_source_surface(cr, surf, layer.clip_box.left(), layer.clip_box.top());
    cairo_paint(cr);
    cairo_restore(cr);
}

void CairoContainer::transform_text(litehtml::string& text, litehtml::text_transform tt)
{
    if(tt == litehtml::text_transform_capitalize && !text.empty()) {
        text[0] = toupper(text[0]);
    } else if(tt == litehtml::text_transform_uppercase) {
        for(auto& ch : text) ch = toupper(ch);
    } else if(tt == litehtml::text_transform_lowercase) {
        for(auto& ch : text) ch = tolower(ch);
    }
}

void CairoContainer::get_viewport(litehtml::position& viewport) const
{
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = cairo_image_surface_get_width(canvas_.surface());
    viewport.height = cairo_image_surface_get_height(canvas_.surface());
}

void CairoContainer::get_media_features(litehtml::media_features& media) const
{
    media.type = litehtml::media_type_screen;
    media.width = cairo_image_surface_get_width(canvas_.surface());
    media.height = cairo_image_surface_get_height(canvas_.surface());
    media.color = 8;
}

std::filesystem::path CairoContainer::resolve_path(const std::string& src, const std::string& base)
{
    std::filesystem::path b = base.empty() ? base_path_ : std::filesystem::path(base);
    return html2img::resolve_asset(src, b, allow_remote_);
}

