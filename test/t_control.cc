#include <tap++.h>

using namespace TAP;

#include "../server/classes/control.h"

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

void test_equality(void)
{
    std::string test = "equality: ";
    Control *con1 = new Control(1LL, NULL);
    Control *con2 = new Control(2LL, NULL);

    is(*con1 == *con2, false, test + "objects are not equal");

    con1->userid = con2->userid;

    is(*con1 == *con2, true, test + "objects are equal");

    delete con2;
    delete con1;
}

void test_assignment(void)
{
    std::string test = "assignment: ";
    GameObject *go1 = new GameObject(NULL, NULL, 123LL);
    GameObject *go2 = new GameObject(NULL, NULL, 124LL);
    Control *con1 = new Control(123LL, go1);
    con1->slave = go2;

    Control *con2 = new Control(987LL, NULL);

    isnt(con1->userid, con2->userid, test + "userids are not equal");
    is(con1->default_slave == con2->default_slave, false,
       test + "default slaves are not equal");
    is(con1->slave == con2->slave, false, test + "slaves are not equal");

    *con2 = *con1;

    is(con1->userid, con2->userid, test + "userids are equal");
    is(con1->default_slave, con2->default_slave,
       test + "default slaves are equal");
    is(con1->slave, con2->slave, test + "slaves are equal");

    delete con2;
    delete con1;
    delete go2;
    delete go1;
}

void test_take_over(void)
{
    std::string test = "take over: ";
    GameObject *go1 = new GameObject(NULL, NULL, 123LL);
    Control *con = new Control(123LL, NULL);

    bool result = con->take_over(go1);

    is(result, true, test + "took over object");

    result = con->take_over(go1);

    is(result, false, test + "couldn't take over object again");

    delete con;
    delete go1;
}

int main(int argc, char **argv)
{
    plan(17);

    test_create_delete();
    test_create_delete_connected();
    test_less_than();
    test_equality();
    test_assignment();
    test_take_over();
    return exit_status();
}
