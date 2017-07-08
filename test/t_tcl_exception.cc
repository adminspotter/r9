#include "../server/classes/modules/r9tcl.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

Tcl_Interp *Tcl_CreateInterp(void)
{
    /* We're only testing creation failure here, so no need to return
     * anything else.
     */
    return NULL;
}

TEST(TclTest, ConstructorFailure)
{
    TclLanguage *tcl;

    ASSERT_THROW(
        {
            tcl = new TclLanguage();
        },
        std::runtime_error);
}
