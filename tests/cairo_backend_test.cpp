#include <gtest/gtest.h>
#include "../php-8.4.10-devel-vs17-x64/src/cairo_canvas.hpp"
#include "../php-8.4.10-devel-vs17-x64/src/rml_cairo_renderer.hpp"
#include <RmlUi/Core.h>

TEST(TransparentSurfaceTest, AlphaZeroWhenNoBackground)
{
    CairoCanvas canvas(10,10,false);
    CairoRenderInterface renderer(canvas);
    Rml::SetRenderInterface(&renderer);
    Rml::Initialise();
    auto* context = Rml::CreateContext("test", Rml::Vector2i(10,10));
    auto* document = context->LoadDocumentFromMemory("<div style='width:10px;height:10px'></div>");
    document->Show();
    context->Update();
    context->Render();
    unsigned char* data = cairo_image_surface_get_data(canvas.surface());
    EXPECT_EQ(data[3], 0);
}

TEST(OpaqueSurfaceTest, BackgroundFill)
{
    const char* html = "<div style=\"background:#7B7F8D;width:10px;height:10px\"></div>";
    CairoCanvas canvas(10,10,true, 0x7B7F8D);
    CairoRenderInterface renderer(canvas);
    Rml::SetRenderInterface(&renderer);
    Rml::Initialise();
    auto* context = Rml::CreateContext("test2", Rml::Vector2i(10,10));
    auto* document = context->LoadDocumentFromMemory(html);
    document->Show();
    context->Update();
    context->Render();
    unsigned char* data = cairo_image_surface_get_data(canvas.surface());
    EXPECT_EQ(data[3], 255);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
