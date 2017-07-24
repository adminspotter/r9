#include "../server/classes/dgram.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

TEST(DgramUserTest, CreateDelete)
{
    Control *control = new Control(0LL, NULL);
    dgram_user *dgu;

    ASSERT_NO_THROW(
        {
            dgu = new dgram_user(control->userid, control);
        });

    delete dgu;
    delete control;
}
