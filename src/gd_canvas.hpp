#ifndef HTML2IMG_GD_CANVAS_HPP
#define HTML2IMG_GD_CANVAS_HPP

#include <gd.h>
#include <string>
#include <vector>

class GDCanvas {
public:
    GDCanvas(int w, int h, bool has_bg=false, unsigned int bg=0);
    ~GDCanvas();

    gdImagePtr img() const { return img_; }
    bool export_image(const std::string &path, const std::string &format);
private:
    gdImagePtr img_{};
};

#endif
