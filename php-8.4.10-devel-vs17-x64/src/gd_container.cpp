#define LITEHTML_STD_VARIANT 1
#define LITEHTML_STD_OPTIONAL 1
#define LITEHTML_STD_FILESYSTEM 1

#include "gd_container.hpp"
#include <gd.h>
#include <cstring>
#include <fstream>
#include <filesystem>

GDContainer::GDContainer(GDCanvas& canvas,
                         const std::filesystem::path& base,
                         const std::filesystem::path& font_dir,
                         bool allow_remote)
    : canvas_(canvas), base_path_(base), font_dir_(font_dir), allow_remote_(allow_remote) {}

void GDContainer::register_font(const std::string& family, const std::filesystem::path& path)
{
    font_map_[family] = path;
}

litehtml::uint_ptr GDContainer::create_font(const litehtml::font_description& descr, const litehtml::document*, litehtml::font_metrics* fm)
{
    FT_Face face = nullptr;
    auto it = font_map_.find(descr.family);
    std::filesystem::path path;
    if(it != font_map_.end()) {
        path = it->second;
    } else {
        path = font_dir_ / descr.family;
    }
    if(std::filesystem::exists(path)) {
        face = ft_.load(path);
    }
    if(face) {
        FT_Set_Pixel_Sizes(face, 0, descr.size);
        if(fm) {
            fm->ascent = face->size->metrics.ascender >> 6;
            fm->descent = -face->size->metrics.descender >> 6;
            fm->height = fm->ascent + fm->descent;
        }
    }
    return reinterpret_cast<litehtml::uint_ptr>(face);
}

void GDContainer::delete_font(litehtml::uint_ptr hFont)
{
    // fonts cached; do nothing
}

int GDContainer::text_width(const char* text, litehtml::uint_ptr hFont)
{
    FT_Face face = reinterpret_cast<FT_Face>(hFont);
    if(!face) return strlen(text)*6;
    int pen_x = 0;
    for(const char* p=text; *p; ++p) {
        if(FT_Load_Char(face, *p, FT_LOAD_DEFAULT)) continue;
        pen_x += face->glyph->advance.x >> 6;
    }
    return pen_x;
}

void GDContainer::draw_text(litehtml::uint_ptr, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos)
{
    FT_Face face = reinterpret_cast<FT_Face>(hFont);
    if(!face) return;
    int pen_x = pos.x;
    for(const char* p=text; *p; ++p) {
        if(FT_Load_Char(face, *p, FT_LOAD_RENDER)) continue;
        gdImagePtr bmp = gdImageCreateTrueColor(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        gdImageAlphaBlending(bmp, 0);
        gdImageSaveAlpha(bmp, 1);
        for(int y=0;y<face->glyph->bitmap.rows;y++) {
            for(int x=0;x<face->glyph->bitmap.width;x++) {
                unsigned char a = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
                int c = gdTrueColorAlpha(color.red, color.green, color.blue, 127 - a/2);
                gdImageSetPixel(bmp, x, y, c);
            }
        }
        gdImageCopy(canvas_.img(), bmp, pen_x + face->glyph->bitmap_left, pos.y - face->glyph->bitmap_top,
                    0,0, face->glyph->bitmap.width, face->glyph->bitmap.rows);
        pen_x += face->glyph->advance.x >> 6;
        gdImageDestroy(bmp);
    }
}

int GDContainer::pt_to_px(int pt) const
{
    return pt * 96 / 72;
}

int GDContainer::get_default_font_size() const
{
    return 16;
}

const char* GDContainer::get_default_font_name() const
{
    return "Arial";
}

void GDContainer::draw_solid_fill(litehtml::uint_ptr, const litehtml::background_layer& layer, const litehtml::web_color& color)
{
    gdImageFilledRectangle(canvas_.img(), layer.clip_box.left(), layer.clip_box.top(), layer.clip_box.right(), layer.clip_box.bottom(),
                           gdTrueColor(color.red, color.green, color.blue));
}

void GDContainer::transform_text(litehtml::string& text, litehtml::text_transform tt)
{
    if(tt == litehtml::text_transform_capitalize && !text.empty()) {
        text[0] = toupper(text[0]);
    } else if(tt == litehtml::text_transform_uppercase) {
        for(auto& ch : text) ch = toupper(ch);
    } else if(tt == litehtml::text_transform_lowercase) {
        for(auto& ch : text) ch = tolower(ch);
    }
}

void GDContainer::get_viewport(litehtml::position& viewport) const
{
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = gdImageSX(canvas_.img());
    viewport.height = gdImageSY(canvas_.img());
}

void GDContainer::get_media_features(litehtml::media_features& media) const
{
    media.type = litehtml::media_type_screen;
    media.width = gdImageSX(canvas_.img());
    media.height = gdImageSY(canvas_.img());
    media.color = 8;
}
