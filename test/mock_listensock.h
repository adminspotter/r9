#ifndef __INC_MOCK_LISTENSOCK_H__
#define __INC_MOCK_LISTENSOCK_H__

#include "../server/classes/listensock.h"

class fake_listen_socket : public listen_socket
{
  public:
    fake_listen_socket(struct addrinfo *a) : listen_socket(a) {};
    virtual ~fake_listen_socket() {};

    std::string port_type(void) { return "fake"; };

    virtual void start(void) {};
    virtual void stop(void) {};

    virtual void login_user(access_list&) {};
    virtual void logout_user(access_list&) {};

    virtual void do_login(uint64_t, Control *, access_list&) {};
    virtual uint64_t get_userid(login_request&) { return 0LL; };
    virtual void connect_user(base_user *, access_list&) {};

    virtual void do_logout(base_user *) {};

    /* Shouldn't need to mock the access_pool_worker static method,
     * since the fake start/stop methods won't do anything.
     */
};

#endif /* __INC_MOCK_LISTENSOCK_H__ */
