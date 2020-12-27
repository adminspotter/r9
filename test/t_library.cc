#include <tap++.h>

using namespace TAP;

#include <stdexcept>

#include "../server/classes/library.h"

#include <config.h>
#include <string.h>
#include <glob.h>

bool error_fail = false;
int glob_fail = 0;
char bad_news[] = "oh noes";
char good_lib[] = "goodlib.so";
char found_libs[2][10] = {"first.so", "second.so"};

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

    int glob(const char *a, int b, int (*c)(const char *, int), glob_t *d)
    {
        if (glob_fail == 0)
        {
            d->gl_pathc = 2;
            d->gl_pathv = new char *[2];
            d->gl_pathv[0] = found_libs[0];
            d->gl_pathv[1] = found_libs[1];
        }
        return glob_fail;
    }

    void globfree(glob_t *a)
    {
        delete[] a->gl_pathv;
    }
}

int open_count = 0, close_count = 0;

class default_Library : public Library
{
  public:
    default_Library() : Library() {};
    virtual ~default_Library() {};

    void open(void)
        {
            ++open_count;
        };
    void close(void)
        {
            ++close_count;
        };
};

void test_default_constructor(void)
{
    std::string test = "default constructor: ";
    default_Library *lib = NULL;

    try
    {
        lib = new default_Library();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    is(open_count, 0, test + "expected open count");

    delete lib;

    is(close_count, 0, test + "expected close count");
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

void test_bad_close(void)
{
    std::string test = "close failure: ";
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
        lib1->close();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("error closing library"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    try
    {
        delete lib1;
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
    error_fail = false;
}

void test_find_libraries(void)
{
    std::string test = "find libraries: ", st;
    std::vector<Library *> libs;

    st = "unknown error: ";
    glob_fail = 5;
    try
    {
        find_libraries("whatever", libs);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Unknown error"), std::string::npos,
             test + st + "correct error contents");
    }
    is(libs.size(), 0, test + st + "expected vector size");

    st = "not implemented: ";
    glob_fail = 4;
    try
    {
        find_libraries("whatever", libs);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Function not implemented"), std::string::npos,
             test + st + "correct error contents");
    }
    is(libs.size(), 0, test + st + "expected vector size");

    st = "no match: ";
    glob_fail = 3;
    try
    {
        find_libraries("whatever", libs);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("No match found"), std::string::npos,
             test + st + "correct error contents");
    }
    is(libs.size(), 0, test + st + "expected vector size");

    st = "read error: ";
    glob_fail = 2;
    try
    {
        find_libraries("whatever", libs);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Read error"), std::string::npos,
             test + st + "correct error contents");
    }
    is(libs.size(), 0, test + st + "expected vector size");

    st = "OOM: ";
    glob_fail = 1;
    try
    {
        find_libraries("whatever", libs);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Out of memory"), std::string::npos,
             test + st + "correct error contents");
    }
    is(libs.size(), 0, test + st + "expected vector size");

    st = "success: ";
    glob_fail = 0;
    find_libraries("whatever", libs);
    is(libs.size(), 2, test + st + "expected vector size");

    st = "deleting existent stuff: ";
    find_libraries("whatever", libs);
    is(libs.size(), 2, test + st + "expected vector size");
}

int main(int argc, char **argv)
{
    plan(22);

    test_default_constructor();
    test_bad_constructor();
    test_good_constructor();
    test_missing_symbol();
    test_good_symbol();
    test_bad_close();
    test_find_libraries();
    return exit_status();
}
