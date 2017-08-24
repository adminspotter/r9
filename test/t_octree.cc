#include "../server/classes/octree.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

TEST(OctreeTest, CreateDelete)
{
    Octree *tree = NULL;
    glm::dvec3 min = {0.0, 0.0, 0.0}, max = {100.0, 100.0, 100.0};

    ASSERT_NO_THROW(
        {
            tree = new Octree(NULL, min, max, 0);
        });

    ASSERT_EQ(tree->min_point, min);
    ASSERT_EQ(tree->max_point, max);

    ASSERT_NO_THROW(
        {
            delete tree;
        });
}

TEST(OctreeTest, BuildEmptyList)
{
    std::list<GameObject *> objs;
    glm::dvec3 min = {0.0, 0.0, 0.0}, max = {100.0, 100.0, 100.0};
    Octree *tree = new Octree(NULL, min, max, 0);

    ASSERT_NO_THROW(
        {
            tree->build(objs);
        });

    ASSERT_TRUE(tree->empty() == true);

    Octree *sub = tree->octants[0];
    ASSERT_TRUE(sub != NULL);
    while (sub->octants[0] != NULL)
        sub = sub->octants[0];
    ASSERT_EQ(sub->parent_index, 0);
    ASSERT_EQ(sub->depth, Octree::MIN_DEPTH);

    delete tree;
}

TEST(OctreeTest, Build)
{
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

    ASSERT_NO_THROW(
        {
            tree->build(objs);
        });

    ASSERT_TRUE(tree->empty() == false);

    Octree *sub = tree->octants[0];
    ASSERT_TRUE(sub != NULL);
    while (sub->octants[0] != NULL)
        sub = sub->octants[0];
    ASSERT_EQ(sub->parent_index, 0);
    std::cerr << "depth is " << sub->depth << std::endl;
    ASSERT_EQ(sub->depth, Octree::MAX_DEPTH);

    delete tree;
    delete go4;
    delete go3;
    delete go2;
    delete go1;
}
