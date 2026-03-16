#include <tap++.h>

using namespace TAP;

#include "../server/classes/basesock.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdexcept>
#include <typeinfo>

bool socket_error = false, socket_zero = false, bind_error = false;
bool am_root = false, listen_error = false, pthread_create_error = false;
bool pthread_cancel_error = false, pthread_join_error = false;
bool unlink_error = false;
int seteuid_count, setegid_count;

class fake_basesock : public basesock
{
  public:
    fake_basesock() : basesock() {}
    fake_basesock(Addrinfo *ai) : basesock(ai) {}
    ~fake_basesock() {}
    using basesock::sa;
    using basesock::sock;
};

/* This will never actually be run, so it doesn't need to do anything */
void *test_thread_worker(void *arg)
{
    return NULL;
}

int socket(int a, int b, int c)
{
    if (socket_error == true)
        return -1;
    if (socket_zero == true)
        return 0;
    return 123;
}

int setsockopt(int a, int b, int c, const void *d, socklen_t e)
{
    return 0;
}

int bind(int a, const struct sockaddr *b, socklen_t c)
{
    if (bind_error == true)
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int listen(int a, int b)
{
    if (listen_error == true)
    {
        errno = ENOTSOCK;
        return -1;
    }
    return 0;
}

uid_t getuid(void)
{
    if (am_root == true)
        return 0;
    return 123;
}

uid_t geteuid(void)
{
    return 123;
}

int seteuid(uid_t a)
{
    ++seteuid_count;
    return 0;
}

gid_t getgid(void)
{
    if (am_root == true)
        return 0;
    return 123;
}

gid_t getegid(void)
{
    return 123;
}

int setegid(gid_t a)
{
    ++setegid_count;
    return 0;
}

int pthread_create(pthread_t *a, const pthread_attr_t *b,
                   void *(*c)(void *), void *d)
{
    if (pthread_create_error == true)
        return EINVAL;
    return 0;
}

int pthread_cancel(pthread_t a)
{
    if (pthread_cancel_error == true)
        return EINVAL;
    return 0;
}

int pthread_join(pthread_t a, void **b)
{
    if (pthread_join_error == true)
        return EINVAL;
    return 0;
}

int unlink(const char *a)
{
    if (unlink_error == true)
        return -1;
    return 0;
}

void test_create_delete(void)
{
    std::string test = "create/delete: ", st;
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235", AF_INET);
    fake_basesock *base;

    st = "IPv4: ";
    socket_zero = true;
    try
    {
        base = new fake_basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(base->sa->ntop(), "127.0.0.1", 10),
       0,
       test + st + "expected address");
    is(base->sa->port(), 1235, test + st + "expected port");

    delete base;
    delete ai;

    st = "IPv6: ";
    ai = new Addrinfo(DGRAM, "localhost", "1235", AF_INET6);

    try
    {
        base = new fake_basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(base->sa->ntop(), "::1", 4),
       0,
       test + st + "expected address");
    is(base->sa->port(), 1235, test + st + "expected port");

    delete base;
    delete ai;
    socket_zero = false;
}

void test_delete_unix(void)
{
    std::string test = "delete unix: ";
    std::stringstream *new_clog = new std::stringstream;
    auto old_clog_rdbuf = std::clog.rdbuf(new_clog->rdbuf());

    Addrinfo_un *ai = new Addrinfo_un("t_basesock.sock");
    fake_basesock *base;

    try
    {
        base = new fake_basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    unlink_error = true;

    delete base;
    delete ai;

    unlink_error = false;

    std::clog.rdbuf(old_clog_rdbuf);
    isnt(new_clog->str().find("could not unlink path"), std::string::npos,
         test + "expected unlink failure");

    /* We explicitly leak the new_clog object here because we're
     * seeing segfaults on Linux hosts when we try to delete it.  It's
     * a test, so it's not super-critical that we are leaking
     * something, nor is this object that we're leaking important to
     * the test itself.  We're just trying to make sure that the
     * failure clause was actually executed, and that's what the
     * clause does:  generate an error message in the log.
     */
}

void test_bad_socket(void)
{
    std::string test = "bad socket: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235");
    basesock *base;

    socket_error = true;
    try
    {
        base = new basesock(ai);
        base->start(test_thread_worker);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("socket creation failed"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    socket_error = false;
    delete ai;
}

void test_privileged_socket(void)
{
    std::string test = "privileged socket creation: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "123");
    basesock *base;

    am_root = false;
    seteuid_count = setegid_count = 0;
    try
    {
        base = new basesock(ai);
        base->start(test_thread_worker);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("non-root user"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    is(seteuid_count, 0, test + "expected seteuid count");
    is(setegid_count, 0, test + "expected setegid count");

    am_root = true;
    try
    {
        base = new basesock(ai);
        base->start(test_thread_worker);
    }
    catch (...)
    {
        fail(test + "create_socket exception");
    }
    is(seteuid_count, 2, test + "expected seteuid count");
    is(setegid_count, 2, test + "expected setegid count");

    delete base;
    delete ai;
    seteuid_count = setegid_count = 0;
    am_root = false;
}

void test_bad_bind(void)
{
    std::string test = "bad bind: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235");
    basesock *base;

    bind_error = true;
    try
    {
        base = new basesock(ai);
        base->start(test_thread_worker);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("bind failed for"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    delete base;
    delete ai;
    bind_error = false;
}

void test_bad_listen(void)
{
    std::string test = "bad listen: ";
    Addrinfo *ai = new Addrinfo(STREAM, "localhost", "1235");
    basesock *base;

    listen_error = true;
    try
    {
        base = new basesock(ai);
        base->start(test_thread_worker);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("listen failed for"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    delete base;
    delete ai;
    listen_error = false;
}

void test_start_stop(void)
{
    std::string test = "start/stop: ", st;
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235", AF_INET);
    fake_basesock *base;

    st = "IPv4: ";
    try
    {
        base = new fake_basesock(ai);
        base->start(test_thread_worker);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(base->sa->ntop(), "127.0.0.1", 10), 0,
       test + st + "expected address");
    is(base->sa->port(), 1235, test + st + "expected port");
    ok(base->sock >= 0, test + st + "socket created");

    try
    {
        base->stop();
    }
    catch (...)
    {
        fail(test + st + "start/stop exception");
    }

    delete base;
    delete ai;

    st = "IPv6: ";
    ai = new Addrinfo(DGRAM, "localhost", "1235", AF_INET6);

    try
    {
        base = new fake_basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(base->sa->ntop(), "::1", 4),
       0,
       test + st + "expected address");
    is(base->sa->port(), 1235, test + st + "expected port");
    ok(base->sock >= 0, test + st + "socket created");

    try
    {
        base->start(test_thread_worker);
        base->stop();
    }
    catch (...)
    {
        fail(test + st + "start/stop exception");
    }

    delete base;
    delete ai;
}

void test_start_bad_socket(void)
{
    std::string test = "start w/bad socket: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235");
    fake_basesock *base;

    socket_zero = true;
    try
    {
        base = new fake_basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    try
    {
        base->start(test_thread_worker);
    }
    catch (...)
    {
        fail(test + "start exception");
    }

    delete base;
    delete ai;
    socket_zero = false;
}

void test_start_create_thread_fail(void)
{
    std::string test = "start w/bad pthread_create: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235");
    basesock *base;

    pthread_create_error = true;
    try
    {
        base = new basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    try
    {
        base->start(test_thread_worker);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't start listen thread"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    delete base;
    delete ai;
    pthread_create_error = false;
}

void test_stop_cancel_fail(void)
{
    std::string test = "stop w/bad pthread_cancel: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235", AF_INET);
    basesock *base;

    try
    {
        base = new basesock(ai);
        base->start(test_thread_worker);
    }
    catch (...)
    {
        fail(test + "setup failure");
    }
    is(strncmp(base->sa->ntop(), "127.0.0.1", 10), 0,
       test + "expected address");
    is(base->sa->port(), 1235, test + "expected port");
    ok(base->sock >= 0, test + "socket created");

    pthread_cancel_error = true;
    try
    {
        base->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't cancel listen thread"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    try
    {
        delete(base);
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
    delete ai;
    pthread_cancel_error = false;
}

void test_stop_join_fail(void)
{
    std::string test = "stop w/bad pthread_join: ";
    Addrinfo *ai = new Addrinfo(DGRAM, "localhost", "1235", AF_INET);
    basesock *base;

    try
    {
        base = new basesock(ai);
        base->start(test_thread_worker);
    }
    catch (...)
    {
        fail(test + "setup failure");
    }
    is(strncmp(base->sa->ntop(), "127.0.0.1", 10), 0,
       test + "expected address");
    is(base->sa->port(), 1235, test + "expected port");
    ok(base->sock >= 0, test + "socket created");

    pthread_join_error = true;
    try
    {
        base->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't join listen thread"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    delete base;
    delete ai;
    pthread_join_error = false;
}

int main(int argc, char **argv)
{
    plan(28);

    test_create_delete();
    test_delete_unix();
    test_bad_socket();
    test_privileged_socket();
    test_bad_bind();
    test_bad_listen();
    test_start_stop();
    test_start_bad_socket();
    test_start_create_thread_fail();
    test_stop_cancel_fail();
    test_stop_join_fail();
    return exit_status();
}
