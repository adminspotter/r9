#include <tap++.h>

using namespace TAP;

#include <config.h>
#include "../server/classes/library.h"
#include "../server/classes/modules/language.h"

#define PYTHON_MOD "../server/classes/modules/.libs/libr9_python" LT_MODULE_EXT

Library *lib;
lang_create_t create_language;
lang_execute_t execute_language;
lang_destroy_t destroy_language;

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    Language *lang = NULL, *lang2 = NULL;

    try
    {
        lang = create_language();
    }
    catch (...)
    {
        fail(test + "create exception");
    }
    is(lang != NULL, true, test + "language object created");
    try
    {
        lang2 = create_language();
    }
    catch (...)
    {
        fail(test + "create exception");
    }
    is(lang2 != NULL, true, test + "second language object created");

    try
    {
        destroy_language(lang2);
    }
    catch (...)
    {
        fail(test + "destroy exception");
    }
    try
    {
        destroy_language(lang);
    }
    catch (...)
    {
        fail(test + "destroy exception");
    }
}

int main(int argc, char **argv)
{
    plan(2);

    Library *lib = new Library(PYTHON_MOD);
    create_language = (lang_create_t)lib->symbol("create_language");
    execute_language = (lang_execute_t)lib->symbol("lang_execute");
    destroy_language = (lang_destroy_t)lib->symbol("destroy_language");

    test_create_delete();

    delete lib;

    return exit_status();
}
