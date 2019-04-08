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

#include <iostream>

void show_banner(void)
{
    std::cout << "r9_keygen v" PACKAGE_VERSION << std::endl;
}

int main(int argc, char **argv)
{
    show_banner();
    return 0;
}
