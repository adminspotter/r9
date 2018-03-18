#include <tap++.h>

using namespace TAP;

#include "../server/classes/modules/console.h"

#include <string.h>
#include <unistd.h>

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
    std::string func = "send_data_to: ";
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

    if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << "connect error: " << err << " (" << errno << ')';
        throw std::runtime_error(s.str());
    }

    /* First thing, we should get a prompt */
    len = recv(sock, buf, sizeof(buf), 0);
    is(len >= 0, true, func + "got the prompt");

    /* Send a fake command */
    len = snprintf(buf, sizeof(buf), "whoa\n");
    len = send(sock, buf, len, 0);
    is(len >= 0, true, func + "sent the fake command");

    len = recv(sock, buf, sizeof(buf), 0);
    is(len >= 0, true, func + "got a response");
    is(strstr(buf, "not implemented") != NULL, true, func + "got the message");

    /* Logout */
    len = snprintf(buf, sizeof(buf), "exit\n");
    len = send(sock, buf, len, 0);
    is(len >= 0, true, func + "sent the logout");

    /* We should get a message, then a closed connection */
    len = recv(sock, buf, sizeof(buf), 0);
    is(len >= 0, true, func + "got the message");

    while ((len = recv(sock, buf, sizeof(buf), 0)) != 0)
        ;

    close(sock);
    freeaddrinfo(ai);
}

void test_create_inet(void)
{
    std::string test = "create inet console: ";
    struct addrinfo hints, *ai;
    int ret;
    Console *con;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    try
    {
        con = new Console(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(con->sa->ntop(), "127.0.0.1", 10), 0, test + "expected address");
    is(con->sa->port(), 1235, test + "expected port");
    is(con->sock >= 0, true, test + "expected socket descriptor");

    delete(con);
    freeaddrinfo(ai);
}

void test_create_factory(void)
{
    std::string test = "create factory: ";
    struct addrinfo hints, *ai;
    int ret;
    Console *con;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1234", &hints, &ai);

    try
    {
        con = console_create(ai);
    }
    catch (...)
    {
        fail(test + "factory exception");
    }
    is(strncmp(con->sa->ntop(), "127.0.0.1", 10), 0, test + "expected address");
    is(con->sa->port(), 1234, test + "expected port");
    is(con->sock >= 0, true, test + "expected socket descriptor");

    console_destroy(con);
    freeaddrinfo(ai);
}

void test_wrap_request(void)
{
    std::string test = "wrap_request: ";
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
    is(con->wrap_request(sa), 1, test + "expected result");

    delete(con);
    freeaddrinfo(ai);
}

void test_inet_listener(void)
{
    std::string test = "inet listener:";
    struct addrinfo hints, *ai;
    int ret;
    Console *con;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1237", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    try
    {
        con = new Console(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(con->sa->ntop(), "127.0.0.1", 10), 0, test + "expected address");
    is(con->sa->port(), 1237, test + "expected port");
    is(con->sock >= 0, true, test + "expected socket descriptor");

    con->listen_arg = (void *)con;
    con->start(Console::console_listener);

    send_data_to(1237);

    con->stop();

    delete(con);
    freeaddrinfo(ai);
}

int main(int argc, char **argv)
{
    plan(18);

    test_create_inet();
    test_create_factory();
    test_wrap_request();
    test_inet_listener();
    return exit_status();
}
