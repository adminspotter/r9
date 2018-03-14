#include <tap++.h>

using namespace TAP;

#include "../server/classes/listensock.h"

#include "mock_db.h"
#include "mock_server_globals.h"
#include "mock_zone.h"

bool create_error = false, cancel_error = false, join_error = false;
int create_count, cancel_count, join_count;

int pthread_create(pthread_t *a, const pthread_attr_t *b,
                   void *(*c)(void *), void *d)
{
    ++create_count;
    if (create_error == true)
        return EINVAL;
    return 0;
}

int pthread_cancel(pthread_t a)
{
    ++cancel_count;
    if (cancel_error == true)
        return EINVAL;
    return 0;
}

int pthread_join(pthread_t a, void **b)
{
    ++join_count;
    if (join_error == true)
        return EINVAL;
    return 0;
}

struct addrinfo *create_addrinfo(void)
{
    struct addrinfo hints, *addr = NULL;
    static int port = 9876;
    char port_str[6];
    int ret;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    snprintf(port_str, sizeof(port_str), "%d", port--);
    if ((ret = getaddrinfo(NULL, port_str, &hints, &addr)) != 0)
    {
        std::cerr << gai_strerror(ret) << std::endl;
        throw std::runtime_error("getaddrinfo broke");
    }

    return addr;
}

int fake_server_objects(std::map<uint64_t, GameObject *>& gom)
{
    glm::dvec3 pos(100.0, 100.0, 100.0);

    gom[1234LL] = new GameObject(NULL, NULL, 1234LL);
    gom[1234LL]->position = pos;

    gom[1235LL] = new GameObject(NULL, NULL, 1235LL);
    pos.x = 125.0;
    gom[1235LL]->position = pos;

    return 2;
}

void test_base_user_create_delete(void)
{
    std::string test = "base_user create/delete: ";
    base_user *base = NULL;

    try
    {
        base = new base_user(0LL, NULL, NULL);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(base->userid, 0LL, test + "expected userid");
    is(base->pending_logout, false, test + "expected logout");

    delete base;
}

void test_base_user_less_than(void)
{
    std::string test = "base_user less: ";
    base_user *base1 = new base_user(123LL, NULL, NULL);
    base_user *base2 = new base_user(124LL, NULL, NULL);

    is(*base1 < *base2, true, test + "object less than object");

    delete base2;
    delete base1;
}

void test_base_user_equal_to(void)
{
    std::string test = "base_user equal: ";
    base_user *base1 = new base_user(123LL, NULL, NULL);
    base_user *base2 = new base_user(124LL, NULL, NULL);

    is(*base1 == *base2, false, test + "objects not equal");

    delete base2;
    delete base1;
}

void test_base_user_assignment(void)
{
    std::string test = "base_user assign: ";
    base_user *base1 = new base_user(123LL, NULL, NULL);
    base_user *base2 = new base_user(124LL, NULL, NULL);

    is(*base1 == *base2, false, test + "objects not equal");

    *base1 = *base2;

    is(*base1 == *base2, true, test + "objects equal");

    delete base2;
    delete base1;
}

void test_base_user_disconnect_on_destroy(void)
{
    std::string test = "base_user disconnect: ";
    base_user *base = NULL;
    GameObject *go = new GameObject(NULL, NULL, 1234LL);

    base = new base_user(123LL, go, NULL);
    is(go->natures.size(), 0, test + "expected natures size");

    delete base;
    is(go->natures.size(), 2, test + "expected natures size");
    isnt(go->natures.find("invisible"), go->natures.end(),
         test + "added invisible nature");
    isnt(go->natures.find("non-interactive"), go->natures.end(),
         test + "added non-interactive nature");
    delete go;
}

void test_base_user_send_ping(void)
{
    std::string test = "base_user send_ping: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    is(listen->send_pool->queue_size(), 0, test + "expected send queue size");

    bu->send_ping();

    isnt(listen->send_pool->queue_size(), 0, test + "expected send queue size");

    delete listen;
    freeaddrinfo(addr);
}

void test_base_user_send_ack(void)
{
    std::string test = "base_user send_ack: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    is(listen->send_pool->queue_size(), 0, test + "expected send queue size");

    bu->send_ack(1, 2);

    isnt(listen->send_pool->queue_size(), 0, test + "expected send queue size");

    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_create_delete(void)
{
    std::string test = "listen_socket create/delete: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;

    try
    {
        listen = new listen_socket(addr);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    /* Thread pools should not be started */
    is(listen->send_pool->pool_size(), 0, test + "expected send size");
    is(listen->access_pool->pool_size(), 0, test + "expected access size");

    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_port_type(void)
{
    std::string test = "listen_socket port: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    is(listen->port_type() == "listen", true, test + "expected port type");

    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_start_stop(void)
{
    std::string test = "listen_socket start/stop: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;

    create_count = cancel_count = join_count = 0;
    listen = new listen_socket(addr);

    create_error = true;
    try
    {
        listen->start();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't create reaper"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    is(create_count, 1, test + "expected creates");

    create_error = false;
    create_count = 0;
    try
    {
        listen->start();
    }
    catch (...)
    {
        fail(test + "start exception");
    }
    is(create_count > 1, true, test + "expected creates");

    cancel_error = true;
    try
    {
        listen->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't cancel reaper"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    cancel_error = false;
    join_error = true;
    try
    {
        listen->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't join reaper"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    join_error = false;
    try
    {
        listen->stop();
    }
    catch (...)
    {
        fail(test + "stop exception");
    }

    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_handle_ack(void)
{
    std::string test = "listen_socket handle_ack: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);
    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[bu->userid] = bu;

    bu->timestamp = 0;

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_ACKPKT;

    listen_socket::handle_ack(listen, p, bu, NULL);

    isnt(bu->timestamp, 0, test + "expected timestamp");

    delete bu;
    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_handle_action(void)
{
    std::string test = "listen_socket handle_action: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);
    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[bu->userid] = bu;

    bu->timestamp = 0;

    /* Creating an actual ActionPool will be prohibitive, since it
     * requires so many other objects to make it go.  All we need here
     * is the push() method, and the queue_size() method to make sure
     * our function is doing what we expect it to.
     */
    action_pool = (ActionPool *)new ThreadPool<packet_list>("test", 1);

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_ACTREQ;

    is(action_pool->queue_size(), 0, test + "expected queue size");

    listen_socket::handle_action(listen, p, bu, NULL);

    isnt(bu->timestamp, 0, test + "expected timestamp");
    isnt(action_pool->queue_size(), 0, test + "expected queue size");

    delete (ThreadPool<packet_list> *)action_pool;
    delete bu;
    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_handle_logout(void)
{
    std::string test = "listen_socket handle_logout: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);
    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[bu->userid] = bu;

    bu->timestamp = 0;

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_LGTREQ;

    is(listen->access_pool->queue_size(), 0, test + "expected queue size");

    listen_socket::handle_logout(listen, p, bu, NULL);

    isnt(bu->timestamp, 0, test + "expected timestamp");
    isnt(listen->access_pool->queue_size(), 0, test + "expected queue size");
    access_list al;
    memset(&al, 0, sizeof(access_list));
    listen->access_pool->pop(&al);
    is(al.buf.basic.type, TYPE_LGTREQ, test + "expected logout packet");
    is(al.what.logout.who, 123LL, test + "expected userid");

    delete bu;
    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_get_userid(void)
{
    std::string test = "listen_socket get_userid: ";
    database = new fake_DB("a", "b", "c", "d");

    check_authentication_count = 0;
    check_authentication_result = 12345LL;

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    uint64_t userid = listen->get_userid(log);

    is(check_authentication_count, 1, test + "expected database call");
    is(userid, 12345LL, test + "expected userid");
    is(strncmp(log.password, "", sizeof(log.password)), 0,
       test + "password is cleared");

    delete listen;
    freeaddrinfo(addr);
    delete database;
}

void test_listen_socket_no_access(void)
{
    std::string test = "listen_socket no access: ";
    database = new fake_DB("a", "b", "c", "d");

    check_authorization_count = 0;
    check_authorization_result = ACCESS_NONE;

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);
    strncpy(log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = listen->check_access(123LL, log);

    is(check_authorization_count, 1, test + "expected database call");
    is(bu == NULL, true, test + "expected result");

    delete listen;
    freeaddrinfo(addr);
    delete (fake_DB *)database;
}

void test_listen_socket_check_access(void)
{
    std::string test = "listen_socket check_access: ";
    database = new fake_DB("a", "b", "c", "d");

    check_authorization_count = 0;
    check_authorization_result = ACCESS_MOVE;

    get_character_objectid_count = 0;
    get_character_objectid_result = 1234LL;

    get_characterid_count = 0;
    get_characterid_result = 12345678LL;

    get_player_server_skills_count = 0;

    zone = new fake_Zone(1000, 1, database);

    send_nearby_objects_count = 0;

    GameObject *go = new GameObject(NULL, NULL, 1234LL);

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);
    strncpy(log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = listen->check_access(123LL, log);

    is(check_authorization_count, 1, test + "expected authorization call");
    is(get_character_objectid_count, 1, test + "expected objectid call");
    is(get_characterid_count, 1, test + "expected characterid call");
    is(get_player_server_skills_count, 1, test + "expected skills call");
    is(send_nearby_objects_count, 1, test + "expected nearby objects call");
    is(bu != NULL, true, test + "got a good base_user");

    delete bu;
    delete listen;
    freeaddrinfo(addr);
    delete (fake_Zone *)zone;
    delete (fake_DB *)database;
}

void test_listen_socket_login_no_user(void)
{
    std::string test = "listen_socket no user: ";
    database = new fake_DB("a", "b", "c", "d");

    check_authentication_count = 0;
    check_authentication_result = 0LL;

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);
    strncpy(access.buf.log.charname, "bob", 4);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    is(listen->users.size(), 0, test + "expected user list size");

    listen->login_user(access);

    is(check_authentication_count, 1, test + "expected authentication call");
    is(listen->users.size(), 0, test + "expected user list size");
    is(strncmp(access.buf.log.password, "", sizeof(access.buf.log.password)), 0,
       test + "password is cleared");

    delete listen;
    freeaddrinfo(addr);
    delete database;
}

void test_listen_socket_login_already(void)
{
    std::string test = "listen_socket already logged in: ";
    database = new fake_DB("a", "b", "c", "d");

    check_authentication_count = 0;
    check_authentication_result = 123LL;

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);
    bu->pending_logout = true;
    listen->users[123LL] = bu;

    is(listen->users.size(), 1, test + "expected user list size");

    listen->login_user(access);

    is(check_authentication_count, 1, test + "expected authentication call");
    is(listen->users.size(), 1, test + "expected user list size");
    is(bu->pending_logout, false, test + "user not logging out");

    delete listen;
    freeaddrinfo(addr);
    delete database;
}

void test_listen_socket_login_no_access(void)
{
    std::string test = "listen_socket no access: ";
    database = new fake_DB("a", "b", "c", "d");

    check_authentication_count = 0;
    check_authentication_result = 123LL;

    check_authorization_count = 0;
    check_authorization_result = ACCESS_NONE;

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);
    strncpy(access.buf.log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    is(listen->users.size(), 0, test + "expected user list size");

    listen->login_user(access);

    is(check_authentication_count, 1, test + "expected authentication call");
    is(check_authorization_count, 1, test + "expected authorization call");
    is(listen->users.size(), 0, test + "expected user list size");

    delete listen;
    freeaddrinfo(addr);
    delete (fake_DB *)database;
}

void test_listen_socket_login(void)
{
    std::string test = "listen_socket login: ";
    database = new fake_DB("a", "b", "c", "d");

    check_authentication_count = 0;
    check_authentication_result = 123LL;

    check_authorization_count = 0;
    check_authorization_result = ACCESS_MOVE;

    get_character_objectid_count = 0;
    get_character_objectid_result = 1234LL;

    get_characterid_count = 0;

    get_player_server_skills_count = 0;

    GameObject *go = new GameObject(NULL, NULL, 1234LL);

    zone = new fake_Zone(1000, 1, database);

    send_nearby_objects_count = 0;

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);
    strncpy(access.buf.log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    is(listen->users.size(), 0, test + "expected user list size");

    listen->login_user(access);

    is(check_authorization_count, 1, test + "expected authorization call");
    is(get_character_objectid_count, 1, test + "expected objectid call");
    is(get_characterid_count, 1, test + "expected characterid call");
    is(get_player_server_skills_count, 1, test + "expected skills call");
    is(send_nearby_objects_count, 1, test + "expected nearby objects call");
    is(listen->users.size(), 1, test + "expected user list size");
    is(listen->users[123LL]->auth_level, ACCESS_MOVE,
       test + "expected auth level");

    delete (fake_Zone *)zone;
    delete listen;
    freeaddrinfo(addr);
    delete (fake_DB *)database;
}

void test_listen_socket_logout(void)
{
    std::string test = "listen_socket logout: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    is(listen->send_pool->queue_size(), 0, test + "expected send queue size");

    listen->logout_user(123LL);

    is(bu->pending_logout, true, test + "user set to logout");
    isnt(listen->send_pool->queue_size(), 0, test + "expected send queue size");

    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_connect_user(void)
{
    std::string test = "listen_socket connect_user: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    access_list access;

    memset(&access.buf, 0, sizeof(packet));

    is(listen->send_pool->queue_size(), 0, test + "expected send queue size");
    is(listen->users.size(), 0, test + "expected user list size");

    listen->connect_user(bu, access);

    isnt(listen->send_pool->queue_size(), 0, test + "expected send queue size");
    is(listen->users.size(), 1, test + "expected user list size");

    delete listen;
    freeaddrinfo(addr);
}

void test_listen_socket_disconnect_user(void)
{
    std::string test = "listen_socket disconnect_user: ";
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    is(listen->users.size(), 1, test + "expected user list size");

    listen->disconnect_user(bu);

    is(listen->users.size(), 0, test + "expected user list size");

    delete listen;
    freeaddrinfo(addr);
}

int main(int argc, char **argv)
{
    plan(71);

    test_base_user_create_delete();
    test_base_user_less_than();
    test_base_user_equal_to();
    test_base_user_assignment();
    test_base_user_disconnect_on_destroy();
    test_base_user_send_ping();
    test_base_user_send_ack();

    test_listen_socket_create_delete();
    test_listen_socket_port_type();
    test_listen_socket_start_stop();
    test_listen_socket_handle_ack();
    test_listen_socket_handle_action();
    test_listen_socket_handle_logout();
    test_listen_socket_get_userid();
    test_listen_socket_no_access();
    test_listen_socket_check_access();
    test_listen_socket_login_no_user();
    test_listen_socket_login_already();
    test_listen_socket_login_no_access();
    test_listen_socket_login();
    test_listen_socket_logout();
    test_listen_socket_connect_user();
    test_listen_socket_disconnect_user();
    return exit_status();
}
