#include <tap++.h>

using namespace TAP;

#include "../server/classes/modules/console.h"
#include "mock_server_globals.h"

#include <string.h>
#include <unistd.h>

#include <atomic>

#define TMP_PATH  "./consoletest"

extern "C" Console *console_create(Addrinfo *);
extern "C" void console_destroy(Console *);

#if HAVE_LIBWRAP
#include <tcpd.h>
#include "../server/classes/config_data.h"

extern config_data config;

class fake_Console : public Console
{
  public:
    fake_Console(Addrinfo *a) : Console(a) {}
    using Console::sa;
    using Console::sock;
};

int hosts_ctl(char *prefix, char *hostname, char *address, char *user)
{
    int ret = 1;

    if (strcmp(prefix, "prefix"))
        ret = 0;
    if (strcmp(hostname, "localhost"))
        ret = 0;
    if (strlen(address) && strcmp(address, "127.0.0.1"))
        ret = 0;
    if (strcmp(user, STRING_UNKNOWN))
        ret = 0;
    return ret;
}
#endif /* HAVE_LIBWRAP */

void send_data_to(short port)
{
    std::string func = "send_data_to: ";
    Addrinfo_un *ai = new Addrinfo_un(TMP_PATH);
    int sock, len;
    char buf[1024];

    if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        std::ostringstream s;
        char err[128];

        s << "socket error: " << strerror_r(errno, err, sizeof(err))
          << " (" << errno << ')';
        fail(func + s.str());
        goto CLEANUP;
    }

    if (connect(sock, ai->ai->ai_addr, ai->ai->ai_addrlen) < 0)
    {
        std::ostringstream s;
        char err[128];

        s << "connect error: " << strerror_r(errno, err, sizeof(err))
          << " (" << errno << ')';
        fail(func + s.str());
        goto CLEANUP;
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

  CLEANUP:
    close(sock);
    delete ai;
}

void test_create_inet(void)
{
    std::string test = "create inet console: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235", AF_INET);
    fake_Console *con;

    try
    {
        con = new fake_Console(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(con->sa->ntop(), "127.0.0.1", 10), 0, test + "expected address");
    is(con->sa->port(), 1235, test + "expected port");
    is(con->sock >= 0, true, test + "expected socket descriptor");

    delete con;
    delete ai;
}

void test_create_factory(void)
{
    std::string test = "create factory: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1234", AF_INET);
    Console *con;

    try
    {
        con = console_create(ai);
    }
    catch (...)
    {
        fail(test + "factory exception");
    }
    skip(3, test + "skipping protected field assertions");
    //is(strncmp(con->sa->ntop(), "127.0.0.1", 10), 0, test + "expected address");
    //is(con->sa->port(), 1234, test + "expected port");
    //is(con->sock >= 0, true, test + "expected socket descriptor");

    console_destroy(con);
    delete ai;
}

void test_wrap_request(void)
{
    std::string test = "wrap_request: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1236", AF_INET);
    Console *con;

    con = new Console(ai);

    Sockaddr *sa = ai->sockaddr();

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

    delete con;
    delete sa;
    delete ai;
}

void test_listener(void)
{
    std::string test = "listener: ";
    skip(6, test + "skipping");
    return;
    Addrinfo *ai = new Addrinfo_un(TMP_PATH);
    Console *con;

    unlink(TMP_PATH);
    try
    {
        con = new Console(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    con->listen_arg = (void *)con;
    main_loop_exit_flag = false;
    con->start(Console::console_listener);

    send_data_to(1237);

    main_loop_exit_flag = true;
    con->stop();

    unlink(TMP_PATH);
    delete con;
    delete ai;
}

int main(int argc, char **argv)
{
    plan(13);

    test_create_inet();
    test_create_factory();
    test_wrap_request();
    test_listener();
    return exit_status();
}
