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
    ASSERT_GT(lock_count, 0);

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
    ASSERT_GT(lock_count, 0);

    GameObject *go2 = go->clone();
    ASSERT_EQ(go2->get_object_id(), 46LL);
    ASSERT_TRUE(go2->master == con);
    ASSERT_TRUE(go2->geometry != geom);

    delete go;
    delete go2;
    delete con;
}

TEST(GameObjTest, ConnectDisconnect)
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
    ASSERT_GT(lock_count, 0);

    Control *con2 = new Control(2LL, NULL);

    ASSERT_TRUE(go->connect(con2));
    ASSERT_TRUE(go->master == con2);

    Control *con3 = new Control(3LL, NULL);

    ASSERT_FALSE(go->connect(con3));
    ASSERT_TRUE(go->master == con2);

    delete con3;

    /* Disconnect something that's not the current master, should be no-op. */
    go->disconnect(con);
    ASSERT_TRUE(go->master == con2);

    go->disconnect(con2);
    ASSERT_TRUE(go->master == con);

    delete con2;
    delete go;
    delete con;
}

TEST(GameObjTest, ResetId)
{
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 123LL);
    ASSERT_EQ(go->get_object_id(), 123LL);
    ASSERT_TRUE(go->master == con);
    ASSERT_TRUE(go->geometry == geom);
    ASSERT_EQ(lock_count, unlock_count);
    ASSERT_GT(lock_count, 0);

    delete go;

    geom = new Geometry();
    go = new GameObject(geom, con);
    ASSERT_EQ(go->get_object_id(), 124LL);

    delete go;

    lock_count = unlock_count = 0;
    GameObject::reset_max_id();
    ASSERT_EQ(lock_count, unlock_count);
    ASSERT_GT(lock_count, 0);

    geom = new Geometry();
    go = new GameObject(geom, con);
    ASSERT_EQ(go->get_object_id(), 0LL);

    delete go;
    delete con;
}

TEST(GameObjTest, Distance)
{
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 123LL);
    ASSERT_EQ(go->get_object_id(), 123LL);
    ASSERT_TRUE(go->master == con);
    ASSERT_TRUE(go->geometry == geom);
    ASSERT_EQ(lock_count, unlock_count);
    ASSERT_GT(lock_count, 0);

    glm::dvec3 pt = {0.0, 0.0, 0.0};
    go->position.x = 1.0;
    ASSERT_EQ(go->distance_from(pt), 1.0);

    delete go;
    delete con;
}
