#include <tap++.h>

using namespace TAP;

#include "../server/classes/action_pool.h"
#include "../server/classes/game_obj.h"

#include "mock_db.h"
#include "mock_library.h"
#include "mock_listensock.h"
#include "mock_server_globals.h"
#include "mock_zone.h"

void register_actions(std::map<uint16_t, action_rec>&);
void unregister_actions(std::map<uint16_t, action_rec>&);
int fake_action(GameObject *, int, GameObject *, glm::dvec3&);

fake_Library *lib;
GameObject::objects_map *game_objs;
fake_listen_socket *listensock;
int register_count, unregister_count, action_count;

void register_actions(std::map<uint16_t, action_rec>& a)
{
    ++register_count;

    a[567].valid = false;
    a[567].def = 789;

    a[789].valid = true;
    a[789].action = &fake_action;
}

void unregister_actions(std::map<uint16_t, action_rec>& a)
{
    ++unregister_count;

    a.erase(567);
    a.erase(789);
}

int fake_action(GameObject *a, int b, GameObject *c, glm::dvec3& d)
{
    ++action_count;

    return 4;
}

void setup_fixture(void)
{
    database = new fake_DB("a", "b", "c", "d");

    /* The action pool takes control of this library, and
     * deletes it when the pool is destroyed.  We don't need
     * to delete it ourselves.
     */
    lib = new fake_Library("doesn't matter");

    symbol_count = 0;
    symbol_result = (void *)register_actions;

    game_objs = new GameObject::objects_map();
    (*game_objs)[9876LL] = new GameObject(NULL, NULL, 9876LL);

    listensock = new fake_listen_socket(NULL);

    zone = new fake_Zone(1000, 1, database);
}

void cleanup_fixture(void)
{
    delete (fake_Zone *)zone;
    delete listensock;
    delete (*game_objs)[9876LL];
    delete game_objs;
    delete (fake_DB *)database;
}

/* In testing the library itself, we know that it will throw an
 * exception when it can't find a symbol, so that's not interesting to
 * test in context of the ActionPool constructor.
 */
void test_create_delete(void)
{
    std::string test = "create/delete: ";

    setup_fixture();

    get_server_skills_count = register_count = unregister_count = 0;

    try
    {
        action_pool = new ActionPool(1, *game_objs, lib, database);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(get_server_skills_count, 1, test + "expected skills calls");
    is(register_count, 1, test + "expected register calls");

    symbol_result = (void *)unregister_actions;

    delete action_pool;
    is(unregister_count, 1, test + "expected unregister calls");

    cleanup_fixture();
}

void test_start_stop(void)
{
    std::string test = "start/stop: ";

    setup_fixture();

    action_pool = new ActionPool(1, *game_objs, lib, database);
    action_pool->start();
    is(action_pool->startup_arg == action_pool, true,
       test + "expected startup arg");
    is(action_pool->pool_size(), 1, test + "expected pool size");
    action_pool->stop();
    is(action_pool->pool_size(), 0, test + "expected pool size");

    symbol_result = (void *)unregister_actions;

    delete action_pool;

    cleanup_fixture();
}

void test_no_skill(void)
{
    std::string test = "no skill: ";

    setup_fixture();

    check_authorization_result = ACCESS_MOVE;
    get_character_objectid_result = 9876LL;
    base_user *bu = new base_user(123LL, "a", "b", listensock);

    action_pool = new ActionPool(1, *game_objs, lib, database);

    action_request pkt;
    memset(&pkt, 0, sizeof(action_request));
    pkt.type = TYPE_ACTREQ;
    pkt.version = 1;
    pkt.object_id = 9876LL;
    pkt.action_id = 12345;
    pkt.power_level = 5;

    action_count = 0;

    action_pool->execute_action(bu, pkt);
    is(action_count, 0, test + "expected action count");

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;

    cleanup_fixture();
}

void test_invalid_skill(void)
{
    std::string test = "invalid skill: ";

    setup_fixture();

    check_authorization_result = ACCESS_MOVE;
    get_character_objectid_result = 9876LL;
    base_user *bu = new base_user(123LL, "a", "b", listensock);
    bu->actions[567] = {567, 5, 0, 0};

    action_pool = new ActionPool(1, *game_objs, lib, database);

    action_request pkt;
    memset(&pkt, 0, sizeof(action_request));
    pkt.type = TYPE_ACTREQ;
    pkt.version = 1;
    pkt.object_id = 9876LL;
    pkt.action_id = 567;
    pkt.power_level = 5;

    action_count = 0;

    action_pool->execute_action(bu, pkt);
    is(action_count, 1, test + "expected action count");

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;

    cleanup_fixture();
}

void test_wrong_object_id(void)
{
    std::string test = "wrong object id: ";

    setup_fixture();

    check_authorization_result = ACCESS_MOVE;
    get_character_objectid_result = 9876LL;
    base_user *bu = new base_user(123LL, "a", "b", listensock);
    bu->actions[789] = {789, 5, 0, 0};

    action_pool = new ActionPool(1, *game_objs, lib, database);

    action_request pkt;
    memset(&pkt, 0, sizeof(action_request));
    pkt.type = TYPE_ACTREQ;
    pkt.version = 1;
    pkt.object_id = 123LL;
    pkt.action_id = 789;
    pkt.power_level = 5;

    action_count = 0;

    action_pool->execute_action(bu, pkt);
    is(action_count, 0, test + "expected action count");

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;

    cleanup_fixture();
}

void test_good_object_id(void)
{
    std::string test = "good object id: ";

    setup_fixture();

    check_authorization_result = ACCESS_MOVE;
    get_character_objectid_result = 9876LL;
    base_user *bu = new base_user(123LL, "a", "b", listensock);
    bu->actions[789] = {789, 5, 0, 0};

    action_pool = new ActionPool(1, *game_objs, lib, database);

    action_request pkt;
    memset(&pkt, 0, sizeof(action_request));
    pkt.type = TYPE_ACTREQ;
    pkt.version = 1;
    pkt.object_id = 9876LL;
    pkt.action_id = 789;
    pkt.power_level = 5;

    action_count = 0;

    action_pool->execute_action(bu, pkt);
    is(action_count, 1, test + "expected action count");

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;

    cleanup_fixture();
}

void test_worker(void)
{
    std::string test = "worker: ";

    setup_fixture();

    check_authorization_result = ACCESS_MOVE;
    get_character_objectid_result = 9876LL;
    base_user *bu = new base_user(123LL, "a", "b", listensock);
    bu->actions[789] = {789, 5, 0, 0};

    action_pool = new ActionPool(1, *game_objs, lib, database);

    packet_list pl;
    memset(&pl.buf, 0, sizeof(action_request));
    pl.buf.act.type = TYPE_ACTREQ;
    pl.buf.act.version = 1;
    pl.buf.act.object_id = 9876LL;
    pl.buf.act.action_id = 789;
    pl.buf.act.power_level = 5;
    pl.who = bu;
    action_pool->push(pl);

    action_count = 0;

    action_pool->start();

    while (action_count == 0)
        ;

    action_pool->stop();
    is(action_count, 1, test + "expected action count");
    symbol_error = true;
    try
    {
        delete action_pool;
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
    symbol_error = false;

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    cleanup_fixture();
}

int main(int argc, char **argv)
{
    plan(11);

    test_create_delete();
    test_start_stop();
    test_no_skill();
    test_invalid_skill();
    test_wrong_object_id();
    test_good_object_id();
    test_worker();
    return exit_status();
}
