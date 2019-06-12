#include <tap++.h>

#include <string.h>

#include <iostream>
#include <iomanip>

using namespace TAP;

#include "../proto/encrypt.h"

void test_encrypt(void)
{
    std::string test = "r9_encrypt: ";

    const unsigned char plaintext[16] = "test text";
    const unsigned char key[33] = "abcd1234efgh5678ijkl9012mnop3456";
    unsigned char iv[33] = "12345678123456781234567812345678";
    uint64_t sequence = 1LL;
    unsigned char ciphertext[16];

    memset(ciphertext, 0, sizeof(ciphertext));
    int result = r9_encrypt(plaintext, strlen((const char *)plaintext),
                            key, iv, sequence, ciphertext);

    is(result, 9, test + "expected result");

    const unsigned char expected[16] =
        {
            0xb7, 0x00, 0x90, 0x21, 0x50, 0x71, 0xfa, 0x40,
            0xec, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
    if (!is(memcmp(ciphertext, expected, sizeof(expected)),
            0,
            test + "expected ciphertext"))
    {
        std::cerr << "# expected:" << std::endl << "# {" << std::endl
                  << "#     ";
        for (unsigned char i : expected)
            std::cerr << std::hex << std::setfill('0') << std::setw(2)
                      << (unsigned int)i << ' ';
        std::cerr << std::endl << "# }" << std::endl
                  << "# ciphertext:" << std::endl << "# {" << std::endl
                  << "#     ";
        for (unsigned char i : ciphertext)
            std::cerr << std::hex << std::setfill('0') << std::setw(2)
                      << (unsigned int)i << ' ';
        std::cerr << std::dec << std::endl << "# }" << std::endl;
    }
    isnt(memcmp(ciphertext, plaintext, sizeof(plaintext)),
         0,
         test + "ciphertext is not plaintext");
}

void test_decrypt(void)
{
    std::string test = "r9_decrypt: ";

    const unsigned char plaintext[16] = "test text";
    const unsigned char key[33] = "abcd1234efgh5678ijkl9012mnop3456";
    unsigned char iv[33] = "12345678123456781234567812345678";
    uint64_t sequence = 1LL;
    unsigned char ciphertext[16], cleartext[16];

    memset(ciphertext, 0, sizeof(ciphertext));
    memset(cleartext, 0, sizeof(cleartext));
    int cipherlen = r9_encrypt(plaintext, strlen((const char *)plaintext),
                               key, iv, sequence, ciphertext);
    int result = r9_decrypt(ciphertext, cipherlen,
                            key, iv, sequence, cleartext);

    is(result, 9, test + "expected result");
    is(memcmp(plaintext, cleartext, sizeof(cleartext)),
       0,
       test + "expected cleartext");
}

void test_in_place_operation(void)
{
    std::string test = "in-place round-trip: ";

    const unsigned char expected_clear[16] = "test text";
    const unsigned char expected_encrypt[16] =
        {
            0xb7, 0x00, 0x90, 0x21, 0x50, 0x71, 0xfa, 0x40,
            0xec, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
    unsigned char text[16] = "test text";
    const unsigned char key[33] = "abcd1234efgh5678ijkl9012mnop3456";
    unsigned char iv[33] = "12345678123456781234567812345678";
    uint64_t sequence = 1LL;

    int result = r9_encrypt(text, strlen((const char *)text),
                            key, iv, sequence, text);

    is(result, 9, test + "expected ciphertext length");
    is(memcmp(text, expected_encrypt, sizeof(expected_encrypt)),
       0,
       test + "expected ciphertext");

    result = r9_decrypt(text, result, key, iv, sequence, text);

    is(result, 9, test + "expected cleartext length");
    is(memcmp(text, expected_clear, sizeof(expected_clear)),
       0,
       test + "expected cleartext");
}

int main(int argc, char **argv)
{
    plan(9);

    test_encrypt();
    test_decrypt();
    test_in_place_operation();
    return exit_status();
}
