#include "../client/comm.h"
#include "../client/object.h"

#include <gtest/gtest.h>

ObjectCache *obj;
struct object *self_obj;

bool socket_error = false, mutex_error = false, cond_error = false;
bool create_send_error = false, create_recv_error = false;
int create_calls, cancel_calls, join_calls;
int cond_destroy_calls, mutex_destroy_calls;

void move_object(uint64_t a, uint16_t b,
                 float c, float d, float e,
                 float f, float g, float h, float i)
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
    return 123;
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
    return 0;
}

int pthread_join(pthread_t a, void **b)
{
    ++join_calls;
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
    return 0;
}

TEST(CommTest, SocketFailure)
{
    Comm *obj = NULL;
    struct addrinfo a;

    socket_error = true;
    ASSERT_THROW(
        {
            obj = new Comm(&a);
        },
        std::runtime_error);
    socket_error = false;
}

TEST(CommTest, MutexFailure)
{
    Comm *obj = NULL;
    struct addrinfo a;

    mutex_error = true;
    ASSERT_THROW(
        {
            obj = new Comm(&a);
        },
        std::runtime_error);
    mutex_error = false;
}

TEST(CommTest, CondFailure)
{
    Comm *obj = NULL;
    struct addrinfo a;

    cond_error = true;
    ASSERT_THROW(
        {
            obj = new Comm(&a);
        },
        std::runtime_error);
    cond_error = false;
}

TEST(CommTest, StartFailure)
{
    Comm *obj = NULL;
    struct addrinfo a;

    create_send_error = true;
    cancel_calls = join_calls = cond_destroy_calls = mutex_destroy_calls
        = create_calls = 0;
    ASSERT_NO_THROW(
        {
            obj = new Comm(&a);
        });
    ASSERT_THROW(
        {
            obj->start();
        },
        std::runtime_error);
    ASSERT_EQ(cancel_calls, 0);
    ASSERT_EQ(join_calls, 0);
    ASSERT_EQ(cond_destroy_calls, 1);
    ASSERT_EQ(mutex_destroy_calls, 1);
    create_send_error = false;
    create_recv_error = true;
    cancel_calls = join_calls = cond_destroy_calls = mutex_destroy_calls
        = create_calls = 0;
    ASSERT_THROW(
        {
            obj->start();
        },
        std::runtime_error);
    ASSERT_EQ(cancel_calls, 1);
    ASSERT_EQ(join_calls, 1);
    ASSERT_EQ(cond_destroy_calls, 1);
    ASSERT_EQ(mutex_destroy_calls, 1);
    create_recv_error = false;
    cancel_calls = join_calls = cond_destroy_calls = mutex_destroy_calls
        = create_calls = 0;
    delete obj;
    ASSERT_EQ(cancel_calls, 1);
    ASSERT_EQ(join_calls, 2);
    ASSERT_EQ(cond_destroy_calls, 1);
    ASSERT_EQ(mutex_destroy_calls, 1);
    cancel_calls = join_calls = cond_destroy_calls = mutex_destroy_calls
        = create_calls = 0;
}
