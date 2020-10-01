#include <tap++.h>

using namespace TAP;

#include "../server/classes/game_obj.h"

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

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    GameObject *go = NULL;
    Geometry *geom = new Geometry(), *geom2 = new Geometry();
    Control *con = new Control(1LL, NULL);

    GameObject::reset_max_id();

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 38LL);
    is(go->get_object_id(), 38LL, test + "expected objectid");
    is(go->master, con, test + "expected master");
    is(go->geometry, geom, test + "expected geometry");
    ok(lock_count > 0, test + "performed locks");
    is(lock_count, unlock_count, test + "all locks unlocked");

    delete go;

    geom = new Geometry();
    go = new GameObject(geom, con);
    go->geometry = geom2;
    is(go->get_object_id(), 39LL, test + "expected objectid");

    delete go;
    delete con;
}

void test_clone(void)
{
    std::string test = "clone: ";
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    GameObject::reset_max_id();

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 45LL);
    is(go->get_object_id(), 45LL, test + "expected objectid");
    is(go->master, con, test + "expected master");
    is(go->geometry, geom, test + "expected geometry");
    ok(lock_count > 0, test + "performed locks");
    is(lock_count, unlock_count, test + "all locks unlocked");

    GameObject *go2 = go->clone();
    is(go2->get_object_id(), 46LL, test + "expected objectid");
    is(go2->master, con, test + "expected master");
    isnt(go2->geometry, geom, test + "expected geometry");

    delete go;
    delete go2;
    delete con;
}

void test_connect_disconnect(void)
{
    std::string test = "connect/disconnect: ";
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    GameObject::reset_max_id();

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 45LL);
    is(go->get_object_id(), 45LL, test + "expected objectid");
    is(go->master, con, test + "expected master");
    is(go->geometry, geom, test + "expected geometry");
    ok(lock_count > 0, test + "performed locks");
    is(lock_count, unlock_count, test + "all locks unlocked");

    Control *con2 = new Control(2LL, NULL);

    is(go->connect(con2), true, test + "connect succeeded");
    is(go->master, con2, test + "set master");

    Control *con3 = new Control(3LL, NULL);

    is(go->connect(con3), false, test + "connect failed");
    is(go->master, con2, test + "didn't set master");

    delete con3;

    /* Disconnect something that's not the current master, should be no-op. */
    go->disconnect(con);
    is(go->master, con2, test + "didn't reset master");

    go->disconnect(con2);
    is(go->master, con, test + "reset master");

    delete con2;
    delete go;
    delete con;
}

void test_activate_deactivate(void)
{
    std::string test = "activate/deactivate: ";
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    go = new GameObject(geom, con, 45LL);
    is(go->natures.size(), 0, test + "expected initial natures size");

    go->deactivate();
    is(go->natures.size(), 2, test + "expected deactivated natures size");

    go->activate();
    is(go->natures.size(), 0, test + "expected activated natures size");

    delete go;
    delete con;
}

void test_reset_id(void)
{
    std::string test = "reset_max_id: ";
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 123LL);
    is(go->get_object_id(), 123LL, test + "expected objectid");
    is(go->master, con, test + "expected master");
    is(go->geometry, geom, test + "expected geometry");
    ok(lock_count > 0, test + "performed locks");
    is(lock_count, unlock_count, test + "all locks unlocked");

    delete go;

    geom = new Geometry();
    go = new GameObject(geom, con);
    is(go->get_object_id(), 124LL, test + "expected objectid");

    delete go;

    lock_count = unlock_count = 0;
    GameObject::reset_max_id();
    ok(lock_count > 0, test + "performed locks");
    is(lock_count, unlock_count, test + "all locks unlocked");

    geom = new Geometry();
    go = new GameObject(geom, con);
    is(go->get_object_id(), 0LL, test + "expected objectid");

    delete go;
    delete con;
}

void test_distance(void)
{
    std::string test = "distance_from: ";
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    lock_count = unlock_count = 0;
    go = new GameObject(geom, con, 123LL);
    is(go->get_object_id(), 123LL, test + "expected objectid");
    is(go->master, con, test + "expected master");
    is(go->geometry, geom, test + "expected geometry");
    ok(lock_count > 0, test + "performed locks");
    is(lock_count, unlock_count, test + "all locks unlocked");

    glm::dvec3 pt = {0.0, 0.0, 0.0};
    go->set_position(glm::dvec3(1.0, 0.0, 0.0));
    is(go->distance_from(pt), 1.0, test + "expected distance");

    delete go;
    delete con;
}

void test_accessors(void)
{
    std::string test = "accessors: ";
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);
    glm::dvec3 test_vec(1.0, 2.0, 3.0);
    glm::dquat test_quat(1.0, 2.0, 3.0, 4.0);

    go = new GameObject(geom, con, 45LL);

    go->set_position(test_vec);
    ok(go->get_position() == test_vec, test + "expected position");
    go->set_look(test_vec);
    ok(go->get_look() == test_vec, test + "expected look");
    go->set_movement(test_vec);
    ok(go->get_movement() == test_vec, test + "expected movement");
    go->set_orientation(test_quat);
    ok(go->get_orientation() == test_quat, test + "expected orientation");
    go->set_rotation(test_quat);
    ok(go->get_rotation() == test_quat, test + "expected rotation");

    delete go;
    delete con;
}

void test_move_and_rotate(void)
{
    std::string test = "move and rotate: ";
    GameObject *go = NULL;
    Geometry *geom = new Geometry();
    Control *con = new Control(1LL, NULL);

    go = new GameObject(geom, con, 45LL);
    go->move_and_rotate();

    ok(go->get_position() == glm::dvec3(0.0, 0.0, 0.0),
       test + "expected no movement");
    ok(go->get_orientation() == glm::dquat(1.0, 0.0, 0.0, 0.0),
       test + "expected no rotation");

    go->set_movement(glm::dvec3(1.0, 1.0, 1.0));
    go->set_rotation(glm::dquat(1.0, 0.0, 0.0, 1.0));

    sleep(1);
    go->move_and_rotate();

    ok(go->get_position() != glm::dvec3(0.0, 0.0, 0.0),
       test + "expected movement");
    ok(go->get_orientation() != glm::dquat(1.0, 0.0, 0.0, 0.0),
       test + "expected rotation");

    delete go;
    delete con;
}

int main(int argc, char **argv)
{
    plan(52);

    test_create_delete();
    test_clone();
    test_connect_disconnect();
    test_activate_deactivate();
    test_reset_id();
    test_distance();
    test_accessors();
    test_move_and_rotate();
    return exit_status();
}
