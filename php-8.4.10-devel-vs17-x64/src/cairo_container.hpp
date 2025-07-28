#pragma once
#define LITEHTML_STD_VARIANT    1
#define LITEHTML_STD_OPTIONAL   1
#define LITEHTML_STD_FILESYSTEM 1
#include <litehtml.h>
#include <filesystem>
#include <vector>
#include "cairo_canvas.hpp"

/* Text shadow structure */
struct TextShadow {
    double offset_x;
    double offset_y;
    double blur_radius;
    double red, green, blue, alpha;
    
    TextShadow() : offset_x(0), offset_y(0), blur_radius(0), red(0), green(0), blue(0), alpha(0) {}
};

/* one tiny struct = font handle */
struct ToyFont {
    cairo_font_face_t* face;
    double             size_px;
};

class CairoContainer : public litehtml::document_container {
public:
    CairoContainer(CairoCanvas& canvas,
                   const std::filesystem::path& base_dir,
                   const std::filesystem::path& user_fonts,
                   bool allow_remote);

    /* ---- mandatory overrides ---------------------------------- */
    litehtml::uint_ptr create_font(const litehtml::font_description&,
                                   const litehtml::document*,
                                   litehtml::font_metrics*) override;
    void               delete_font(litehtml::uint_ptr) override;

    int  text_width(const char*, litehtml::uint_ptr) override;
    void draw_text(litehtml::uint_ptr, const char*, litehtml::uint_ptr,
                   litehtml::web_color, const litehtml::position&) override;

    void draw_solid_fill(litehtml::uint_ptr,
                         const litehtml::background_layer&,
                         const litehtml::web_color&) override;

    /* Image handling functions - properly declared */
    void draw_image(litehtml::uint_ptr, const litehtml::background_layer&,
                    const std::string&, const std::string&) override;
    void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;

    /* the rest that LiteHTML insists on but we ignore */
    void draw_list_marker(litehtml::uint_ptr,const litehtml::list_marker&) override {}
    void draw_linear_gradient(litehtml::uint_ptr,const litehtml::background_layer&,
                    const litehtml::background_layer::linear_gradient&) override {}
    void draw_radial_gradient(litehtml::uint_ptr,const litehtml::background_layer&,
                    const litehtml::background_layer::radial_gradient&) override {}
    void draw_conic_gradient(litehtml::uint_ptr,const litehtml::background_layer&,
                    const litehtml::background_layer::conic_gradient&) override {}
    void draw_borders(litehtml::uint_ptr,const litehtml::borders&,
                    const litehtml::position&,bool) override {}
    void link(const std::shared_ptr<litehtml::document>&,
              const litehtml::element::ptr&) override {}

    void set_caption(const char*) override {}
    void set_base_url(const char*) override {}
    void on_anchor_click(const char*,const litehtml::element::ptr&) override {}
    void on_mouse_event(const litehtml::element::ptr&,litehtml::mouse_event) override {}
    void set_cursor(const char*) override {}
    void import_css(std::string&,const std::string&,std::string&) override {}
    void set_clip(const litehtml::position&,const litehtml::border_radiuses&) override {}
    void del_clip() override {}
    void get_language(std::string& l,std::string& c) const override {l.clear();c.clear();}
    std::shared_ptr<litehtml::element>
      create_element(const char*,const litehtml::string_map&,
                     const std::shared_ptr<litehtml::document>&) override {return nullptr;}

    /* not present in your header → remove 'override' */
    litehtml::uint_ptr get_image(const char*, const char*, bool) { return 0; }
    int         get_default_font_size() const override { return 16; }
    const char* get_default_font_name() const override { return "sans"; }

    /* viewport + media */
    int  pt_to_px(int) const override;
    void transform_text(litehtml::string&,litehtml::text_transform) override;
    void get_viewport(litehtml::position&) const override;
    void get_media_features(litehtml::media_features&) const override;

private:
    CairoCanvas&                    canvas_;
    std::filesystem::path           base_path_;
    std::filesystem::path           font_dir_;
    bool                            allow_remote_;
    std::vector<ToyFont*>           pool_;
    
    /* Helper functions for text shadow parsing */
    std::vector<TextShadow> parse_text_shadow(const std::string& shadow_str);
    void parse_css_color(const std::string& color_str, double& r, double& g, double& b, double& a);
    std::string get_element_text_shadow(litehtml::uint_ptr hdc);
};