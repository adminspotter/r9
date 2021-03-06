#include <tap++.h>

using namespace TAP;

#include "../proto/key.h"
#include "../proto/ec.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

void test_pkey_to_string(void)
{
    std::string test = "pkey_to_string: ";

    EVP_PKEY *key = generate_ecdh_key();

    unsigned char str[1024];
    size_t str_size = pkey_to_string(key, (unsigned char **)&str, sizeof(str));
    isnt(str_size, 0, test + "string conversion correct");
    diag(str_size);

    OPENSSL_free(key);
}

void test_string_to_pkey(void)
{
    std::string test = "string_to_pkey: ";

    EVP_PKEY *key = generate_ecdh_key();

    unsigned char str[1024];
    size_t str_size = pkey_to_string(key, (unsigned char **)&str, sizeof(str));
    isnt(str_size, 0, test + "string conversion correct");
    diag(str_size);

    EVP_PKEY *converted = string_to_pkey(str, str_size);

    is(converted != NULL, true, test + "key conversion correct");

    OPENSSL_free(converted);
    OPENSSL_free(key);
}

void test_pkey_to_pub_string(void)
{
    std::string test = "pkey_to_pub_string: ";

    EVP_PKEY *key = generate_ecdh_key();

    unsigned char str[1024];
    size_t str_size = pkey_to_pub_string(key,
                                         (unsigned char **)&str,
                                         sizeof(str));
    isnt(str_size, 0, test + "string conversion correct");
    diag(str_size);

    OPENSSL_free(key);
}

void test_pub_string_to_pkey(void)
{
    std::string test = "pub_string_to_pkey: ";

    EVP_PKEY *key = generate_ecdh_key();

    unsigned char str[1024];
    size_t str_size = pkey_to_pub_string(key,
                                         (unsigned char **)&str,
                                         sizeof(str));
    isnt(str_size, 0, test + "string conversion correct");
    diag(str_size);

    EVP_PKEY *converted = pub_string_to_pkey(str, str_size);

    is(converted != NULL, true, test + "key conversion correct");

    OPENSSL_free(converted);
    OPENSSL_free(key);
}

void test_pkey_to_pub_der(void)
{
    std::string test = "pkey_to_pub_der: ";

    EVP_PKEY *key = generate_ecdh_key();

    unsigned char str[1024];
    size_t str_size = pkey_to_pub_der(key,
                                      (unsigned char **)&str,
                                      sizeof(str));
    isnt(str_size, 0, test + "string conversion correct");
    diag(str_size);

    OPENSSL_free(key);
}

void test_pub_der_to_pkey(void)
{
    std::string test = "pub_der_to_pkey: ";

    EVP_PKEY *key = generate_ecdh_key();

    unsigned char str[1024];
    size_t str_size = pkey_to_pub_der(key,
                                      (unsigned char **)&str,
                                      sizeof(str));
    isnt(str_size, 0, test + "string conversion correct");
    diag(str_size);

    EVP_PKEY *converted = pub_der_to_pkey(str, str_size);

    is(converted != NULL, true, test + "key conversion correct");

    OPENSSL_free(converted);
    OPENSSL_free(key);
}

void test_pkey_to_file(void)
{
    std::string test = "pkey_to_file: ";

    EVP_PKEY *key = generate_ecdh_key();
    const char *fname = "t_pkey_to_file.pem";
    unsigned char pass[] = "abc123";

    int result = pkey_to_file(key, fname, pass);

    is(result, 1, test + "expected result");

    struct stat stat_struct;
    int stat_result = stat(fname, &stat_struct);
    is(stat_result, 0, test + "key file exists");
    is(stat_struct.st_mode,
       S_IFREG | S_IRUSR | S_IWUSR,
       test + "expected file permissions");
    diag(stat_struct.st_size);
    unlink(fname);
    OPENSSL_free(key);
}

void test_file_to_pkey(void)
{
    std::string test = "file_to_pkey: ";

    EVP_PKEY *key = generate_ecdh_key();
    const char *fname = "t_file_to_pkey.pem";

    int result = pkey_to_file(key, fname, NULL);

    EVP_PKEY *loaded_key = file_to_pkey(fname, NULL);

    is(loaded_key != NULL, true, test + "loaded a key");

    unlink(fname);
    OPENSSL_free(key);
    OPENSSL_free(loaded_key);
}

int main(int argc, char **argv)
{
    plan(13);

#if OPENSSL_API_COMPAT < 0x10100000
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();
#endif /* OPENSSL_API_COMPAT */
    test_pkey_to_string();
    test_string_to_pkey();
    test_pkey_to_pub_string();
    test_pub_string_to_pkey();
    test_pkey_to_pub_der();
    test_pub_der_to_pkey();
    test_pkey_to_file();
    test_file_to_pkey();
    return exit_status();
}
