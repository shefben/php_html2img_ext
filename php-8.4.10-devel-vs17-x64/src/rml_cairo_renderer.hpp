#ifndef HTML2IMG_RML_CAIRO_RENDERER_HPP
#define HTML2IMG_RML_CAIRO_RENDERER_HPP

#include "cairo_canvas.hpp"
#include <RmlUi/Core.h>
#include <vector>
#include <unordered_map>

class CairoRenderInterface : public Rml::RenderInterface
{
public:
    explicit CairoRenderInterface(CairoCanvas& canvas);
    ~CairoRenderInterface() override;

    void RenderGeometry(Rml::Vertex* vertices, int num_vertices,
                        int* indices, int num_indices,
                        Rml::TextureHandle texture,
                        const Rml::Vector2f& translation) override;
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* vertices, int num_vertices,
                                                int* indices, int num_indices,
                                                Rml::TextureHandle texture) override;
    void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry,
                                const Rml::Vector2f& translation,
                                Rml::TextureHandle texture) override;
    void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(int x, int y, int width, int height) override;

    bool LoadTexture(Rml::TextureHandle& texture_handle,
                     Rml::Vector2i& texture_dimensions,
                     const Rml::String& source) override;
    bool GenerateTexture(Rml::TextureHandle& texture_handle,
                         const Rml::byte* source,
                         const Rml::Vector2i& dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture_handle) override;

private:
    cairo_pattern_t* make_pattern(Rml::TextureHandle texture);

    CairoCanvas& canvas_;
    struct Geometry { std::vector<Rml::Vertex> verts; std::vector<int> indices; Rml::TextureHandle tex{0}; };
    std::vector<Geometry*> geometries_;

    struct Image { cairo_surface_t* surf{nullptr}; int w{}; int h{}; };
    std::unordered_map<Rml::TextureHandle, Image> textures_;
    int next_texture_{1};
};

#endif // HTML2IMG_RML_CAIRO_RENDERER_HPP
