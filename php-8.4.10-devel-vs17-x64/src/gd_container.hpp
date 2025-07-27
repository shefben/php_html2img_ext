#ifndef HTML2IMG_GD_CONTAINER_HPP
#define HTML2IMG_GD_CONTAINER_HPP
#define LITEHTML_STD_VARIANT 1
#define LITEHTML_STD_OPTIONAL 1
#define LITEHTML_STD_FILESYSTEM 1
#include <litehtml.h>
#include "gd_canvas.hpp"
#include "ft_cache.hpp"
#include "path_utils.hpp"
#include <filesystem>
#include <unordered_map>

class GDContainer : public litehtml::document_container
{
public:
    GDContainer(GDCanvas& canvas,
                const std::filesystem::path& base,
                const std::filesystem::path& font_dir,
                bool allow_remote);
    void register_font(const std::string& family, const std::filesystem::path& path);
    ~GDContainer() override = default;

    litehtml::uint_ptr create_font(const litehtml::font_description& descr, const litehtml::document* doc, litehtml::font_metrics* fm) override;
    void delete_font(litehtml::uint_ptr hFont) override;
    int text_width(const char* text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;
    int pt_to_px(int pt) const override;
    int get_default_font_size() const override;
    const char* get_default_font_name() const override;
    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override {}
    void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;
    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const std::string& url, const std::string& base_url) override;
    void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::web_color& color) override;
    void draw_linear_gradient(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::background_layer::linear_gradient&) override {}
    void draw_radial_gradient(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::background_layer::radial_gradient&) override {}
    void draw_conic_gradient(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::background_layer::conic_gradient&) override {}
    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override {}

    void set_caption(const char*) override {}
    void set_base_url(const char* base_url) override { base_path_ = base_url ? base_url : ""; }
    void link(const std::shared_ptr<litehtml::document>&, const litehtml::element::ptr&) override {}
    void on_anchor_click(const char*, const litehtml::element::ptr&) override {}
    void on_mouse_event(const litehtml::element::ptr&, litehtml::mouse_event) override {}
    void set_cursor(const char*) override {}
    void transform_text(litehtml::string& text, litehtml::text_transform tt) override;
    void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override {}
    void set_clip(const litehtml::position&, const litehtml::border_radiuses&) override {}
    void del_clip() override {}
    void get_viewport(litehtml::position& viewport) const override;
    litehtml::element::ptr create_element(const char*, const litehtml::string_map&, const std::shared_ptr<litehtml::document>&) override { return nullptr; }
    void get_media_features(litehtml::media_features& media) const override;
    void get_language(litehtml::string& language, litehtml::string& culture) const override { language="en"; culture=""; }

private:
    GDCanvas& canvas_;
    std::filesystem::path base_path_;
    std::filesystem::path font_dir_;
    bool allow_remote_{};
    FtCache ft_;
    std::unordered_map<std::string, std::filesystem::path> font_map_;
    struct ImageData { gdImagePtr img{nullptr}; int w{}; int h{}; };
    std::unordered_map<std::string, ImageData> image_cache_;

    gdImagePtr load_image_internal(const std::string& src, const std::string& base);
    std::filesystem::path resolve_path(const std::string& src, const std::string& base);
};

#endif
