/* config_data.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 27 Nov 2015, 06:23:54 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2015  Trinity Annabelle Quirk
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

typedef struct location_struct
{
    uint64_t dim[3];
    uint16_t steps[3];
}
location;

class config_data
{
  public:
    /* Some default constants */
    static const int LINGER_LEN;
    static const int LOG_FACILITY;
    static const int MIN_SUBSERV;
    static const int MAX_SUBSERV;
    static const int NUM_THREADS;
    static const int ZONE_SIZE;
    static const int ZONE_STEPS;
    static const float LOAD_THRESH;
    static const char SERVER_ROOT[];
    static const char LOG_PREFIX[];
    static const char PID_FNAME[];
    static const char DB_TYPE[];
    static const char DB_HOST[];
    static const char DB_NAME[];
    static const char ACTION_LIB[];

    std::vector<std::string> argv;
    std::vector<int> stream, dgram;
    bool daemonize, use_keepalive, use_nonblock, use_reuse;
    int use_linger, log_facility;
    std::string server_root, log_prefix, pid_fname;
    float load_threshold;
    int min_subservers, max_subservers;
    int access_threads, action_threads, motion_threads, send_threads;
    int update_threads;
    location size, spawn;
    std::string db_type, db_host, db_user, db_pass, db_name;
    std::string action_lib;

    std::string console_fname;
    int console_inet;

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
