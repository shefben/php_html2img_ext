#include "rml_cairo_renderer.hpp"
#include <cstring>

CairoRenderInterface::CairoRenderInterface(CairoCanvas& canvas)
    : canvas_(canvas) {}

CairoRenderInterface::~CairoRenderInterface()
{
    for(auto& t : textures_)
        cairo_surface_destroy(t.second->surface);
}

static void draw_triangle(cairo_t* cr, const Rml::Vertex& v0,
                          const Rml::Vertex& v1, const Rml::Vertex& v2)
{
    cairo_move_to(cr, v0.position.x, v0.position.y);
    cairo_line_to(cr, v1.position.x, v1.position.y);
    cairo_line_to(cr, v2.position.x, v2.position.y);
    cairo_close_path(cr);
    Rml::Colourb c = v0.colour;
    cairo_set_source_rgba(cr, c.red/255.0, c.green/255.0, c.blue/255.0, c.alpha/255.0);
    cairo_fill(cr);
}

void CairoRenderInterface::RenderGeometry(Rml::Span<const Rml::Vertex> vertices,
                                          Rml::Span<const int> indices,
                                          Rml::TextureHandle texture,
                                          const Rml::Vector2f& translation)
{
    cairo_t* cr = canvas_.cr();
    cairo_save(cr);
    cairo_translate(cr, translation.x, translation.y);
    if(texture)
    {
        auto it = textures_.find(texture);
        if(it != textures_.end())
        {
            cairo_set_source_surface(cr, it->second->surface, 0, 0);
            cairo_paint(cr);
            cairo_restore(cr);
            return;
        }
    }
    for(size_t i=0;i<indices.size();i+=3)
    {
        const Rml::Vertex& v0 = vertices[indices[i]];
        const Rml::Vertex& v1 = vertices[indices[i+1]];
        const Rml::Vertex& v2 = vertices[indices[i+2]];
        draw_triangle(cr, v0, v1, v2);
    }
    cairo_restore(cr);
}

Rml::CompiledGeometryHandle CairoRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                                  Rml::Span<const int> indices,
                                                                  Rml::TextureHandle texture)
{
    auto geom = std::make_unique<Geometry>();
    geom->verts.assign(vertices.begin(), vertices.end());
    geom->idx.assign(indices.begin(), indices.end());
    geom->tex = texture;
    Rml::CompiledGeometryHandle handle = reinterpret_cast<Rml::CompiledGeometryHandle>(geom.get());
    geometries_[handle] = std::move(geom);
    return handle;
}

void CairoRenderInterface::RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry,
                                                  const Rml::Vector2f& translation)
{
    auto it = geometries_.find(geometry);
    if(it == geometries_.end()) return;
    RenderGeometry(it->second->verts, it->second->idx, it->second->tex, translation);
}

void CairoRenderInterface::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry)
{
    geometries_.erase(geometry);
}

bool CairoRenderInterface::LoadTexture(Rml::TextureHandle& texture_handle,
                                       Rml::Vector2i& texture_dimensions,
                                       const Rml::String& source)
{
    int x,y,n;
    filesystem::path p(source); // assume absolute
    unsigned char* data = stbi_load(p.string().c_str(), &x, &y, &n, 4);
    if(!data) return false;
    cairo_surface_t* s = cairo_image_surface_create_for_data(data,
        CAIRO_FORMAT_ARGB32,x,y,x*4);
    auto tex = std::make_unique<Texture>();
    tex->surface = s;
    tex->w=x; tex->h=y;
    texture_handle = reinterpret_cast<Rml::TextureHandle>(tex.get());
    texture_dimensions = Rml::Vector2i(x,y);
    textures_[texture_handle] = std::move(tex);
    return true;
}

bool CairoRenderInterface::GenerateTexture(Rml::TextureHandle& handle,
                                           const Rml::byte* source,
                                           const Rml::Vector2i& dimensions)
{
    cairo_surface_t* s = cairo_image_surface_create_for_data((unsigned char*)source,
        CAIRO_FORMAT_ARGB32, dimensions.x, dimensions.y, dimensions.x*4);
    auto tex = std::make_unique<Texture>();
    tex->surface = s; tex->w=dimensions.x; tex->h=dimensions.y;
    handle = reinterpret_cast<Rml::TextureHandle>(tex.get());
    textures_[handle] = std::move(tex);
    return true;
}

void CairoRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle)
{
    auto it = textures_.find(texture_handle);
    if(it != textures_.end())
        textures_.erase(it);
}

void CairoRenderInterface::SetTransform(const Rml::Matrix4f* transform)
{
    if(transform)
        transform_ = *transform;
    else
        transform_ = Rml::Matrix4f{1};
}
