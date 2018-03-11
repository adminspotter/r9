#include <tap++.h>

using namespace TAP;

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

void test_bad_constructor(void)
{
    std::string test = "constructor failure: ";
    error_fail = true;
    try
    {
        std::string badname = "this doesn't exist";
        Library *lib1 = new Library(badname);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("error opening library"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    error_fail = false;
}

void test_good_constructor(void)
{
    std::string test = "constructor: ";
    Library *lib1 = NULL;
    try
    {
        std::string libname = good_lib;
        lib1 = new Library(libname);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    ok(lib1 != NULL, test + "library created");
    delete lib1;
}

void test_missing_symbol(void)
{
    std::string test = "symbol failure: ";
    Library *lib1 = NULL;
    try
    {
        std::string libname = good_lib;
        lib1 = new Library(libname);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    ok(lib1 != NULL, test + "library created");
    error_fail = true;
    try
    {
        lib1->symbol("symbol that doesn't exist");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("error finding symbol"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    error_fail = false;
    delete lib1;
}

void test_good_symbol(void)
{
    std::string test = "symbol: ";
    Library *lib1 = NULL;

    try
    {
        std::string libname = good_lib;
        lib1 = new Library(libname);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    ok(lib1 != NULL, test + "library created");
    void *sym1 = NULL;
    try
    {
        sym1 = lib1->symbol("symbol that does exist");
    }
    catch (...)
    {
        fail(test + "symbol exception");
    }
    ok(sym1 != NULL, test + "expected symbol");
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

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    plan(6);

    int gtests = RUN_ALL_TESTS();

    test_bad_constructor();
    test_good_constructor();
    test_missing_symbol();
    test_good_symbol();
    return gtests & exit_status();
}
