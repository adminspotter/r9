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

TEST(DgramUserTest, Assignment)
{
    Control *control1 = new Control(0LL, NULL);
    Control *control2 = new Control(123LL, NULL);
    dgram_user *dgu1, *dgu2;

    dgu1 = new dgram_user(control1->userid, control1);
    dgu2 = new dgram_user(control2->userid, control2);

    ASSERT_NE(dgu1->userid, dgu2->userid);
    ASSERT_NE(dgu1->control, dgu2->control);

    *dgu2 = *dgu1;

    ASSERT_EQ(dgu1->userid, dgu2->userid);
    ASSERT_EQ(dgu1->control, dgu2->control);

    delete dgu2;
    delete dgu1;
    delete control2;
    delete control1;
}
