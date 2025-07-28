#ifndef HTML2IMG_RML_CAIRO_RENDERER_HPP
#define HTML2IMG_RML_CAIRO_RENDERER_HPP

#include <RmlUi/Core.h>
#include <cairo.h>
#include <map>
#include <vector>
#include <memory>
#include "cairo_canvas.hpp"
#include "stb_image.h"

class CairoRenderInterface : public Rml::RenderInterface {
public:
    explicit CairoRenderInterface(CairoCanvas& canvas);
    ~CairoRenderInterface() override;

    void RenderGeometry(Rml::Span<const Rml::Vertex> vertices,
                        Rml::Span<const int> indices,
                        Rml::TextureHandle texture,
                        const Rml::Vector2f& translation) override;

    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                Rml::Span<const int> indices,
                                                Rml::TextureHandle texture) override;
    void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry,
                                const Rml::Vector2f& translation) override;
    void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry) override;

    bool LoadTexture(Rml::TextureHandle& texture_handle,
                     Rml::Vector2i& texture_dimensions,
                     const Rml::String& source) override;
    bool GenerateTexture(Rml::TextureHandle& texture_handle,
                         const Rml::byte* source,
                         const Rml::Vector2i& dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture_handle) override;
    void SetTransform(const Rml::Matrix4f* transform) override;

private:
    struct Texture { cairo_surface_t* surface; int w; int h; };
    struct Geometry { std::vector<Rml::Vertex> verts; std::vector<int> idx; Rml::TextureHandle tex; };

    CairoCanvas& canvas_;
    std::map<Rml::TextureHandle, std::unique_ptr<Texture>> textures_;
    std::map<Rml::CompiledGeometryHandle, std::unique_ptr<Geometry>> geometries_;
    Rml::Matrix4f transform_{1};
};

#endif // HTML2IMG_RML_CAIRO_RENDERER_HPP
