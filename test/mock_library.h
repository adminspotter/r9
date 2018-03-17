#ifndef __INC_MOCK_LIBRARY_H__
#define __INC_MOCK_LIBRARY_H__

#include <gmock/gmock.h>

#include "../server/classes/library.h"

class mock_Library : public Library
{
  public:
    mock_Library(const std::string& a) : Library() {};
    virtual ~mock_Library() {};

    MOCK_METHOD0(open, void(void));
    MOCK_METHOD1(symbol, void *(const char *));
    MOCK_METHOD0(close, void(void));
};

int symbol_count = 0;
void *symbol_result = NULL;

class fake_Library : public Library
{
  public:
    fake_Library(const std::string& a) : Library() {};
    virtual ~fake_Library() {};

    virtual void open(void) {};
    virtual void *symbol(const char *a)
        {
            ++symbol_count;
            return symbol_result;
        };
    virtual void close(void) {};
};

#endif /* __INC_MOCK_LIBRARY_H__ */
