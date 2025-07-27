#include "gd_canvas.hpp"
#include <stdexcept>

GDCanvas::GDCanvas(int w, int h, bool has_bg, unsigned int bg)
{
    img_ = gdImageCreateTrueColor(w, h);
    gdImageAlphaBlending(img_, 0);
    gdImageSaveAlpha(img_, 1);
    if(has_bg) {
        gdImageFilledRectangle(img_, 0,0,w,h, bg);
    } else {
        gdImageFilledRectangle(img_, 0,0,w,h, gdTrueColorAlpha(0,0,0,127));
    }
}

GDCanvas::~GDCanvas()
{
    if(img_) gdImageDestroy(img_);
}

bool GDCanvas::export_image(const std::string &path, const std::string &format)
{
    FILE* f = fopen(path.c_str(), "wb");
    if(!f) return false;
    if(format == "png") {
        gdImagePng(img_, f);
    } else if(format == "gif") {
        gdImageGif(img_, f);
    } else if(format == "jpg" || format == "jpeg") {
        gdImageJpeg(img_, f, 90);
    } else {
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}
