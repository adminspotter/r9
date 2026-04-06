#include <tap++.h>

using namespace TAP;

#include "../server/classes/geometry.h"

void test_default_constructor(void)
{
    std::string test = "default constructor: ";
    Geometry *geom = NULL;

    try
    {
        geom = new Geometry();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(geom->center.x == 0.0, true, test + "expected x value");
    is(geom->center.y == 0.0, true, test + "expected y value");
    is(geom->center.z == 0.0, true, test + "expected z value");
    is(geom->radius == 0.5, true, test + "expected radius");

    delete geom;
}

void test_copy_constructor(void)
{
    std::string test = "default constructor: ";
    Geometry *geom1 = new Geometry(), *geom2 = NULL;

    geom1->center = {1.0, 2.0, 3.0};
    geom1->radius = 4.0;

    geom2 = new Geometry(*geom1);

    is(geom2->center == geom1->center, true, test + "expected x value");
    is(geom2->radius == geom1->radius, true, test + "expected radius");

    delete geom2;
    delete geom1;
}

int main(int argc, char **argv)
{
    plan(6);

    test_default_constructor();
    test_copy_constructor();
    return exit_status();
}
