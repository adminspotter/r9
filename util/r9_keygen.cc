/* r9_keygen.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 07 Apr 2019, 17:23:37 tquirk
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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

#include <iostream>

#define ARGPARSE_RETURN 2

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
                std::cerr << "Can't do both server and client keys."
                          << std::endl;
                exit(ARGPARSE_RETURN);
            }
            break;
          case 's':
            if (do_client_key == false)
                do_server_key = true;
            else
            {
                std::cerr << "Can't do both server and client keys."
                          << std::endl;
                exit(ARGPARSE_RETURN);
            }
            break;
          case 'f':
            key_path = optarg;
            break;
          case '?':
#if !HAVE_GETOPT_LONG
            std::cerr << "Unknown option " << (char)opt << std::endl;
#endif /* !HAVE_GETOPT_LONG */
            exit(ARGPARSE_RETURN);
          default:
            std::cerr << "Error in argument parsing" << std::endl;
            exit(ARGPARSE_RETURN);
        }
    }
}

int main(int argc, char **argv)
{
    show_banner();
    process_command_line(argc, argv);
    return 0;
}
