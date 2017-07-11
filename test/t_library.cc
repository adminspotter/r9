#include "../server/classes/library.h"
#include <gtest/gtest.h>

#include <config.h>
#include <string.h>

bool error_fail = false;
char bad_news[] = "oh noes";
char good_lib[] = "goodlib.so";

extern "C" {
    char *dlerror(void)
    {
        if (error_fail == true)
            return bad_news;
        return NULL;
    }

    void *dlopen(const char *a, int b)
    {
        if (strcmp(a, good_lib))
            return NULL;
        /* Doesn't matter what we return, as long as it's not NULL */
        return (void *)good_lib;
    }

    int dlclose(void *a)
    {
        return 0;
    }

    void *dlsym(void *a, const char *b)
    {
        /* Again, doesn't matter what we return. */
        return (void *)good_lib;
    }
}

TEST(LibraryTest, BadConstructor)
{
    error_fail = true;
    ASSERT_THROW(
        {
            std::string badname = "this doesn't exist";
            Library *lib1 = new Library(badname);
        },
        std::runtime_error);
    error_fail = false;
}

TEST(LibraryTest, GoodConstructor)
{
    Library *lib1 = NULL;
    ASSERT_NO_THROW(
        {
            std::string libname = good_lib;
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
            std::string libname = good_lib;
            lib1 = new Library(libname);
        });
    ASSERT_TRUE(lib1 != NULL);
    error_fail = true;
    ASSERT_THROW(
        {
            lib1->symbol("symbol that doesn't exist");
        },
        std::runtime_error);
    error_fail = false;
    delete lib1;
}

TEST(LibraryTest, GoodSymbol)
{
    Library *lib1 = NULL;
    ASSERT_NO_THROW(
        {
            std::string libname = good_lib;
            lib1 = new Library(libname);
        });
    ASSERT_TRUE(lib1 != NULL);
    void *sym1 = NULL;
    ASSERT_NO_THROW(
        {
            sym1 = lib1->symbol("symbol that does exist");
        });
    ASSERT_TRUE(sym1 != NULL);
    delete lib1;
}

TEST(LibraryTest, BadClose)
{
    Library *lib1 = NULL;
    ASSERT_NO_THROW(
        {
            std::string libname = good_lib;
            lib1 = new Library(libname);
        });
    ASSERT_TRUE(lib1 != NULL);

    error_fail = true;
    ASSERT_THROW(
        {
            lib1->close();
        },
        std::runtime_error);

    ASSERT_NO_THROW(delete lib1);
    error_fail = false;
}
