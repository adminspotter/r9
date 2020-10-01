#include <tap++.h>

using namespace TAP;

#include "../server/classes/zone.h"

#include "mock_db.h"
#include "mock_server_globals.h"

class object_DB : public fake_DB
{
  public:
    object_DB(const std::string& a, const std::string& b,
              const std::string& c, const std::string& d)
        : fake_DB(a, b, c, d)
        {};
    virtual ~object_DB() {};

    virtual int get_server_objects(GameObject::objects_map& a)
        {
            glm::dvec3 pos(100.0, 100.0, 100.0);

            a[1234LL] = new GameObject(NULL, NULL, 1234LL);
            a[1234LL]->set_position(pos);

            a[1235LL] = new GameObject(NULL, NULL, 1235LL);
            pos.x = 125.0;
            a[1235LL]->set_position(pos);

            ++get_server_objects_count;
            return 2;
        };
};

void test_create_simple(void)
{
    std::string test = "create simple: ";

    database = new fake_DB("a", "b", "c", "d");

    get_server_objects_count = 0;

    try
    {
        zone = new Zone(1000, 1, database);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(get_server_objects_count, 1, test + "expected objects call");
    is(zone->game_objects.size(), 0, test + "expected objects size");

    delete zone;
    delete (fake_DB *)database;
}

void test_create_complex(void)
{
    std::string test = "create complex: ";

    database = new object_DB("a", "b", "c", "d");

    get_server_objects_count = 0;

    try
    {
        zone = new Zone(1000, 2000, 3000, 1, 2, 3, database);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(get_server_objects_count, 1, test + "expected objects call");
    is(zone->game_objects.size(), 2, test + "expected objects size");

    delete zone;
    delete (object_DB *)database;
}

void test_sector_methods(void)
{
    std::string test = "sector methods: ";

    database = new fake_DB("a", "b", "c", "d");

    zone = new Zone(1000, 1000, 1000, 1, 1, 3, database);

    glm::dvec3 where1(500.0, 500.0, 500.0);
    glm::dvec3 where2(500.0, 500.0, 1500.0);
    Octree *o1 = zone->sector_contains(where1);
    Octree *o2 = zone->sector_contains(where2);
    is(o1 == o2, false, test + "sectors not equal");

    glm::ivec3 which1 = zone->which_sector(where1);
    glm::ivec3 expected1(0, 0, 0);
    is(which1 == expected1, true, test + "sectors equal");

    glm::ivec3 which2 = zone->which_sector(where2);
    glm::ivec3 expected2(0, 0, 1);
    is(which2 == expected2, true, test + "sectors equal");

    delete zone;
    delete (fake_DB *)database;
}

void test_send_objects(void)
{
    std::string test = "send_objects: ";
    int obj_size, queue_size;

    database = new object_DB("a", "b", "c", "d");

    zone = new Zone(1000, 1, database);
    obj_size = zone->game_objects.size();
    is(obj_size, 2, test + "expected objects size");

    update_pool = new UpdatePool("zonetest", 1);

    zone->send_nearby_objects(1234LL);
    is(zone->game_objects.size(), obj_size, test + "no new objects");

    /* Update queue will have length of this object, plus all other
     * objects "within visual range".  In our case here, it will be 2.
     */
    queue_size = update_pool->queue_size();
    is(queue_size, 2, test + "expected update size");

    zone->send_nearby_objects(9876LL);
    is(zone->game_objects.size(), obj_size + 1, test + "new object");
    is(update_pool->queue_size(), (queue_size * 2) + 1,
       test + "expected update size");

    delete update_pool;
    delete zone;
    delete (object_DB *)database;
}

int main(int argc, char **argv)
{
    plan(12);

    test_create_simple();
    test_create_complex();
    test_sector_methods();
    test_send_objects();
    return exit_status();
}
