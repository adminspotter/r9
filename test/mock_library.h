#ifndef __INC_MOCK_LIBRARY_H__
#define __INC_MOCK_LIBRARY_H__

#include "../server/classes/library.h"

int symbol_count = 0;
void *symbol_result = NULL;
bool symbol_error = false;

class fake_Library : public Library
{
  public:
    fake_Library(const std::string& a) : Library() {};
    virtual ~fake_Library() {};

    virtual void open(void) {};
    virtual void *symbol(const char *a)
        {
            ++symbol_count;
            if (symbol_error)
                throw std::runtime_error("oh noes!");
            return symbol_result;
        };
    virtual void close(void) {};
};

#endif /* __INC_MOCK_LIBRARY_H__ */
