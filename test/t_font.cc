#include <iostream>

#include "../client/ui/font.h"
#include "../client/configdata.h"

#include <gtest/gtest.h>

/* This will be available on Macs, but probably nowhere else */
#define FONT_NAME "Times New Roman.ttf"

/* Ideally we would set up a tempdir with no config file, so we would
 * get the default settings.  We may yet need to do this.  For now,
 * however, we'll just take whatever we get, and hopefully that'll
 * include the system paths.
 */
ConfigData config;

TEST(FontTest, BasicCreateDelete)
{
    std::string font_name = FONT_NAME;
    Font *f = NULL;

    ASSERT_NO_THROW(
        {
            f = new Font(font_name, 10);
        });
    ASSERT_TRUE(f != NULL);

    ASSERT_NO_THROW(
        {
            delete f;
        });
}
