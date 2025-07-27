#include "cairo_canvas.hpp"
#include <gif_lib.h>
#include <jpeglib.h>
#include <vector>
#include <cstring>

CairoCanvas::CairoCanvas(int w, int h, bool has_bg, unsigned int bg)
{
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cr_ = cairo_create(surface_);
    double a = has_bg ? 1.0 : 0.0;
    cairo_set_source_rgba(cr_,
                          ((bg >> 16) & 0xFF) / 255.0,
                          ((bg >> 8) & 0xFF) / 255.0,
                          (bg & 0xFF) / 255.0,
                          a);
    cairo_paint(cr_);
}

CairoCanvas::~CairoCanvas()
{
    if(cr_) cairo_destroy(cr_);
    if(surface_) cairo_surface_destroy(surface_);
}

bool CairoCanvas::export_image(const std::string& path, const std::string& format)
{
    if(format == "png") {
        return cairo_surface_write_to_png(surface_, path.c_str()) == CAIRO_STATUS_SUCCESS;
    } else if(format == "jpg" || format == "jpeg") {
        int width = cairo_image_surface_get_width(surface_);
        int height = cairo_image_surface_get_height(surface_);
        unsigned char* data = cairo_image_surface_get_data(surface_);
        FILE* f = fopen(path.c_str(), "wb");
        if(!f) return false;
        jpeg_compress_struct cinfo{};
        jpeg_error_mgr jerr{};
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, f);
        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 90, TRUE);
        jpeg_start_compress(&cinfo, TRUE);
        std::vector<unsigned char> row(width*3);
        while(cinfo.next_scanline < cinfo.image_height) {
            unsigned char* src = data + cinfo.next_scanline * cairo_image_surface_get_stride(surface_);
            for(int x=0;x<width;x++) {
                row[x*3+0] = src[x*4+2];
                row[x*3+1] = src[x*4+1];
                row[x*3+2] = src[x*4+0];
            }
            unsigned char* rowptr = row.data();
            jpeg_write_scanlines(&cinfo, &rowptr, 1);
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(f);
        return true;
    } else if(format == "gif") {
        int width = cairo_image_surface_get_width(surface_);
        int height = cairo_image_surface_get_height(surface_);
        unsigned char* data = cairo_image_surface_get_data(surface_);
        int err = 0;
        GifFileType* gif = EGifOpenFileName(path.c_str(), false, &err);
        if(!gif) return false;
        EGifPutScreenDesc(gif, width, height, 8, 0, nullptr);
        std::vector<GifByteType> img(width*height*4);
        for(int y=0;y<height;y++) {
            unsigned char* src = data + y*cairo_image_surface_get_stride(surface_);
            for(int x=0;x<width;x++) {
                img[(y*width+x)*4+0] = src[x*4+2];
                img[(y*width+x)*4+1] = src[x*4+1];
                img[(y*width+x)*4+2] = src[x*4+0];
                img[(y*width+x)*4+3] = 255 - src[x*4+3];
            }
        }
        EGifPutImageDesc(gif, 0,0,width,height,false,nullptr);
        for(int y=0;y<height;y++) {
            GifByteType* row = &img[y*width*4];
            if(EGifPutLine(gif, row, width*4) == GIF_ERROR) { err=1; break; }
        }
        EGifCloseFile(gif, &err);
        return err==0;
    }
    return false;
}

