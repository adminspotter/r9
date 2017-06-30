#ifndef __INC_MOCK_LIBRARY_H__
#define __INC_MOCK_LIBRARY_H__

#include <gmock/gmock.h>

#include "../server/classes/library.h"

bool dlopen_error = false, dlclose_error = false, dlerror_error = false;
char *bad_news = "oh noes";

extern "C" {
    void *dlopen(const char *a, int b)
    {
        if (dlopen_error == true)
            return NULL;
        return (void *)a;
    }

    int dlclose(void *a)
    {
        if (dlclose_error == true)
            return 1;
        return 0;
    }

    char *dlerror(void)
    {
        if (dlerror_error == true)
            return bad_news;
        return NULL;
    }
}

class mock_Library : public Library
{
  public:
    mock_Library(const std::string& a) : Library(a) {};
    virtual ~mock_Library() {};

    MOCK_METHOD0(open, void(void));
    MOCK_METHOD1(symbol, void *(const char *));
    MOCK_METHOD0(close, void(void));
};

#endif /* __INC_MOCK_LIBRARY_H__ */
