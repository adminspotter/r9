#include <tap++.h>

using namespace TAP;

#include <config.h>
#include "../server/classes/library.h"
#include "../server/classes/modules/language.h"

#include <string.h>

#define LUA_MOD "../server/classes/modules/.libs/libr9_lua" LT_MODULE_EXT

Language *(*create_language)(void);
std::string (*execute_language)(Language *, const std::string&);
void (*destroy_language)(Language *);

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    Language *lang = NULL;

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
        destroy_language(lang);
    }
    catch (...)
    {
        fail(test + "destroy exception");
    }
}

void test_run_script(void)
{
    std::string test = "run script: ";
    Language *lang = NULL;

    try
    {
        lang = create_language();
    }
    catch (...)
    {
        fail(test + "create exception");
    }
    is(lang != NULL, true, test + "language object created");

    std::string str = "function power (base, p)\n  local result = 1\n  while (p > 0) do\n    result = result * base\n    p = p - 1\n  end\n  return result\nend\nreturn power(2, 8)";
    try
    {
        str = execute_language(lang, str);
    }
    catch (...)
    {
        fail(test + "execute exception");
    }
    is(strcmp(str.c_str(), "256"), 0, test + "expected result");

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
    plan(3);

    /* Load up the lua lib and fetch the symbols */
    Library *lib = new Library(LUA_MOD);
    create_language = (Language *(*)(void))lib->symbol("create_language");
    execute_language = (std::string (*)(Language *, const std::string &))lib->symbol("lang_execute");
    destroy_language = (void (*)(Language *))lib->symbol("destroy_language");

    test_create_delete();
    test_run_script();

    delete lib;

    return exit_status();
}
