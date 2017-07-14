#include <stdlib.h>

#include <iostream>
#include <string>
#include <vector>

#include "../client/ui/font.h"

#include <gtest/gtest.h>

#define FONT_NAME "techover.ttf"

std::vector<std::string> paths =
{
    "~/whatever/man",
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
    ".",
};

char fake_home[] = "/path/that/should/not/exist";

TEST(GlyphTest, IsLToR)
{
    ui::glyph g;

    /* Not in our map */
    g.code_point = 0x0099;
    ASSERT_TRUE(g.is_l_to_r());

    /* A singleton in the map */
    g.code_point = 0x05be;
    ASSERT_FALSE(g.is_l_to_r());

    /* A range in the map */
    g.code_point = 0x05df;
    ASSERT_FALSE(g.is_l_to_r());
}

TEST(FontTest, BasicCreateDelete)
{
    std::string font_name = FONT_NAME;
    ui::font *f = NULL;

    ASSERT_NO_THROW(
        {
            f = new ui::font(font_name, 10, paths);
        });
    ASSERT_TRUE(f != NULL);

    delete f;
}

TEST(FontTest, SearchPath)
{
    std::string font_name = FONT_NAME;
    ui::font *f = NULL;

    char *home = getenv("HOME"), *saved_home = NULL;

    if (home != NULL)
    {
        saved_home = strdup(home);
        unsetenv("HOME");
    }

    ASSERT_THROW(
        {
            f = new ui::font(font_name, 10, paths);
        },
        std::runtime_error);

    if (saved_home != NULL)
    {
        setenv("HOME", saved_home, 1);
        free(saved_home);
    }

    font_name = "noway_nohow_wontexist.font.hahaha";

    ASSERT_THROW(
        {
            f = new ui::font(font_name, 10, paths);
        },
        std::runtime_error);
}

TEST(FontTest, MaxCellSize)
{
    std::string font_name = FONT_NAME;
    ui::font *f = new ui::font(font_name, 30, paths);

    std::vector<int> sizes = {0, 0, 0};

    f->max_cell_size(sizes);

    ASSERT_GT(sizes[0], 0);
    ASSERT_GT(sizes[1], 0);
    ASSERT_GT(sizes[2], 0);

    delete f;
}

TEST(FontTest, GlyphAccess)
{
    std::string font_name = FONT_NAME;
    ui::font *f = NULL;

    ASSERT_NO_THROW(
        {
            f = new ui::font(font_name, 30, paths);
        });

    ui::glyph &g = (*f)['g'];
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
