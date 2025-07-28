#include "rml_cairo_renderer.hpp"
#include <gif_lib.h>
#include <jpeglib.h>
#include <cstring>

CairoRenderInterface::CairoRenderInterface(CairoCanvas& canvas)
    : canvas_(canvas) {}

CairoRenderInterface::~CairoRenderInterface()
{
    for(auto g : geometries_) delete g;
    for(auto& kv : textures_) {
        if(kv.second.surf) cairo_surface_destroy(kv.second.surf);
    }
}

cairo_pattern_t* CairoRenderInterface::make_pattern(Rml::TextureHandle tex)
{
    auto it = textures_.find(tex);
    if(it == textures_.end() || !it->second.surf) return nullptr;
    return cairo_pattern_create_for_surface(it->second.surf);
}

void CairoRenderInterface::RenderGeometry(Rml::Vertex* vertices, int num_vertices,
                                          int* indices, int num_indices,
                                          Rml::TextureHandle texture,
                                          const Rml::Vector2f& translation)
{
    auto handle = CompileGeometry(vertices, num_vertices, indices, num_indices, texture);
    RenderCompiledGeometry(handle, translation, texture);
    ReleaseCompiledGeometry(handle);
}

Rml::CompiledGeometryHandle CairoRenderInterface::CompileGeometry(Rml::Vertex* vertices, int num_vertices,
                                                                  int* indices, int num_indices,
                                                                  Rml::TextureHandle texture)
{
    auto geo = new Geometry();
    geo->verts.assign(vertices, vertices + num_vertices);
    geo->indices.assign(indices, indices + num_indices);
    geo->tex = texture;
    geometries_.push_back(geo);
    return reinterpret_cast<Rml::CompiledGeometryHandle>(geo);
}

void CairoRenderInterface::RenderCompiledGeometry(Rml::CompiledGeometryHandle handle,
                                                  const Rml::Vector2f& translation,
                                                  Rml::TextureHandle texture)
{
    auto geo = reinterpret_cast<Geometry*>(handle);
    if(!geo) return;
    if(texture == 0) texture = geo->tex;
    cairo_t* cr = canvas_.cr();
    cairo_save(cr);
    cairo_translate(cr, translation.x, translation.y);
    cairo_new_path(cr);
    for(size_t i=0;i+2<geo->indices.size();i+=3) {
        const auto& v0 = geo->verts[geo->indices[i]];
        const auto& v1 = geo->verts[geo->indices[i+1]];
        const auto& v2 = geo->verts[geo->indices[i+2]];
        cairo_move_to(cr, v0.position.x, v0.position.y);
        cairo_line_to(cr, v1.position.x, v1.position.y);
        cairo_line_to(cr, v2.position.x, v2.position.y);
        cairo_close_path(cr);
    }
    if(texture) {
        if(auto pat = make_pattern(texture)) {
            cairo_set_source(cr, pat);
            cairo_pattern_destroy(pat);
        }
    } else if(!geo->verts.empty()) {
        auto col = geo->verts[0].colour;
        cairo_set_source_rgba(cr, col.red/255.0, col.green/255.0, col.blue/255.0, col.alpha/255.0);
    }
    cairo_fill(cr);
    cairo_restore(cr);
}

void CairoRenderInterface::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle handle)
{
    auto geo = reinterpret_cast<Geometry*>(handle);
    if(!geo) return;
    geometries_.erase(std::remove(geometries_.begin(), geometries_.end(), geo), geometries_.end());
    delete geo;
}

void CairoRenderInterface::EnableScissorRegion(bool enable)
{
    if(!enable) {
        cairo_reset_clip(canvas_.cr());
    }
}

void CairoRenderInterface::SetScissorRegion(int x, int y, int width, int height)
{
    cairo_t* cr = canvas_.cr();
    cairo_reset_clip(cr);
    cairo_rectangle(cr, x, y, width, height);
    cairo_clip(cr);
}

static cairo_surface_t* load_png(const std::string& path)
{
    return cairo_image_surface_create_from_png(path.c_str());
}

static cairo_surface_t* load_jpeg(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "rb");
    if(!f) return nullptr;
    jpeg_decompress_struct cinfo{};
    jpeg_error_mgr jerr{};
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                      cinfo.output_width,
                                                      cinfo.output_height);
    int stride = cairo_image_surface_get_stride(surf);
    unsigned char* data = cairo_image_surface_get_data(surf);
    std::vector<unsigned char> row(cinfo.output_width * cinfo.output_components);
    while(cinfo.output_scanline < cinfo.output_height) {
        unsigned char* rowptr = row.data();
        jpeg_read_scanlines(&cinfo, &rowptr, 1);
        unsigned char* dst = data + (cinfo.output_scanline-1) * stride;
        for(unsigned int x=0;x<cinfo.output_width;x++) {
            dst[x*4+0] = row[x*3+2];
            dst[x*4+1] = row[x*3+1];
            dst[x*4+2] = row[x*3+0];
            dst[x*4+3] = 255;
        }
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(f);
    cairo_surface_mark_dirty(surf);
    return surf;
}

static cairo_surface_t* load_gif(const std::string& path)
{
    int err = 0;
    GifFileType* gif = DGifOpenFileName(path.c_str(), &err);
    if(!gif) return nullptr;
    DGifSlurp(gif);
    if(gif->SavedImages == nullptr) { DGifCloseFile(gif, &err); return nullptr; }
    auto& im = gif->SavedImages[0];
    int w = im.ImageDesc.Width;
    int h = im.ImageDesc.Height;
    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    int stride = cairo_image_surface_get_stride(surf);
    unsigned char* data = cairo_image_surface_get_data(surf);
    GifColorType* colors = gif->SColorMap ? gif->SColorMap->Colors : nullptr;
    for(int y=0;y<h;y++) {
        for(int x=0;x<w;x++) {
            GifByteType idx = im.RasterBits[y*w + x];
            GifColorType c = colors ? colors[idx] : GifColorType{0,0,0};
            unsigned char* dst = data + y*stride + x*4;
            dst[0] = c.Blue;
            dst[1] = c.Green;
            dst[2] = c.Red;
            dst[3] = 255;
        }
    }
    cairo_surface_mark_dirty(surf);
    DGifCloseFile(gif, &err);
    return surf;
}

bool CairoRenderInterface::LoadTexture(Rml::TextureHandle& texture_handle,
                                       Rml::Vector2i& texture_dimensions,
                                       const Rml::String& source)
{
    std::string path(source); // assume ascii
    cairo_surface_t* surf = nullptr;
    if(path.rfind(".png") != std::string::npos) surf = load_png(path);
    else if(path.rfind(".jpg") != std::string::npos || path.rfind(".jpeg") != std::string::npos) surf = load_jpeg(path);
    else if(path.rfind(".gif") != std::string::npos) surf = load_gif(path);
    if(!surf) return false;
    texture_handle = next_texture_++;
    Image img{surf, cairo_image_surface_get_width(surf), cairo_image_surface_get_height(surf)};
    textures_[texture_handle] = img;
    texture_dimensions = {img.w, img.h};
    return true;
}

bool CairoRenderInterface::GenerateTexture(Rml::TextureHandle& texture_handle,
                                           const Rml::byte* source,
                                           const Rml::Vector2i& dimensions)
{
    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                      dimensions.x, dimensions.y);
    int stride = cairo_image_surface_get_stride(surf);
    unsigned char* data = cairo_image_surface_get_data(surf);
    for(int y=0;y<dimensions.y;y++) {
        const unsigned char* src = source + y*dimensions.x*4;
        unsigned char* dst = data + y*stride;
        memcpy(dst, src, dimensions.x*4);
    }
    cairo_surface_mark_dirty(surf);
    texture_handle = next_texture_++;
    textures_[texture_handle] = {surf, dimensions.x, dimensions.y};
    return true;
}

void CairoRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle)
{
    auto it = textures_.find(texture_handle);
    if(it != textures_.end()) {
        cairo_surface_destroy(it->second.surf);
        textures_.erase(it);
    }
}
