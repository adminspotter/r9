#include <tap++.h>

using namespace TAP;

#include "../server/classes/listensock.h"
#include "../server/classes/config_data.h"

#include <stdlib.h>
#include <unistd.h>

#include "mock_base_user.h"
#include "mock_db.h"
#include "mock_server_globals.h"

class test_listen_socket : public listen_socket
{
  public:
    test_listen_socket(Addrinfo *a) : listen_socket(a) {};
    virtual ~test_listen_socket() {};
    using listen_socket::reap_timeout;
    using listen_socket::users;
    using listen_socket::send_pool;
};

void test_reaper_worker(void)
{
    std::string test = "reaper worker: ";
    config.send_threads = 1;
    config.access_threads = 1;

    Addrinfo *addr = new Addrinfo(DGRAM, "localhost", "8765");
    test_listen_socket *listen = new test_listen_socket(addr);
    listen->reap_timeout = 0;

    /* This user will be sent a ping */
    fake_base_user *bu = new fake_base_user(123LL);
    bu->parent = listen;
    bu->pending_logout = false;
    bu->timestamp = time(NULL) - listen_socket::PING_TIMEOUT - 1;
    listen->users[123LL] = bu;

    /* This user will be logged out entirely */
    fake_base_user *bu2 = new fake_base_user(1234LL);
    bu2->username = "bobbo";
    bu2->pending_logout = true;
    bu2->timestamp = time(NULL) - listen_socket::LINK_DEAD_TIMEOUT - 1;
    listen->users[1234LL] = bu2;

    listen->start();

    /* The user will be deleted absolutely last, so that's what we
     * need to wait for.
     */
    while (listen->users.size() > 1)
        ;

    listen->stop();

    is(listen->send_pool->queue_size() > 0, true,
       test + "expected send queue size");
    is(listen->users.size(), 0, test + "expected user list size");

    delete listen;
    delete addr;
}

void test_access_worker(void)
{
    std::string test = "access worker: ";
    config.send_threads = 1;
    config.access_threads = 1;

    database = new fake_DB("a", 0, "b", "c", "d");

    check_authentication_count = 0;
    check_authentication_result = 0LL;

    Addrinfo *addr = new Addrinfo(DGRAM, "localhost", "8765");
    test_listen_socket *listen = new test_listen_socket(addr);

    access_list req;

    memset(&req, 0, sizeof(access_list));
    req.buf.basic.type = TYPE_LOGREQ;
    listen->access_pool->push(req);

    req.buf.basic.type = TYPE_LGTREQ;
    req.what.logout.who = 123LL;
    listen->access_pool->push(req);

    is(listen->access_pool->queue_size(), 2,
       test + "expected access queue size");

    listen->start();

    while (listen->access_pool->queue_size() != 0)
        ;

    listen->stop();

    is(check_authentication_count > 0, true,
       test + "expected authentication count");
    is(listen->access_pool->queue_size(), 0,
       test + "expected access queue size");

    delete listen;
    delete addr;
    delete database;
}

int main(int argc, char **argv)
{
    plan(5);

    test_reaper_worker();
    test_access_worker();
    return exit_status();
}
