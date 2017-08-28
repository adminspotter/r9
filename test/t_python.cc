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

int main(int argc, char **argv)
{
    plan(0);

    Library *lib = new Library(PYTHON_MOD);
    create_language = (lang_create_t)lib->symbol("create_language");
    execute_language = (lang_execute_t)lib->symbol("lang_execute");
    destroy_language = (lang_destroy_t)lib->symbol("destroy_language");

    /* Tests here */

    delete lib;

    return exit_status();
}
