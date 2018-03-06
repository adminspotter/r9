#include <tap++.h>

using namespace TAP;

#include "../server/classes/control.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    Control *con;

    try
    {
        con = new Control(0LL, NULL);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(con->userid, 0LL, test + "expected userid");
    is(con->default_slave == NULL, true, test + "expected default slave");
    is(con->slave == NULL, true, test + "expected slave");

    try
    {
        delete con;
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
}

void test_create_delete_connected(void)
{
    std::string test = "create/delete connected: ";
    Control *con;
    GameObject *go1 = new GameObject(NULL, NULL, 123LL);
    GameObject *go2 = new GameObject(NULL, NULL, 124LL);

    try
    {
        con = new Control(123LL, go1);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(con->default_slave, go1, test + "expected default slave");
    is(con->slave, go1, test + "expected slave");

    con->slave = go2;

    try
    {
        delete con;
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }

    delete go2;
    delete go1;
}

void test_less_than(void)
{
    std::string test = "less-than: ";
    Control *con1 = new Control(1LL, NULL);
    Control *con2 = new Control(2LL, NULL);

    is(*con1 < *con2, true, test + "lesser is less than greater");
    is(*con2 < *con1, false, test + "greater is not less than lesser");

    delete con2;
    delete con1;
}

TEST(ControlTest, Equality)
{
    Control *con1 = new Control(1LL, NULL);
    Control *con2 = new Control(2LL, NULL);

    ASSERT_FALSE(*con1 == *con2);

    con1->userid = con2->userid;

    ASSERT_TRUE(*con1 == *con2);

    delete con2;
    delete con1;
}

TEST(ControlTest, Assignment)
{
    GameObject *go1 = new GameObject(NULL, NULL, 123LL);
    GameObject *go2 = new GameObject(NULL, NULL, 124LL);
    Control *con1 = new Control(123LL, go1);
    con1->slave = go2;
    con1->username = "howdy";

    Control *con2 = new Control(987LL, NULL);

    ASSERT_NE(con1->userid, con2->userid);
    ASSERT_FALSE(con1->default_slave == con2->default_slave);
    ASSERT_FALSE(con1->slave == con2->slave);
    ASSERT_FALSE(con1->username == con2->username);

    *con2 = *con1;

    ASSERT_EQ(con1->userid, con2->userid);
    ASSERT_TRUE(con1->default_slave == con2->default_slave);
    ASSERT_TRUE(con1->slave == con2->slave);
    ASSERT_TRUE(con1->username == con2->username);

    delete con2;
    delete con1;
    delete go2;
    delete go1;
}

TEST(ControlTest, TakeOver)
{
    GameObject *go1 = new GameObject(NULL, NULL, 123LL);
    Control *con = new Control(123LL, NULL);

    bool result = con->take_over(go1);

    ASSERT_TRUE(result);

    result = con->take_over(go1);

    ASSERT_FALSE(result);

    delete con;
    delete go1;
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    plan(7);

    int gtests = RUN_ALL_TESTS();

    test_create_delete();
    test_create_delete_connected();
    test_less_than();
    return gtests & exit_status();
}
