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

int main(int argc, char **argv)
{
    plan(0);

    /* Load up the lua lib and fetch the symbols */
    Library *lib = new Library(LUA_MOD);
    create_language = (Language *(*)(void))lib->symbol("create_language");
    execute_language = (std::string (*)(Language *, const std::string &))lib->symbol("lang_execute");
    destroy_language = (void (*)(Language *))lib->symbol("destroy_language");

    /* Tests here */

    delete lib;

    return exit_status();
}
