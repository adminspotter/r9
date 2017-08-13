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

TEST(ControlTest, CreateDeleteConnected)
{
    Control *con;
    GameObject *go1 = new GameObject(NULL, NULL, 123LL);
    GameObject *go2 = new GameObject(NULL, NULL, 124LL);

    ASSERT_NO_THROW(
        {
            con = new Control(123LL, go1);
        });
    ASSERT_TRUE(con->default_slave == go1);
    ASSERT_TRUE(con->slave == go1);

    con->slave = go2;

    ASSERT_NO_THROW(
        {
            delete con;
        });

    delete go2;
    delete go1;
}
