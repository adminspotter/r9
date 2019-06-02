/* config_data.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Jun 2019, 22:30:08 tquirk
 *
 * Revision IX game server
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
 * This file contains the configuration class declaration.
 *
 * Things to do
 *
 */

#ifndef __INC_CONFIG_DATA_H__
#define __INC_CONFIG_DATA_H__

#include <cstdint>
#include <vector>
#include <string>

#include <openssl/evp.h>

typedef struct location_struct
{
    uint64_t dim[3];
    uint16_t steps[3];
}
location;

/* We need to be able to represent any kind of port in a single structure */
typedef enum port_type_tag
{
    port_unix = 0,
    port_dgram,
    port_stream
}
port_type;
typedef struct port_struct
{
    port_type type;
    std::string addr, port;

    port_struct() : addr(), port() {};
}
port;

typedef struct crypto_key_struct
{
    EVP_PKEY *priv_key;
    uint8_t pub_key[170];
}
crypto_key;

class config_data
{
  public:
    /* Some default constants */
    static const int LINGER_LEN;
    static const int LOG_FACILITY;
    static const int NUM_THREADS;
    static const int ZONE_SIZE;
    static const int ZONE_STEPS;
    static const char SERVER_ROOT[];
    static const char LOG_PREFIX[];
    static const char PID_FNAME[];
    static const char DB_TYPE[];
    static const char DB_HOST[];
    static const char DB_NAME[];
    static const char ACTION_LIB[];

    std::vector<std::string> argv;
    std::vector<port> listen_ports, consoles;
    bool daemonize, use_keepalive, use_nonblock, use_reuse;
    int use_linger, log_facility;
    std::string server_root, log_prefix, pid_fname;
    int access_threads, action_threads, motion_threads, send_threads;
    int update_threads;
    location size, spawn;
    std::string db_type, db_host, db_user, db_pass, db_name;
    std::string action_lib;

    crypto_key key;

    config_data();
    ~config_data();

    void parse_command_line(int, char **);
    void read_config_file(std::string&);

    void set_defaults(void);

  private:
    void parse_config_line(std::string&);
};

extern config_data config;

void setup_configuration(int, char **);
void cleanup_configuration(void);

#endif /*__INC_CONFIG_DATA_H__*/
