#include "../server/classes/modules/console.h"

#include <string.h>
#include <unistd.h>

#include <iostream>

#include <gtest/gtest.h>

#define TMP_PATH  "./consoletest"

extern "C" Console *console_create(struct addrinfo *);
extern "C" void console_destroy(Console *);

#if HAVE_LIBWRAP
#include <tcpd.h>
#include "../server/classes/config_data.h"

extern config_data config;

int hosts_ctl(char *prefix, char *hostname, char *address, char *user)
{
    int ret = 1;

    std::clog << "into the hosts_ctl mock" << std::endl;
    if (strcmp(prefix, "prefix"))
        ret = 0;
    if (strcmp(hostname, "localhost"))
        ret = 0;
    if (strcmp(address, "127.0.0.1"))
        ret = 0;
    if (strcmp(user, STRING_UNKNOWN))
        ret = 0;
    return ret;
}
#endif /* HAVE_LIBWRAP */

void send_data_to(short port)
{
    struct addrinfo hints, *ai;
    int sock, len;
    char buf[1024];

    snprintf(buf, sizeof(buf), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo("localhost", buf, &hints, &ai);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << "socket error: " << err << " (" << errno << ')';
        throw std::runtime_error(s.str());
    }
    std::cerr << "sock opened" << std::endl;

    if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << "connect error: " << err << " (" << errno << ')';
        throw std::runtime_error(s.str());
    }
    std::cerr << "connected" << std::endl;

    /* First thing, we should get a prompt */
    len = recv(sock, buf, sizeof(buf), 0);
    ASSERT_GT(len, 0);
    std::cerr << "got the prompt" << std::endl;

    /* Send a fake command */
    len = snprintf(buf, sizeof(buf), "whoa\n");
    len = send(sock, buf, len, 0);
    ASSERT_GT(len, 0);
    std::cerr << "sent the fake command" << std::endl;

    len = recv(sock, buf, sizeof(buf), 0);
    ASSERT_GT(len, 0);
    ASSERT_TRUE(strstr(buf, "not implemented") != NULL);
    std::cerr << "got the message" << std::endl;

    /* Logout */
    len = snprintf(buf, sizeof(buf), "exit\n");
    len = send(sock, buf, len, 0);
    ASSERT_GT(len, 0);
    std::cerr << "sent the logout" << std::endl;

    /* We should get a message, then a closed connection */
    len = recv(sock, buf, sizeof(buf), 0);
    ASSERT_GT(len, 0);
    std::cerr << "got the message" << std::endl;

    while ((len = recv(sock, buf, sizeof(buf), 0)) != 0)
        std::cerr << "read " << len << " bytes" << std::endl;

    close(sock);
    std::cerr << "closed the sock" << std::endl;
    freeaddrinfo(ai);
    std::cerr << "freed the addrinfo" << std::endl;
}

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

TEST(ConsoleTest, CreateFactory)
{
    struct addrinfo hints, *ai;
    int ret;
    Console *con;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1234", &hints, &ai);

    ASSERT_NO_THROW(
        {
            con = console_create(ai);
        });
    ASSERT_STREQ(con->sa->ntop(), "127.0.0.1");
    ASSERT_EQ(con->sa->port(), 1234);
    ASSERT_GE(con->sock, 0);

    console_destroy(con);
    freeaddrinfo(ai);
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

    /* There are more checks within the wrap_request mock call.  If
     * any of them fails, the mock will return 0.  The non-libwrap
     * original function will always return 1.
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
    hints.ai_socktype = SOCK_STREAM;
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

    send_data_to(1237);

    con->stop();

    delete(con);
    freeaddrinfo(ai);
}
