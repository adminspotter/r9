#include <tap++.h>

using namespace TAP;

#include <openssl/opensslv.h>

#include "../proto/dh.h"

struct dh_message msg;

bool malloc_error = false;

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
void *CRYPTO_malloc(size_t a, const char *b, int c)
#else
void *CRYPTO_malloc(int a, const char *b, int c)
#endif
{
    if (malloc_error)
        return NULL;
    return &msg;
}

void test_dh_shared_secret(void)
{
    std::string test = "dh_shared_secret: ", st;

    st = "malloc error: ";
    malloc_error = true;
    is(dh_shared_secret(NULL, NULL) == NULL,
       true,
       test + st + "expected result");
}

void test_digest_message(void)
{
}

int main(int argc, char **argv)
{
    plan(1);

    test_dh_shared_secret();
    test_digest_message();
    return exit_status();
}
