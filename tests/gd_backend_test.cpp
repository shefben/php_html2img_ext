#include <gtest/gtest.h>
#include "../src/gd_canvas.hpp"
#include "../src/gd_container.hpp"
#include <litehtml.h>

TEST(TransparentSurfaceTest, AlphaZeroWhenNoBackground)
{
    GDCanvas canvas(10,10,false);
    GDContainer cont(canvas, ".", "./", false);
    auto doc = litehtml::document::createFromString("<div style=\"width:10px;height:10px\"></div>", &cont);
    doc->render(10);
    litehtml::position clip(0,0,10,10);
    doc->draw(0,0,0,&clip);
    int c = gdImageGetTrueColorPixel(canvas.img(),0,0);
    EXPECT_EQ(gdTrueColorGetAlpha(c), 127);
}

TEST(OpaqueSurfaceTest, BackgroundFill)
{
    const char* html = "<div style=\"background:#7B7F8D;width:10px;height:10px\"></div>";
    GDCanvas canvas(10,10,true, gdTrueColor(0x7B,0x7F,0x8D));
    GDContainer cont(canvas, ".", "./", false);
    auto doc = litehtml::document::createFromString(html, &cont);
    doc->render(10);
    litehtml::position clip(0,0,10,10);
    doc->draw(0,0,0,&clip);
    int c = gdImageGetTrueColorPixel(canvas.img(),0,0);
    EXPECT_EQ(gdTrueColorGetAlpha(c), 0);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
