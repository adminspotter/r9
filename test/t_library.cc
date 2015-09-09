#include "../server/classes/library.h"
#include <gtest/gtest.h>

#include "libtest.h"

#define LIBTEST "./.libs/libtest.dylib"

TEST(LibraryTest, BadConstructor)
{
    ASSERT_THROW(
        {
            std::string noname;
            Library *lib1 = new Library(noname);
        },
        std::runtime_error);
}

TEST(LibraryTest, GoodConstructor)
{
    Library *lib1 = NULL;
    ASSERT_NO_THROW(
        {
            std::string libname = LIBTEST;
            lib1 = new Library(libname);
        });
    ASSERT_TRUE(lib1 != NULL);
    delete lib1;
}

TEST(LibraryTest, Fixture)
{
    Library *lib1 = NULL;
    ASSERT_NO_THROW(
        {
            std::string libname = LIBTEST;
            lib1 = new Library(libname);
        });
    ASSERT_TRUE(lib1 != NULL);
    delete lib1;
}

TEST(LibraryTest, MissingSymbol)
{
    Library *lib1 = NULL;
    ASSERT_NO_THROW(
        {
            std::string libname = LIBTEST;
            lib1 = new Library(libname);
        });
    ASSERT_TRUE(lib1 != NULL);
    ASSERT_THROW(
        {
            lib1->symbol("symbol that doesn't exist");
        },
        std::runtime_error);
    delete lib1;
}

TEST(LibraryTest, GoodSymbol)
{
    Library *lib1 = NULL;
    ASSERT_NO_THROW(
        {
            std::string libname = LIBTEST;
            lib1 = new Library(libname);
        });
    ASSERT_TRUE(lib1 != NULL);
    void *sym1 = NULL;
    ASSERT_NO_THROW(
        {
            sym1 = lib1->symbol("test_func");
        });
    ASSERT_TRUE(sym1 != NULL);

    /* test_func takes an int and adds 2 */
    int (*func1)(int) = (int (*)(int))sym1;
    int x = func1(5);
    ASSERT_EQ(x, 7);
    delete lib1;
}

TEST(LibraryTest, Factory)
{
    Library *lib1 = NULL;
    ASSERT_NO_THROW(
        {
            std::string libname = LIBTEST;
            lib1 = new Library(libname);
        });
    ASSERT_TRUE(lib1 != NULL);

    Thingie *(*factory_create)(void)
        = (Thingie *(*)(void))lib1->symbol("create_thingie");
    void (*factory_destroy)(Thingie *)
        = (void (*)(Thingie *))lib1->symbol("destroy_thingie");
    std::string (*what_are_you)(Thingie *)
        = (std::string (*)(Thingie *))lib1->symbol("what");

    ASSERT_TRUE(factory_create != NULL);
    ASSERT_TRUE(factory_destroy != NULL);
    ASSERT_TRUE(what_are_you != NULL);

    Thingie *t;
    ASSERT_NO_THROW({ t = factory_create(); });
    ASSERT_TRUE(t != NULL);

    std::string str;
    ASSERT_NO_THROW({ str = what_are_you(t); });
    ASSERT_STREQ(str.c_str(), "a thingie!");
    ASSERT_NO_THROW(factory_destroy(t));
    delete lib1;
}
