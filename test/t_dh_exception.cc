#include <tap++.h>

using namespace TAP;

#include <openssl/opensslv.h>

#include "../proto/dh.h"

struct dh_message msg;
/* EVP_PKEY_CTX is an opaque handle, so we need to define something fake. */
struct evp_pkey_ctx_st { int a; };
EVP_PKEY_CTX ctx;

bool malloc_error = false, pkey_new_error = false;
int free_count = 0;

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

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
void CRYPTO_free(void *a, const char *b, int c)
#else
void CRYPTO_free(void *a)
#endif
{
    free_count += 1;
    return;
}

EVP_PKEY_CTX *EVP_PKEY_CTX_new(EVP_PKEY *a, ENGINE *b)
{
    if (pkey_new_error)
        return NULL;
    return &ctx;
}

void test_dh_shared_secret(void)
{
    std::string test = "dh_shared_secret: ", st;

    st = "malloc error: ";
    malloc_error = true;
    is(dh_shared_secret(NULL, NULL) == NULL,
       true,
       test + st + "expected result");

    st = "pkey new error: ";
    malloc_error = false;
    pkey_new_error = true;
    free_count = 0;
    is(dh_shared_secret(NULL, NULL) == NULL,
       true,
       test + st + "expected result");
    is(free_count, 1, test + st + "expected free calls");
}

void test_digest_message(void)
{
}

int main(int argc, char **argv)
{
    plan(3);

    test_dh_shared_secret();
    test_digest_message();
    return exit_status();
}
