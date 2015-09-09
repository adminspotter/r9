#include <string>

#include "libtest.h"

int test_func(int x)
{
    return x + 2;
}

std::string Thingie::what_are_you(void)
{
    std::string s = "a thingie!";
    return s;
}

Thingie *create_thingie(void)
{
    return new Thingie;
}

void destroy_thingie(Thingie *t)
{
    delete t;
}

std::string what(Thingie *t)
{
    return t->what_are_you();
}
