#include "../server/classes/control.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

TEST(ControlTest, CreateDelete)
{
    Control *con;

    ASSERT_NO_THROW(
        {
            con = new Control(0LL, NULL);
        });
    ASSERT_EQ(con->userid, 0LL);
    ASSERT_TRUE(con->default_slave == NULL);
    ASSERT_TRUE(con->slave == NULL);

    ASSERT_NO_THROW(
        {
            delete con;
        });
}
