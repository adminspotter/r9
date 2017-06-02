#include "../server/classes/game_obj.h"

#include <gtest/gtest.h>

bool pthread_mutex_lock_error = false, pthread_mutex_unlock_error = false;
int lock_count, unlock_count;

int pthread_mutex_lock(pthread_mutex_t *a)
{
    ++lock_count;
    if (pthread_mutex_lock_error == true)
        return EINVAL;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *a)
{
    ++unlock_count;
    if (pthread_mutex_unlock_error == true)
        return EINVAL;
    return 0;
}

TEST(GameObjTest, CreateDelete)
{
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 38LL);
    ASSERT_EQ(go->get_object_id(), 38LL);
    ASSERT_TRUE(go->master == con);
    ASSERT_TRUE(go->geometry == geom);
    ASSERT_EQ(lock_count, unlock_count);

    delete go;

    geom = new Geometry();
    go = new GameObject(geom, con);
    ASSERT_EQ(go->get_object_id(), 39LL);

    delete go;
    delete con;
}

TEST(GameObjTest, Clone)
{
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 45LL);
    ASSERT_EQ(go->get_object_id(), 45LL);
    ASSERT_TRUE(go->master == con);
    ASSERT_TRUE(go->geometry == geom);
    ASSERT_EQ(lock_count, unlock_count);

    GameObject *go2 = go->clone();
    ASSERT_EQ(go2->get_object_id(), 46LL);
    ASSERT_TRUE(go2->master == con);
    ASSERT_TRUE(go2->geometry != geom);

    delete go;
    delete go2;
    delete con;
}
