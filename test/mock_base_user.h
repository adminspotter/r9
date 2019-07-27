#ifndef __INC_MOCK_BASE_USER_H__
#define __INC_MOCK_BASE_USER_H__

#include "../server/classes/listensock.h"

class fake_base_user : public base_user
{
  public:
    fake_base_user(uint64_t a) : base_user(a) {};
    virtual ~fake_base_user() {};

    using base_user::parent;
};

#endif /* __INC_MOCK_BASE_USER_H__ */
