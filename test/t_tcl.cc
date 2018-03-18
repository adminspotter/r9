#include <tap++.h>

using namespace TAP;

#include <config.h>
#include "../server/classes/library.h"
#include "../server/classes/modules/language.h"

#include <string.h>

#define TCL_MOD "../server/classes/modules/.libs/libr9_tcl" LT_MODULE_EXT

Language *(*create_language)(void);
std::string (*execute_language)(Language *, const std::string&);
void (*destroy_language)(Language *);

void test_good_constructor(void)
{
    std::string test = "good constructor: ";
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

    /* From https://www.tcl.tk/about/language.html, creates a
     * procedure which raises the first arg to the power of the second
     * arg, which we then call to get 2^8.
     */
    std::string str = "proc power {base p} {\n    set result 1\n    while {$p > 0} {\n        set result [expr $result * $base]\n        set p [expr $p - 1]\n    }\n    return $result\n}\n\npower 2 8";
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

    /* Load up the perl lib and fetch the symbols */
    Library *lib = new Library(TCL_MOD);
    create_language = (Language *(*)(void))lib->symbol("create_language");
    execute_language = (std::string (*)(Language *, const std::string &))lib->symbol("lang_execute");
    destroy_language = (void (*)(Language *))lib->symbol("destroy_language");

    test_good_constructor();
    test_run_script();

    delete lib;

    return exit_status();
}
