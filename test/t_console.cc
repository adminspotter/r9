#include "../server/classes/modules/console.h"

#include <string.h>
#include <unistd.h>

#include <iostream>

#include <gtest/gtest.h>

#define TMP_PATH  "./consoletest"

#if HAVE_LIBWRAP
#include "../server/config_data.h"

extern config_data config;

int hosts_ctl(char *prefix, char *hostname, char *address, char *user)
{
    std::clog << "into the hosts_ctl mock" << std::endl;
/*    ASSERT_STREQ(prefix, "prefix");
    ASSERT_STREQ(hostname, "localhost");
    ASSERT_STREQ(address, "127.0.0.1");
    ASSERT_TRUE(user == NULL);*/
    return 1;
}
#endif /* HAVE_LIBWRAP */

TEST(ConsoleTest, CreateInet)
{
    struct addrinfo hints, *ai;
    int ret;
    Console *con;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    ASSERT_EQ(ret, 0);

    ASSERT_NO_THROW(
        {
            con = new Console(ai);
        });
    ASSERT_STREQ(con->sa->ntop(), "127.0.0.1");
    ASSERT_EQ(con->sa->port(), 1235);
    ASSERT_GE(con->sock, 0);

    delete(con);
    freeaddrinfo(ai);
}

TEST(ConsoleTest, CreateUnix)
{
    struct addrinfo ai;
    struct sockaddr_un sun;
    Console *con;

    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, TMP_PATH);

    ai.ai_family = AF_UNIX;
    ai.ai_socktype = SOCK_STREAM;
    ai.ai_protocol = 0;
    ai.ai_addrlen = sizeof(struct sockaddr_un);
    ai.ai_addr = (struct sockaddr *)&sun;

    ASSERT_NO_THROW(
        {
            con = new Console(&ai);
        });
    ASSERT_STREQ(con->sa->ntop(), TMP_PATH);
    ASSERT_EQ(con->sa->port(), UINT16_MAX);
    ASSERT_STREQ(con->sa->hostname(), "localhost");
    ASSERT_GE(con->sock, 0);

    delete(con);
    unlink(TMP_PATH);
}

TEST(ConsoleTest, WrapRequest)
{
    struct addrinfo hints, *ai;
    int ret;
    Console *con;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1236", &hints, &ai);

    con = new Console(ai);

    Sockaddr *sa = build_sockaddr(*ai->ai_addr);

#if HAVE_LIBWRAP
    /* We'll only actually use the config if we have the wrap_request
     * call.
     */
    config.log_prefix = "prefix";
#endif

    /* There are more assertions within the wrap_request mock call.
     * Both our mock, and the non-libwrap original function will
     * return 1.
     */
    ASSERT_EQ(con->wrap_request(sa), 1);

    delete(con);
    freeaddrinfo(ai);
}

TEST(ConsoleTest, InetListener)
{
    struct addrinfo hints, *ai;
    int ret;
    Console *con;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1237", &hints, &ai);
    ASSERT_EQ(ret, 0);

    ASSERT_NO_THROW(
        {
            con = new Console(ai);
        });
    ASSERT_STREQ(con->sa->ntop(), "127.0.0.1");
    ASSERT_EQ(con->sa->port(), 1237);
    ASSERT_GE(con->sock, 0);

    con->listen_arg = (void *)con;
    con->start(Console::console_listener);
    /* Should send some data to it */
    con->stop();

    delete(con);
    freeaddrinfo(ai);
}

TEST(ConsoleTest, UnixListener)
{
    struct addrinfo ai;
    struct sockaddr_un sun;
    Console *con;

    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, TMP_PATH);

    ai.ai_family = AF_UNIX;
    ai.ai_socktype = SOCK_STREAM;
    ai.ai_protocol = 0;
    ai.ai_addrlen = sizeof(struct sockaddr_un);
    ai.ai_addr = (struct sockaddr *)&sun;

    ASSERT_NO_THROW(
        {
            con = new Console(&ai);
        });
    ASSERT_STREQ(con->sa->ntop(), TMP_PATH);
    ASSERT_EQ(con->sa->port(), UINT16_MAX);
    ASSERT_STREQ(con->sa->hostname(), "localhost");
    ASSERT_GE(con->sock, 0);

    con->listen_arg = (void *)con;
    con->start(Console::console_listener);
    /* Should send some data to it */
    con->stop();

    delete(con);
    unlink(TMP_PATH);
}
