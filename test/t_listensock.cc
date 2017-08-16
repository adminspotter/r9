#include "../server/classes/listensock.h"

#include <gtest/gtest.h>

#include "mock_db.h"
#include "mock_zone.h"
#include "mock_server_globals.h"

using ::testing::_;
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

/* The listen_socket has a couple of pure-virtual methods, so we need
 * to derive something so we can actually test it.
 */
class test_listen_socket : public listen_socket
{
  public:
    test_listen_socket(struct addrinfo *a) : listen_socket(a) {};
    virtual ~test_listen_socket() {};

    virtual std::string port_type(void) override
        {
            return "test";
        };
};

TEST(BaseUserTest, CreateDelete)
{
    base_user *base = NULL;

    ASSERT_NO_THROW(
        {
            base = new base_user(0LL, NULL, NULL);
        });
    ASSERT_EQ(base->userid, 0LL);
    ASSERT_EQ(base->pending_logout, false);

    delete base;
}

TEST(BaseUserTest, LessThan)
{
    base_user *base1 = new base_user(123LL, NULL, NULL);
    base_user *base2 = new base_user(124LL, NULL, NULL);

    ASSERT_TRUE(*base1 < *base2);

    delete base2;
    delete base1;
}

TEST(BaseUserTest, EqualTo)
{
    base_user *base1 = new base_user(123LL, NULL, NULL);
    base_user *base2 = new base_user(124LL, NULL, NULL);

    ASSERT_FALSE(*base1 == *base2);

    delete base2;
    delete base1;
}

TEST(BaseUserTest, Assignment)
{
    base_user *base1 = new base_user(123LL, NULL, NULL);
    base_user *base2 = new base_user(124LL, NULL, NULL);

    ASSERT_FALSE(*base1 == *base2);

    *base1 = *base2;

    ASSERT_TRUE(*base1 == *base2);

    delete base2;
    delete base1;
}

TEST(ListenSocketTest, CreateDelete)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;

    ASSERT_NO_THROW(
        {
            listen = new test_listen_socket(addr);
        });
    /* Thread pools should not be started */
    ASSERT_TRUE(listen->send_pool->pool_size() == 0);
    ASSERT_TRUE(listen->access_pool->pool_size() == 0);

    delete listen;
}

TEST(ListenSocketTest, StartStop)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;

    create_count = cancel_count = join_count = 0;
    listen = new test_listen_socket(addr);

    create_error = true;
    ASSERT_THROW(
        {
            listen->start();
        },
        std::runtime_error);
    ASSERT_EQ(create_count, 1);

    create_error = false;
    create_count = 0;
    ASSERT_NO_THROW(
        {
            listen->start();
        });
    ASSERT_GT(create_count, 1);

    cancel_error = true;
    ASSERT_THROW(
        {
            listen->stop();
        },
        std::runtime_error);
    try { listen->stop(); }
    catch (std::runtime_error& e)
    {
        ASSERT_TRUE(strstr(e.what(), "couldn't cancel reaper") != NULL);
    }

    cancel_error = false;
    join_error = true;
    ASSERT_THROW(
        {
            listen->stop();
        },
        std::runtime_error);
    try { listen->stop(); }
    catch (std::runtime_error& e)
    {
        ASSERT_TRUE(strstr(e.what(), "couldn't join reaper") != NULL);
    }

    join_error = false;
    ASSERT_NO_THROW(
        {
            listen->stop();
        });

    delete listen;
}

TEST(ListenSocketTest, GetUserid)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), check_authentication(_, _))
        .WillOnce(Return(0LL));

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new test_listen_socket(addr);

    uint64_t userid = listen->get_userid(log);

    ASSERT_EQ(userid, 0LL);
    ASSERT_FALSE(strncmp(log.password, "pass", sizeof(log.password)) == 0);

    delete listen;
    delete database;
}

TEST(ListenSocketTest, CheckAccessNoAccess)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), get_character_objectid(_, _))
        .WillOnce(Return(1234LL));
    EXPECT_CALL(*((mock_DB *)database), check_authorization(_, _))
        .WillOnce(Return(ACCESS_NONE));

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);
    strncpy(log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new test_listen_socket(addr);

    base_user *bu = listen->check_access(123LL, log);

    ASSERT_TRUE(bu == NULL);

    delete listen;
    delete database;
}

TEST(ListenSocketTest, CheckAccess)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), get_character_objectid(_, _))
        .WillOnce(Return(1234LL));
    EXPECT_CALL(*((mock_DB *)database), check_authorization(_, _))
        .WillOnce(Return(ACCESS_MOVE));
    EXPECT_CALL(*((mock_DB *)database), get_server_objects(_));

    GameObject *go = new GameObject(NULL, NULL, 1234LL);

    zone = new Zone(1000, 1, database);
    zone->game_objects[1234LL] = go;

    login_request log;

    memset(&log, 0, sizeof(login_request));
    strncpy(log.username, "howdy", 6);
    strncpy(log.password, "pass", 5);
    strncpy(log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    test_listen_socket *listen = new test_listen_socket(addr);

    base_user *bu = listen->check_access(123LL, log);

    ASSERT_TRUE(bu != NULL);

    delete bu;
    delete listen;
    delete zone;
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
    listen_socket *listen = new test_listen_socket(addr);

    ASSERT_TRUE(listen->users.size() == 0);

    listen->login_user(access);

    ASSERT_TRUE(listen->users.size() == 0);
    ASSERT_FALSE(strncmp(access.buf.log.password,
                         "pass",
                         sizeof(access.buf.log.password)) == 0);

    delete listen;
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
    test_listen_socket *listen = new test_listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);
    bu->pending_logout = true;
    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->users.size() == 1);

    listen->login_user(access);

    ASSERT_TRUE(listen->users.size() == 1);
    ASSERT_EQ(bu->pending_logout, false);

    delete listen;
    delete database;
}

TEST(ListenSocketTest, Login)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), check_authentication(_, _))
        .WillOnce(Return(123LL));
    EXPECT_CALL(*((mock_DB *)database), get_character_objectid(_, _))
        .WillOnce(Return(1234LL));
    EXPECT_CALL(*((mock_DB *)database), check_authorization(_, _))
        .WillOnce(Return(ACCESS_MOVE));
    EXPECT_CALL(*((mock_DB *)database), get_server_objects(_));

    GameObject *go = new GameObject(NULL, NULL, 1234LL);

    zone = new Zone(1000, 1, database);
    zone->game_objects[1234LL] = go;

    access_list access;

    memset(&access.buf, 0, sizeof(packet));
    strncpy(access.buf.log.username, "howdy", 6);
    strncpy(access.buf.log.password, "pass", 5);
    strncpy(access.buf.log.charname, "blah", 5);

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new test_listen_socket(addr);


    ASSERT_TRUE(listen->users.size() == 0);

    listen->login_user(access);

    ASSERT_TRUE(listen->users.size() == 1);
    ASSERT_EQ(listen->users[123LL]->auth_level, ACCESS_MOVE);

    delete zone;
    delete listen;
    delete (mock_DB *)database;
}

TEST(ListenSocketTest, Logout)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;
    access_list access;

    listen = new test_listen_socket(addr);

    Control *control = new Control(123LL, NULL);
    base_user *bu = new base_user(123LL, control, listen);

    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);

    /* The logout method never looks at the packet (access.buf), so we
     * won't bother setting anything in it.
     */
    access.what.logout.who = 123LL;

    listen->logout_user(access);

    ASSERT_TRUE(bu->pending_logout == true);
    ASSERT_TRUE(listen->send_pool->queue_size() > 0);

    delete listen;
    delete control;
}

TEST(ListenSocketTest, ConnectUser)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new test_listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    access_list access;

    memset(&access.buf, 0, sizeof(packet));

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);
    ASSERT_TRUE(listen->users.size() == 0);

    listen->connect_user(bu, access);

    ASSERT_TRUE(listen->send_pool->queue_size() > 0);
    ASSERT_TRUE(listen->users.size() == 1);

    delete listen;
}

TEST(ListenSocketTest, DisconnectUser)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new test_listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->users.size() == 1);

    listen->disconnect_user(bu);

    ASSERT_TRUE(listen->users.size() == 0);

    delete listen;
}

TEST(BaseUserTest, SendPing)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;

    listen = new test_listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);

    bu->send_ping();

    ASSERT_TRUE(listen->send_pool->queue_size() > 0);

    delete listen;
}

TEST(BaseUserTest, SendAck)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;

    listen = new test_listen_socket(addr);

    base_user *bu = new base_user(123LL, NULL, listen);

    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);

    bu->send_ack(1, 2);

    ASSERT_TRUE(listen->send_pool->queue_size() > 0);

    delete listen;
}
