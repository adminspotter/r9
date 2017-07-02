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

TEST(BaseUserTest, LessThan)
{
    Control *con1 = new Control(123LL, NULL);
    Control *con2 = new Control(124LL, NULL);
    base_user *base1 = new base_user(123LL, con1);
    base_user *base2 = new base_user(124LL, con2);

    ASSERT_TRUE(*base1 < *base2);

    delete base2;
    delete base1;
    delete con2;
    delete con1;
}

TEST(BaseUserTest, EqualTo)
{
    Control *con1 = new Control(123LL, NULL);
    Control *con2 = new Control(124LL, NULL);
    base_user *base1 = new base_user(123LL, con1);
    base_user *base2 = new base_user(124LL, con2);

    ASSERT_FALSE(*base1 == *base2);

    delete base2;
    delete base1;
    delete con2;
    delete con1;
}

TEST(BaseUserTest, Assignment)
{
    Control *con1 = new Control(123LL, NULL);
    Control *con2 = new Control(124LL, NULL);
    base_user *base1 = new base_user(123LL, con1);
    base_user *base2 = new base_user(124LL, con2);

    ASSERT_FALSE(*base1 == *base2);

    *base1 = *base2;

    ASSERT_TRUE(*base1 == *base2);

    delete base2;
    delete base1;
    delete con2;
    delete con1;
}
