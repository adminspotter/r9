#include <tap++.h>

using namespace TAP;

#include "../server/classes/modules/r9tcl.h"

#include <stdexcept>

#include "mock_server_globals.h"

Tcl_Interp *Tcl_CreateInterp(void)
{
    /* We're only testing creation failure here, so no need to return
     * anything else.
     */
    return NULL;
}

void test_constructor_failure(void)
{
    std::string test = "constructor failure: ";
    TclLanguage *tcl;

    try
    {
        tcl = new TclLanguage();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't create tcl interpreter"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
}

int main(int argc, char **argv)
{
    plan(1);

    test_constructor_failure();
    return exit_status();
}
