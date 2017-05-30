#include "../client/comm.h"

#include <gtest/gtest.h>

bool socket_error = false, mutex_error = false, cond_error = false;

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
