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
    is(geom->mass == 1.0, true, test + "expected mass");
    is(geom->restitution == 0.75, true, test + "expected restitution");
    is(geom->friction == 0.25, true, test + "expected friction");

    delete geom;
}

void test_copy_constructor(void)
{
    std::string test = "default constructor: ";
    Geometry *geom1 = new Geometry(), *geom2 = NULL;

    geom1->center = {1.0, 2.0, 3.0};
    geom1->radius = 4.0;
    geom1->mass = 10.0;
    geom1->restitution = 0.2;
    geom1->friction = 0.8;

    geom2 = new Geometry(*geom1);

    is(geom2->center == geom1->center, true, test + "expected x value");
    is(geom2->radius == geom1->radius, true, test + "expected radius");
    is(geom2->mass == geom1->mass, true, test + "expected mass");
    is(geom2->restitution == geom1->restitution,
       true,
       test + "expected restitution");
    is(geom2->friction == geom1->friction, true, test + "expected friction");

    delete geom2;
    delete geom1;
}

int main(int argc, char **argv)
{
    plan(12);

    test_default_constructor();
    test_copy_constructor();
    return exit_status();
}
