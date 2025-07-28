#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <canvas_ity.hpp>
#include <vector>
#include "cairo_canvas.hpp"
#include <stdexcept>
#include <stb_image_write.h>          // tiny header‑only encoders

CairoCanvas::CairoCanvas(int w,int h,bool opaque,unsigned int bg)
{
    surf_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    if(cairo_surface_status(surf_)!=CAIRO_STATUS_SUCCESS)
        throw std::runtime_error("cairo surface");

    cr_   = cairo_create(surf_);
    cairo_set_source_rgba(cr_,
        ((bg>>16)&0xFF)/255.0,
        ((bg>> 8)&0xFF)/255.0,
        ((bg    )&0xFF)/255.0,
        opaque ? 1.0 : ((bg>>24)&0xFF)/255.0);
    cairo_paint(cr_);
}

CairoCanvas::~CairoCanvas()
{
    cairo_destroy(cr_);
    cairo_surface_destroy(surf_);
}

bool CairoCanvas::export_image(const std::string& path,
                               const std::string& fmt)
{
    if(fmt=="png")
        return cairo_surface_write_to_png(surf_, path.c_str())==CAIRO_STATUS_SUCCESS;

    /* For GIF/JPEG we run through stb_image_write on the raw buffer.       */
    unsigned char* buf = cairo_image_surface_get_data(surf_);
    int w = cairo_image_surface_get_width(surf_);
    int h = cairo_image_surface_get_height(surf_);
    int stride = cairo_image_surface_get_stride(surf_);

    if(fmt=="jpg"||fmt=="jpeg")
        return stbi_write_jpg(path.c_str(),w,h,4,buf,90);

    //if(fmt=="gif")
       // return stbi_write_gif(path.c_str(), w, h, 4, buf, stride); // new API in ≥1.17
    return false;
}

