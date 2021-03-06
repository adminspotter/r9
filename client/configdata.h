/* configdata.h                                            -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 14 Dec 2020, 07:33:34 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2020  Trinity Annabelle Quirk
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
 * This file contains the class declaration for loading and saving the
 * client configuration.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_CONFIGDATA_H__
#define __INC_R9CLIENT_CONFIGDATA_H__

#include <vector>
#include <string>

#include "../proto/ec.h"

class ConfigData
{
  public:
    static const char SERVER_ADDR[];
    static const int SERVER_PORT;
    static const char *FONT_PATHS[];
    static const char CONTROLLER[];

    std::vector<std::string> argv;
    std::string config_dir, config_fname;

    std::string server_addr;
    uint16_t server_port;
    std::string username, charname;
    std::string font_name;
    std::vector<std::string> font_paths;

    std::string controller;

    std::string key_fname;
    EVP_PKEY *priv_key;
    uint8_t pub_key[R9_PUBKEY_SZ];

    ConfigData();
    ~ConfigData();

    void set_defaults(void);

    void parse_command_line(int, const char **);
    void read_config_file(void);
    void write_config_file(void);

    bool read_crypto_key(const std::string&);

  protected:
    void make_config_dirs(void);
    void parse_config_line(std::string&);
};

extern ConfigData config;

#endif /* __INC_R9CLIENT_CONFIGDATA_H__ */
