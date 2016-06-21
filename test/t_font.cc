#include <iostream>
#include <string>
#include <vector>

#include "../client/ui/font.h"

#include <gtest/gtest.h>

/* This will be available on Macs, but probably nowhere else */
#define FONT_NAME "Times New Roman.ttf"

std::vector<std::string> paths =
{
#if defined(__APPLE__)
    "~/Library/Fonts",
    "/Library/Fonts",
    "/Network/Library/Fonts",
    "/System/Library/Fonts",
    "/System/Folder/Fonts",
#elif defined(__linux__)
    "/usr/share/fonts",
    "/usr/share/fonts/default/Type1",
    "/usr/share/fonts/default/ttf",
#endif
};

TEST(FontTest, BasicCreateDelete)
{
    std::string font_name = FONT_NAME;
    Font *f = NULL;

    ASSERT_NO_THROW(
        {
            f = new Font(font_name, 10, paths);
        });
    ASSERT_TRUE(f != NULL);

    delete f;
}

TEST(FontTest, GlyphAccess)
{
    std::string font_name = FONT_NAME;
    Font *f = NULL;

    ASSERT_NO_THROW(
        {
            f = new Font(font_name, 30, paths);
        });

    Glyph &g = (*f)['g'];
    ASSERT_GT(g.x_advance, 0);
    ASSERT_EQ(g.y_advance, 0);
    ASSERT_GT(g.width, 0);
    ASSERT_GT(g.height, 0);
    ASSERT_EQ(g.left, 0);
    ASSERT_GT(g.top, 0);
    ASSERT_GT(g.pitch, 0);
    ASSERT_TRUE(g.bitmap != NULL);

    delete f;
}
