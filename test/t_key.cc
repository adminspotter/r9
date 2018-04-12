#include <tap++.h>

using namespace TAP;

#include "../proto/key.h"
#include "../proto/ec.h"

void test_pkey_to_string(void)
{
    std::string test = "pkey_to_string: ";

    EVP_PKEY *key = generate_ecdh_key();

    unsigned char str[1024];
    size_t str_size = pkey_to_string(key, (unsigned char **)&str, sizeof(str));
    isnt(str_size, 0, test + "string conversion correct");

    OPENSSL_free(key);
}

void test_string_to_pkey(void)
{
    std::string test = "string_to_pkey: ";

    EVP_PKEY *key = generate_ecdh_key();

    unsigned char str[1024];
    size_t str_size = pkey_to_string(key, (unsigned char **)&str, sizeof(str));
    isnt(str_size, 0, test + "string conversion correct");

    EVP_PKEY *converted = string_to_pkey(str, str_size);

    is(converted != NULL, true, test + "key conversion correct");

    OPENSSL_free(converted);
    OPENSSL_free(key);
}

int main(int argc, char **argv)
{
    plan(3);

    test_pkey_to_string();
    test_string_to_pkey();
    return exit_status();
}
