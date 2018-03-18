#include <tap++.h>

using namespace TAP;

#include "../server/classes/octree.h"

#include "mock_server_globals.h"

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    Octree *tree = NULL;
    glm::dvec3 min = {0.0, 0.0, 0.0}, max = {100.0, 100.0, 100.0};

    try
    {
        tree = new Octree(NULL, min, max, 0);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    is(tree->min_point == min, true, test + "expected min point");
    is(tree->max_point == max, true, test + "expected max point");

    try
    {
        delete tree;
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
}

void test_build_empty_list(void)
{
    std::string test = "build w/empty list: ";
    std::list<GameObject *> objs;
    glm::dvec3 min = {0.0, 0.0, 0.0}, max = {100.0, 100.0, 100.0};
    Octree *tree = new Octree(NULL, min, max, 0);

    try
    {
        tree->build(objs);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    is(tree->empty(), true, test + "expected empty");

    Octree *sub = tree->octants[0];
    is(sub != NULL, true, test + "expected subtree");
    while (sub->octants[0] != NULL)
        sub = sub->octants[0];
    is(sub->parent_index, 0, test + "expected parent index");
    is(sub->depth, Octree::MIN_DEPTH, test + "expected depth");

    delete tree;
}

void test_build(void)
{
    std::string test = "build: ";
    glm::dvec3 obj_center = {0.01, 0.01, 0.01};
    std::list<GameObject *> objs;

    Geometry *geom1 = new Geometry();
    geom1->center = obj_center;
    geom1->radius = 0.01;
    GameObject *go1 = new GameObject(geom1, NULL, 123LL);
    objs.push_back(go1);

    obj_center.x += 0.01;
    obj_center.y += 0.01;
    obj_center.z += 0.01;
    Geometry *geom2 = new Geometry();
    geom2->center = obj_center;
    geom2->radius = 0.01;
    GameObject *go2 = new GameObject(geom2, NULL, 234LL);
    objs.push_back(go2);

    obj_center.x += 0.01;
    obj_center.y += 0.01;
    obj_center.z += 0.01;
    Geometry *geom3 = new Geometry();
    geom3->center = obj_center;
    geom3->radius = 0.01;
    GameObject *go3 = new GameObject(geom3, NULL, 345LL);
    objs.push_back(go3);

    obj_center.x += 0.01;
    obj_center.y += 0.01;
    obj_center.z += 0.01;
    Geometry *geom4 = new Geometry();
    geom4->center = obj_center;
    geom4->radius = 0.01;
    GameObject *go4 = new GameObject(geom4, NULL, 456LL);
    objs.push_back(go4);

    glm::dvec3 min = {0.0, 0.0, 0.0}, max = {100.0, 100.0, 100.0};
    Octree *tree = new Octree(NULL, min, max, 0);

    try
    {
        tree->build(objs);
    }
    catch (...)
    {
        fail(test + "build exception");
    }

    is(tree->empty(), false, test + "expected empty");

    Octree *sub = tree->octants[0];
    is(sub != NULL, true, test + "expected subtree");
    while (sub->octants[0] != NULL)
        sub = sub->octants[0];
    is(sub->parent_index, 0, test + "expected parent index");
    is(sub->depth, Octree::MAX_DEPTH, test + "expected depth");

    delete tree;
    delete go4;
    delete go3;
    delete go2;
    delete go1;
}

int main(int argc, char **argv)
{
    plan(10);

    test_create_delete();
    test_build_empty_list();
    test_build();
    return exit_status();
}
