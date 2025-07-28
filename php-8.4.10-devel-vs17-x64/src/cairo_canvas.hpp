#pragma once
#include <cairo.h>
#include <string>

class CairoCanvas {
public:
    CairoCanvas(int w, int h, bool opaque_bg = false,
                unsigned int bg = 0);               // bg = 0xAARRGGBB
    ~CairoCanvas();

    cairo_surface_t* surface() { return surf_; }

    /**  Write PNG, GIF or JPG on disk. */
    bool export_image(const std::string& path,
                      const std::string& format = "png");

private:
    cairo_surface_t* surf_{nullptr};
    cairo_t* cr_{nullptr};
};
