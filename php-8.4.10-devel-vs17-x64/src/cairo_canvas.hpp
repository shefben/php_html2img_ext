#ifndef HTML2IMG_CAIRO_CANVAS_HPP
#define HTML2IMG_CAIRO_CANVAS_HPP

#include <cairo.h>
#include <string>

class CairoCanvas {
public:
    CairoCanvas(int w, int h, bool has_bg = false, unsigned int bg = 0);
    ~CairoCanvas();

    cairo_surface_t* surface() const { return surface_; }
    cairo_t* cr() const { return cr_; }
    bool export_image(const std::string& path, const std::string& format);
private:
    cairo_surface_t* surface_{nullptr};
    cairo_t* cr_{nullptr};
};

#endif
