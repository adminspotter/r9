#ifndef __INC_MOCK_LISTENSOCK_H__
#define __INC_MOCK_LISTENSOCK_H__

#include <gmock/gmock.h>

#include "../server/classes/listensock.h"

class mock_listen_socket : public listen_socket
{
  public:
    mock_listen_socket(struct addrinfo *a) : listen_socket(a) {};
    virtual ~mock_listen_socket() {};

    MOCK_METHOD0(init, void(void));
    MOCK_METHOD0(start, void(void));
    MOCK_METHOD0(stop, void(void));

    MOCK_METHOD1(login_user, void(access_list&));
    MOCK_METHOD1(logout_user, void(access_list&));

    MOCK_METHOD3(do_login, void(uint64_t, Control *, access_list&));
    MOCK_METHOD1(get_userid, uint64_t(login_request&));
    MOCK_METHOD2(connect_user, void(base_user *, access_list&));

    MOCK_METHOD3(send_ack, void(Control *, uint8_t, uint8_t));

    /* Shouldn't need to mock the access_pool_worker static method,
     * since the fake start/stop methods won't do anything.
     */
};

#endif /* __INC_MOCK_LISTENSOCK_H__ */
