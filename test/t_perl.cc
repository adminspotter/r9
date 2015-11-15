#include <gtest/gtest.h>

#include <config.h>
#include "../server/classes/library.h"
#include "../server/classes/modules/language.h"

#define PERL_MOD "../server/classes/modules/.libs/libr9_perl" LT_MODULE_EXT

/* There was so much to mock to not have to deal with an actual perl
 * interpreter, it just made more sense to turn this into an
 * integration test.
 */

Language *(*create_language)(void);
std::string (*execute_language)(Language *, const std::string&);
void (*destroy_language)(Language *);

TEST(PerlTest, GoodConstructor)
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

TEST(PerlTest, RunScript)
{
    Language *lang = NULL;

    ASSERT_NO_THROW(
        {
            lang = create_language();
        });
    ASSERT_TRUE(lang != NULL);

    std::string str = "sub power { my ($base, $exp) = @_; $result = 1; while ($exp > 0) { $result *= $base; --$exp; } return $result; } print power(2, 8);";
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

    /* Load up the perl lib and fetch the symbols */
    Library *lib = new Library(PERL_MOD);
    create_language = (Language *(*)(void))lib->symbol("create_language");
    execute_language = (std::string (*)(Language *, const std::string &))lib->symbol("lang_execute");
    destroy_language = (void (*)(Language *))lib->symbol("destroy_language");

    retval = RUN_ALL_TESTS();
    delete lib;

    return retval;
}
