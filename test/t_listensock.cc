#include <tap++.h>

using namespace TAP;

#include "../server/classes/listensock.h"

#include <gtest/gtest.h>

#include "mock_db.h"
#include "mock_server_globals.h"
#include "mock_zone.h"

using ::testing::_;
using ::testing::A;
using ::testing::Return;
using ::testing::Invoke;

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

TEST(ListenSocketTest, GetUserid)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database),
                check_authentication(_, A<const std::string&>()))
        .WillOnce(Return(0LL));

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    uint64_t userid = listen->get_userid(log);

    ASSERT_EQ(userid, 0LL);
    ASSERT_FALSE(strncmp(log.password, "pass", sizeof(log.password)) == 0);

    delete listen;
    freeaddrinfo(addr);
    delete database;
}

TEST(ListenSocketTest, CheckAccessNoAccess)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database),
                check_authorization(_, A<const std::string&>()))
        .WillOnce(Return(ACCESS_NONE));

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);
    strncpy(log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = listen->check_access(123LL, log);

    ASSERT_TRUE(bu == NULL);

    delete listen;
    freeaddrinfo(addr);
    delete database;
}

TEST(ListenSocketTest, CheckAccess)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), get_character_objectid(_, _))
        .WillOnce(Return(1234LL));
    EXPECT_CALL(*((mock_DB *)database),
                check_authorization(_, A<const std::string&>()))
        .WillOnce(Return(ACCESS_MOVE));
    EXPECT_CALL(*((mock_DB *)database),
                get_characterid(_, _))
        .WillOnce(Return(12345678LL));
    EXPECT_CALL(*((mock_DB *)database),
                get_player_server_skills(_, _, _));
    EXPECT_CALL(*((mock_DB *)database), get_server_objects(_));

    zone = new mock_Zone(1000, 1, database);

    EXPECT_CALL(*((mock_Zone *)zone), send_nearby_objects(_));

    GameObject *go = new GameObject(NULL, NULL, 1234LL);

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);
    strncpy(log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = listen->check_access(123LL, log);

    ASSERT_TRUE(bu != NULL);

    delete bu;
    delete listen;
    freeaddrinfo(addr);
    delete (mock_Zone *)zone;
    delete (mock_DB *)database;
}

TEST(ListenSocketTest, LoginNoUser)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), check_authentication(_, _))
        .WillOnce(Return(0LL));

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);
    strncpy(access.buf.log.charname, "bob", 4);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    ASSERT_TRUE(listen->users.size() == 0);

    listen->login_user(access);

    ASSERT_TRUE(listen->users.size() == 0);
    ASSERT_FALSE(strncmp(access.buf.log.password,
                         "pass",
                         sizeof(access.buf.log.password)) == 0);

    delete listen;
    freeaddrinfo(addr);
    delete database;
}

TEST(ListenSocketTest, LoginAlready)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), check_authentication(_, _))
        .WillOnce(Return(123LL));

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);
    bu->pending_logout = true;
    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->users.size() == 1);

    listen->login_user(access);

    ASSERT_TRUE(listen->users.size() == 1);
    ASSERT_EQ(bu->pending_logout, false);

    delete listen;
    freeaddrinfo(addr);
    delete database;
}

TEST(ListenSocketTest, LoginNoAccess)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), check_authentication(_, _))
        .WillOnce(Return(123LL));
    EXPECT_CALL(*((mock_DB *)database),
                check_authorization(_, A<const std::string&>()))
        .WillOnce(Return(ACCESS_NONE));

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);
    strncpy(access.buf.log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    ASSERT_TRUE(listen->users.size() == 0);

    listen->login_user(access);

    ASSERT_TRUE(listen->users.size() == 0);

    delete listen;
    freeaddrinfo(addr);
    delete (mock_DB *)database;
}

TEST(ListenSocketTest, Login)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), check_authentication(_, _))
        .WillOnce(Return(123LL));
    EXPECT_CALL(*((mock_DB *)database), get_character_objectid(_, _))
        .WillOnce(Return(1234LL));
    EXPECT_CALL(*((mock_DB *)database),
                check_authorization(_, A<const std::string&>()))
        .WillOnce(Return(ACCESS_MOVE));
    EXPECT_CALL(*((mock_DB *)database), get_server_objects(_));

    GameObject *go = new GameObject(NULL, NULL, 1234LL);

    zone = new mock_Zone(1000, 1, database);
    EXPECT_CALL(*((mock_Zone *)zone), send_nearby_objects(_));

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);
    strncpy(access.buf.log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    ASSERT_TRUE(listen->users.size() == 0);

    listen->login_user(access);

    ASSERT_TRUE(listen->users.size() == 1);
    ASSERT_EQ(listen->users[123LL]->auth_level, ACCESS_MOVE);

    delete (mock_Zone *)zone;
    delete listen;
    freeaddrinfo(addr);
    delete (mock_DB *)database;
}

TEST(ListenSocketTest, Logout)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);

    listen->logout_user(123LL);

    ASSERT_TRUE(bu->pending_logout == true);
    ASSERT_TRUE(listen->send_pool->queue_size() > 0);

    delete listen;
    freeaddrinfo(addr);
}

TEST(ListenSocketTest, ConnectUser)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    access_list access;

    memset(&access.buf, 0, sizeof(packet));

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);
    ASSERT_TRUE(listen->users.size() == 0);

    listen->connect_user(bu, access);

    ASSERT_TRUE(listen->send_pool->queue_size() > 0);
    ASSERT_TRUE(listen->users.size() == 1);

    delete listen;
    freeaddrinfo(addr);
}

TEST(ListenSocketTest, DisconnectUser)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->users.size() == 1);

    listen->disconnect_user(bu);

    ASSERT_TRUE(listen->users.size() == 0);

    delete listen;
    freeaddrinfo(addr);
}

TEST(BaseUserTest, SendPing)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);

    bu->send_ping();

    ASSERT_TRUE(listen->send_pool->queue_size() > 0);

    delete listen;
    freeaddrinfo(addr);
}

TEST(BaseUserTest, SendAck)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);

    bu->send_ack(1, 2);

    ASSERT_TRUE(listen->send_pool->queue_size() > 0);

    delete listen;
    freeaddrinfo(addr);
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    plan(27);

    int gtests = RUN_ALL_TESTS();

    test_base_user_create_delete();
    test_base_user_less_than();
    test_base_user_equal_to();
    test_base_user_assignment();
    test_base_user_disconnect_on_destroy();

    test_listen_socket_create_delete();
    test_listen_socket_port_type();
    test_listen_socket_start_stop();
    test_listen_socket_handle_ack();
    test_listen_socket_handle_action();
    test_listen_socket_handle_logout();
    return gtests & exit_status();
}
