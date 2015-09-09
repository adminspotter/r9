#include <string>

class Thingie
{
  public:
    Thingie() {};
    std::string what_are_you(void);
};

extern "C"
{
    int test_func(int);
    Thingie *create_thingie(void);
    void destroy_thingie(Thingie *);
    std::string what(Thingie *);
}
