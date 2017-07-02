#include "../server/classes/listensock.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

TEST(BaseUserTest, CreateDelete)
{
    Control *con = new Control(0LL, NULL);
    base_user *base = NULL;

    ASSERT_NO_THROW(
        {
            base = new base_user(0LL, con);
        });
    ASSERT_EQ(base->userid, 0LL);
    ASSERT_EQ(base->control, con);
    ASSERT_EQ(base->pending_logout, false);

    delete base;
    delete con;
}
