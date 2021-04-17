#include <tap++.h>

using namespace TAP;

#include "../client/configdata.h"
#include "../client/comm.h"
#include "../client/object.h"

ConfigData config;
ObjectCache *obj;
struct object *self_obj;

bool socket_error = false, mutex_error = false, cond_error = false;
bool create_send_error = false, create_recv_error = false;
bool broadcast_error = false, join_error = false, second_join_error = false;
bool cancel_error = false;
int create_calls, cancel_calls, join_calls;
int cond_destroy_calls, mutex_destroy_calls;

void move_object(uint64_t a, uint16_t b,
                 float c, float d, float e,
                 float f, float g, float h, float i,
                 float j, float k, float l)
{
    return;
}

int ntoh_packet(packet *a, size_t b)
{
    return 1;
}

int hton_packet(packet *a, size_t b)
{
    return 1;
}

size_t packet_size(packet *a)
{
    return 1;
}

int socket(int a, int b, int c)
{
    if (socket_error == true)
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *a, const pthread_mutexattr_t *b)
{
    if (mutex_error == true)
        return EINVAL;
    return 0;
}

int pthread_cond_init(pthread_cond_t *a, const pthread_condattr_t *b)
{
    if (cond_error == true)
        return EINVAL;
    return 0;
}

int pthread_create(pthread_t *a, const pthread_attr_t *b,
                   void *(*c)(void *), void *d)
{
    ++create_calls;
    if (create_send_error == true && create_calls == 1)
        return EINVAL;
    if (create_recv_error == true && create_calls == 2)
        return EINVAL;
    return 0;
}

int pthread_cancel(pthread_t a)
{
    ++cancel_calls;
    if (cancel_error == true)
        return EINVAL;
    return 0;
}

int pthread_join(pthread_t a, void **b)
{
    ++join_calls;
    if (join_error == true && join_calls == 1)
        return EINVAL;
    if (second_join_error == true && join_calls == 2)
        return EINVAL;
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *a)
{
    ++cond_destroy_calls;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *a)
{
    ++mutex_destroy_calls;
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *a)
{
    if (broadcast_error == true)
        return EINVAL;
    return 0;
}

void test_socket_failure(void)
{
    std::string test = "socket failure: ";
    Comm *obj = NULL;
    struct addrinfo a;

    socket_error = true;
    try
    {
        obj = new Comm(&a);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error opening socket"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    socket_error = false;
}

void test_mutex_failure(void)
{
    std::string test = "mutex failure: ";
    Comm *obj = NULL;
    struct addrinfo a;
    struct sockaddr_storage s;
    a.ai_addr = (struct sockaddr *)&s;

    mutex_error = true;
    try
    {
        obj = new Comm(&a);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error initializing queue mutex"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    mutex_error = false;
}

void test_cond_failure(void)
{
    std::string test = "cond failure: ";
    Comm *obj = NULL;
    struct addrinfo a;
    struct sockaddr_storage s;
    a.ai_addr = (struct sockaddr *)&s;

    cond_error = true;
    try
    {
        obj = new Comm(&a);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error initializing queue-not-empty cond"),
             std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    cond_error = false;
}

void test_start_failure(void)
{
    std::string test = "start failure: ", st;
    Comm *obj = NULL;
    struct addrinfo a;
    struct sockaddr_storage s;
    a.ai_addr = (struct sockaddr *)&s;

    st = "create send: ";
    create_send_error = true;
    cancel_calls = join_calls = cond_destroy_calls = mutex_destroy_calls
        = create_calls = 0;
    try
    {
        obj = new Comm(&a);
    }
    catch (...)
    {
        fail(test + st + "constructor exception");
    }

    try
    {
        obj->start();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error starting send thread"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        fail(test + st + "wrong error type");
    }
    is(cancel_calls, 0, test + st + "expected cancels");
    is(join_calls, 0, test + st + "expected joins");
    is(cond_destroy_calls, 1, test + st + "expected cond destroys");
    is(mutex_destroy_calls, 1, test + st + "expected mutex destroys");

    st = "create recv: ";
    create_send_error = false;
    create_recv_error = true;
    cancel_calls = join_calls = cond_destroy_calls = mutex_destroy_calls
        = create_calls = 0;
    try
    {
        obj->start();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error starting receive thread"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        fail(test + st + "wrong error type");
    }
    is(cancel_calls, 1, test + st + "expected cancels");
    is(join_calls, 1, test + st + "expected joins");
    is(cond_destroy_calls, 1, test + st + "expected cond destroys");
    is(mutex_destroy_calls, 1, test + st + "expected mutex destroys");

    st = "cleanup destruction: ";
    create_recv_error = false;
    cancel_calls = join_calls = cond_destroy_calls = mutex_destroy_calls
        = create_calls = 0;
    obj->start();
    delete obj;
    is(cancel_calls, 1, test + st + "expected cancels");
    is(join_calls, 2, test + st + "expected joins");
    is(cond_destroy_calls, 1, test + st + "expected cond destroys");
    is(mutex_destroy_calls, 1, test + st + "expected mutex destroys");
    cancel_calls = join_calls = cond_destroy_calls = mutex_destroy_calls
        = create_calls = 0;
}

void test_stop_failure(void)
{
    std::string test = "stop failure: ", st;
    Comm *obj = NULL;
    struct addrinfo a;
    struct sockaddr_storage s;
    a.ai_addr = (struct sockaddr *)&s;

    st = "cond broadcast: ";
    try
    {
        obj = new Comm(&a);
        obj->start();
    }
    catch (...)
    {
        fail(test + st + "constructor exception");
    }
    broadcast_error = true;
    try
    {
        obj->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error waking send thread"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        fail(test + st + "wrong error type");
    }

    st = "join: ";
    broadcast_error = false;
    join_error = true;
    join_calls = 0;
    try
    {
        obj->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error joining send thread"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        fail(test + st + "wrong error type");
    }

    st = "cancel: ";
    join_error = false;
    cancel_error = true;
    try
    {
        obj->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error cancelling receive thread"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        fail(test + st + "wrong error type");
    }

    st = "second join: ";
    cancel_error = false;
    second_join_error = true;
    join_calls = 0;
    try
    {
        obj->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Error joining receive thread"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        fail(test + st + "wrong error type");
    }

    delete obj;
}

int main(int argc, char **argv)
{
    plan(21);

    test_socket_failure();
    test_mutex_failure();
    test_cond_failure();
    test_start_failure();
    test_stop_failure();
    return exit_status();
}
