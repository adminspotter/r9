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

int main(int argc, char **argv)
{
    plan(1);

    test_pkey_to_string();
    return exit_status();
}
