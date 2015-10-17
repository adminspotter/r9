#include <gtest/gtest.h>
#include "tap.h"

#include <config.h>
#include "../server/classes/library.h"
#include "../server/classes/modules/language.h"

#define TCL_MOD "../server/classes/modules/.libs/libr9_tcl" LT_MODULE_EXT

Language *(*create_language)(void);
std::string (*execute_language)(Language *, const std::string&);
void (*destroy_language)(Language *);

TEST(TclTest, GoodConstructor)
{
    Language *lang = NULL;

    ASSERT_NO_THROW(
        {
            lang = create_language();
        });
    ASSERT_TRUE(lang != NULL);

    ASSERT_NO_THROW(
        {
            destroy_language(lang);
        });
}

TEST(TclTest, RunScript)
{
    Language *lang = NULL;

    ASSERT_NO_THROW(
        {
            lang = create_language();
        });
    ASSERT_TRUE(lang != NULL);

    /* From https://www.tcl.tk/about/language.html, creates a
     * procedure which raises the first arg to the power of the second
     * arg, which we then call to get 2^8.
     */
    std::string str = "proc power {base p} {\n    set result 1\n    while {$p > 0} {\n        set result [expr $result * $base]\n        set p [expr $p - 1]\n    }\n    return $result\n}\n\npower 2 8";
    ASSERT_NO_THROW(
        {
            str = execute_language(lang, str);
        });
    ASSERT_FALSE(strcmp(str.c_str(), "256"));

    ASSERT_NO_THROW(
        {
            destroy_language(lang);
        });
}

GTEST_API_ int main(int argc, char **argv)
{
    int retval;

    testing::InitGoogleTest(&argc, argv);
    testing::TestEventListeners& listeners
        = testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new tap::TapListener());

    /* Load up the perl lib and fetch the symbols */
    Library *lib = new Library(TCL_MOD);
    create_language = (Language *(*)(void))lib->symbol("create_language");
    execute_language = (std::string (*)(Language *, const std::string &))lib->symbol("lang_execute");
    destroy_language = (void (*)(Language *))lib->symbol("destroy_language");

    retval = RUN_ALL_TESTS();
    delete lib;

    return retval;
}
