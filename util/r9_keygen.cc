/* r9_keygen.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 Apr 2019, 09:49:59 tquirk
 *
 * Revision IX game utility
 * Copyright (C) 2019  Trinity Annabelle Quirk
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * This file contains the utility code for generating ECDH keys.  It
 * will write the private key into the appropriate place in the
 * filesystem, based on the expected location.
 *
 * Client keys will require a passphrase.
 *
 * Things to do
 *
 */

#include <config.h>

#if HAVE_STDIO_H
#include <stdio.h>
#endif /* HAVE_STDIO_H */
#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#if HAVE_TERMIOS_H
#include <termios.h>
#endif /* HAVE_TERMIOS_H */
#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if WANT_LOCALES && HAVE_LIBINTL_H
#include <libintl.h>
#define _(x)  maketext(x)
#else
#define _(x)  x
#endif /* WANT_LOCALES && HAVE_LIBINTL_H */

#include <iostream>

#include <proto/key.h>
#include <proto/ec.h>

#define ARGPARSE_RETURN 2
#define KEYGEN_RETURN 3

#ifndef SYSCONFDIR
#define SYSCONFDIR "/etc"
#endif /* SYSCONFDIR */

bool do_client_key = false, do_server_key = false;
std::string key_path;

void show_banner(void)
{
    std::cout << "r9_keygen v" PACKAGE_VERSION << std::endl;
}

void process_command_line(int argc, char **argv)
{
    const char *optstr = "csf:";
    int opt;
#if HAVE_GETOPT_LONG
    int longindex = 0;
    struct option options[] = {
        { "client",  no_argument,        NULL,  'c' },
        { "server",  no_argument,        NULL,  's' },
        { "file",    required_argument,  NULL,  'f' },
        { 0,         0,                  NULL,  0   }
    };
#endif /* HAVE_GETOPT_LONG */

    while ((opt =
#if HAVE_GETOPT_LONG
            getopt_long(argc, argv, optstr, options, &longindex)
#else
            getopt(argc, argv, optstr)
#endif /* HAVE_GETOPT_LONG */
               ) != -1)
    {
        switch (opt)
        {
          case 'c':
            if (do_server_key == false)
                do_client_key = true;
            else
            {
                std::cerr << _("Can't do both server and client keys.")
                          << std::endl;
                exit(ARGPARSE_RETURN);
            }
            break;
          case 's':
            if (do_client_key == false)
                do_server_key = true;
            else
            {
                std::cerr << _("Can't do both server and client keys.")
                          << std::endl;
                exit(ARGPARSE_RETURN);
            }
            break;
          case 'f':
            key_path = optarg;
            break;
          case '?':
#if !HAVE_GETOPT_LONG
            std::cerr << _("Unknown option ") << (char)opt << std::endl;
#endif /* !HAVE_GETOPT_LONG */
            exit(ARGPARSE_RETURN);
          default:
            std::cerr << _("Error in argument parsing") << std::endl;
            exit(ARGPARSE_RETURN);
        }
    }
}

void generate_key_path(void)
{
    if (key_path.size() == 0)
    {
        if (do_server_key == true)
            key_path = SYSCONFDIR "/r9dkey";
        else if (do_client_key == true)
        {
            key_path = getenv("HOME");
            key_path += "/.r9/key";
        }
        else
            key_path = "./key";
    }
    std::cout << _("Writing to ") << key_path << std::endl;
}

std::string ask_for_passphrase(void)
{
    std::string passphrase, passphrase2;

    do
    {
        struct termios t_old, t_new;

        if (passphrase.size() != 0 && passphrase != passphrase2)
            std::cout << _("Passphrases do not match!") << std::endl;
        std::cout << _("Enter passphrase: ");
        std::cout.flush();
        tcgetattr(STDIN_FILENO, &t_old);
        t_new = t_old;
        t_new.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t_new);
        std::cin >> passphrase;
        std::cout << std::endl << _("Repeat passphrase: ");
        std::cout.flush();
        std::cin >> passphrase2;
        tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
        std::cout << std::endl;
    }
    while (passphrase != passphrase2);
    return passphrase;
}

EVP_PKEY *generate_key(void)
{
    return generate_ecdh_key();
}

void write_key(EVP_PKEY *key, std::string& key_fname, std::string& passphrase)
{
    unsigned char pp[passphrase.size() + 1];
    int i = 0;

    for (char c : passphrase)
        pp[i++] = (unsigned char)c;
    pp[i] = 0;

    if (pkey_to_file(key, key_fname.c_str(), pp) == 0)
    {
        std::cerr << _("Could not write private key file: ")
                  << strerror(errno) << "(" << errno << ")" << std::endl;
        exit(KEYGEN_RETURN);
    }
}

int main(int argc, char **argv)
{
    std::string passphrase;

    show_banner();
    process_command_line(argc, argv);
    generate_key_path();
    if (do_client_key == true)
        passphrase = ask_for_passphrase();

#if OPENSSL_API_COMPAT < 0x10100000
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();
#endif /* OPENSSL_API_COMPAT */
    EVP_PKEY *new_key = generate_key();
    write_key(new_key, key_path, passphrase);

    return 0;
}
