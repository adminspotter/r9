/* config_data.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Aug 2015, 09:46:56 tquirk
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
 * This file contains the routines for reading server configuration
 * data from a file/command-line.
 *
 * Current configuration options include:
 *   AccessThreads <num>    number of access threads to start
 *   ActionLib <libname>    name of library which contains the action routines
 *   ActionThreads <num>    number of action threads to start
 *   ConFilename <string>   filename of the unix console
 *   ConPort <num>          port of the inet console
 *   DBDatabase <dbname>    the name of the database to use
 *   DBHost <host>          database server hostname
 *   DBPassword <password>  the unencrypted password to get into the database
 *   DBType <type>          which database to use - mysql, pgsql, etc.
 *   DBUser <username>      the database username
 *   DgramPort <number>     a UDP port the server will listen to
 *   LogFacility <string>   the facility that the program will use for syslog
 *   LogPrefix <string>     the prefix that the program will use in syslog
 *   MaxSubservers <num>    the maximum number of subservers
 *   MinSubservers <num>    the minimum number of subservers
 *   MotionThreads <num>    number of motion threads to start
 *   PidFile <fname>        the pid/lock file to use
 *   SendThreads <num>      number of send threads to start
 *   ServerGID <group>      the server will run as group id <group>
 *   StreamPort <number>    a TCP port the server will listen to
 *   ServerRoot <path>      the server's root directory
 *   ServerUID <user>       the server will run as user id <user>
 *   UpdateThreads <num>    number of update threads to start
 *   UseBalance <thresh>    uses load-balancing to delegate clients
 *   UseKeepAlive           use keepalive on all sockets
 *   UseLinger <period>     linger for <period> seconds on all sockets
 *   UseNonBlock            use non-blocking IO on all sockets
 *   UseReuse               allow reuse of the socket port numbers
 *   ZoneSize <6 nums>      grid size and size of each zone sector
 *
 * All boolean options (UseNonBlock, UseKeepAlive, etc.) now accept
 * boolean arguments, i.e. "yes", "on", "true" are valid values
 * (case-insensitive), as are numbers >0.  A null or 0-length value is
 * also interpreted as true.  If it doesn't recognize the option
 * string according to the above rules, it is false.
 *
 * Comments can basically be anything that we don't explicitly
 * recognize.  Everything that doesn't fit these parameters is
 * ignored.
 *
 * Things to do
 *   - Consider how we might move module-specific configuration items
 *     into the modules themselves.  Perhaps an ini-like format, with the
 *     module in question (zone, database, etc.) being the section title.
 *     That way, we wouldn't need the huge array here, and each module
 *     that actually needs configuration items can just use the parsing
 *     primitives here, and handle everything it needs by itself.
 *   - Create command-line options for the config file elements that
 *     need them.  If the options come before the config file, they will
 *     probably be replaced, and if they come after, they should replace
 *     what was (possibly) read from the config file.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if HAVE_SYS_SYSLIMITS_H
#include <sys/syslimits.h>
#endif
#if HAVE_PWD_H
#include <pwd.h>
#endif /* HAVE_PWD_H */
#if HAVE_GRP_H
#include <grp.h>
#endif /* HAVE_GRP_H */
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#if HAVE_SYSLOG_H
#include <syslog.h>
#endif /* HAVE_SYSLOG_H */
#include <errno.h>

#include <fstream>
#include <sstream>

#include "config_data.h"
#include "server.h"
#include "log.h"

#define ENTRIES(x)  (sizeof(x) / sizeof(x[0]))

const int config_data::LINGER_LEN     = 0;
const int config_data::LOG_FACILITY   = LOG_DAEMON;
const int config_data::MIN_SUBSERV    = 1;
const int config_data::MAX_SUBSERV    = 250;
const int config_data::NUM_THREADS    = 8;
const int config_data::ZONE_SIZE      = 1000;
const int config_data::ZONE_STEPS     = 2;
const float config_data::LOAD_THRESH  = 0.75;
const char config_data::SERVER_ROOT[] = "/Users/tquirk/src/revision9/server";
const char config_data::LOG_PREFIX[]  = "revision9";
const char config_data::PID_FNAME[]   = "/var/run/revision9.pid";
const char config_data::DB_TYPE[]     = "MySQL";
const char config_data::DB_HOST[]     = "localhost";
const char config_data::DB_NAME[]     = "revision9";
const char config_data::ACTION_LIB[]  = "libr9_actions.so";

typedef void (*config_elem_t)(const std::string&, const std::string&, void *);

static void config_string_element(const std::string&, const std::string&, void *);
static void config_integer_element(const std::string&, const std::string&, void *);
static void config_float_element(const std::string&, const std::string&, void *);
static void config_boolean_element(const std::string&, const std::string&, void *);
static void config_user_element(const std::string&, const std::string&, void *);
static void config_group_element(const std::string&, const std::string&, void *);
static void config_logfac_element(const std::string&, const std::string&, void *);
static void config_socket_element(const std::string&, const std::string&, void *);
static void config_location_element(const std::string&, const std::string&, void *);

/* Global variables */
config_data config;

/* File-global variables */
const struct config_handlers
{
    const char *keyword;
    void *ptr;
    config_elem_t func;
}
handlers[] =
{
#define off(x)  (void *)(&(config.x))
    { "AccessThreads", off(access_threads), &config_integer_element  },
    { "ActionLib",     off(action_lib),     &config_string_element   },
    { "ActionThreads", off(action_threads), &config_integer_element  },
    { "ConFilename",   off(console_fname),  &config_string_element   },
    { "ConPort",       off(console_inet),   &config_integer_element  },
    { "DBDatabase",    off(db_name),        &config_string_element   },
    { "DBHost",        off(db_host),        &config_string_element   },
    { "DBPassword",    off(db_pass),        &config_string_element   },
    { "DBType",        off(db_type),        &config_string_element   },
    { "DBUser",        off(db_user),        &config_string_element   },
    { "DgramPort",     off(dgram),          &config_socket_element   },
    { "LogFacility",   off(log_facility),   &config_logfac_element   },
    { "LogPrefix",     off(log_prefix),     &config_string_element   },
    { "MaxSubservers", off(max_subservers), &config_integer_element  },
    { "MinSubservers", off(min_subservers), &config_integer_element  },
    { "MotionThreads", off(motion_threads), &config_integer_element  },
    { "PidFile",       off(pid_fname),      &config_string_element   },
    { "SendThreads",   off(send_threads),   &config_integer_element  },
    { "ServerGID",     NULL,                &config_group_element    },
    { "StreamPort",    off(stream),         &config_socket_element   },
    { "ServerRoot",    off(server_root),    &config_string_element   },
    { "ServerUID",     NULL,                &config_user_element     },
    { "SpawnPoint",    off(spawn),          &config_location_element },
    { "UpdateThreads", off(update_threads), &config_integer_element  },
    { "UseBalance",    off(load_threshold), &config_float_element    },
    { "UseKeepAlive",  off(use_keepalive),  &config_boolean_element  },
    { "UseLinger",     off(use_linger),     &config_integer_element  },
    { "UseNonBlock",   off(use_nonblock),   &config_boolean_element  },
    { "UseReuse",      off(use_reuse),      &config_boolean_element  },
    { "ZoneSize",      off(size),           &config_location_element },
#undef off
};

config_data::config_data()
    : argv(), stream(), dgram(),
      server_root(config_data::SERVER_ROOT),
      log_prefix(config_data::LOG_PREFIX), pid_fname(config_data::PID_FNAME),
      db_type(config_data::DB_TYPE), db_host(config_data::DB_HOST),
      db_user(), db_pass(), db_name(config_data::DB_NAME),
      action_lib(config_data::ACTION_LIB), console_fname()
{
    this->set_defaults();
}

config_data::~config_data()
{
    this->argv.clear();
    this->stream.clear();
    this->dgram.clear();
}

void config_data::set_defaults(void)
{
    this->server_root    = config_data::SERVER_ROOT;
    this->log_prefix     = config_data::LOG_PREFIX;
    this->pid_fname      = config_data::PID_FNAME;
    this->db_type        = config_data::DB_TYPE;
    this->db_host        = config_data::DB_HOST;
    this->db_user        = "";
    this->db_pass        = "";
    this->db_name        = config_data::DB_NAME;
    this->action_lib     = config_data::ACTION_LIB;
    this->console_fname  = "";

    this->use_keepalive  = false;
    this->use_nonblock   = false;
    this->use_reuse      = true;
    this->use_linger     = config_data::LINGER_LEN;

    this->log_facility   = config_data::LOG_FACILITY;

    this->load_threshold = config_data::LOAD_THRESH;
    this->min_subservers = config_data::MIN_SUBSERV;
    this->max_subservers = config_data::MAX_SUBSERV;

    this->access_threads = config_data::NUM_THREADS;
    this->action_threads = config_data::NUM_THREADS;
    this->motion_threads = config_data::NUM_THREADS;
    this->send_threads   = config_data::NUM_THREADS;
    this->update_threads = config_data::NUM_THREADS;

    this->size.dim[0]    = config_data::ZONE_SIZE;
    this->size.dim[1]    = config_data::ZONE_SIZE;
    this->size.dim[2]    = config_data::ZONE_SIZE;

    this->size.steps[0]  = config_data::ZONE_STEPS;
    this->size.steps[1]  = config_data::ZONE_STEPS;
    this->size.steps[2]  = config_data::ZONE_STEPS;
}

void setup_configuration(int count, char **args)
{
    /* We only want a total init when we pass an actual argv/argc in. */
    if (count && args)
        config.parse_command_line(count, args);
}

void cleanup_configuration(void)
{
    /* We'll leave argv alone because this may be a reinit */
    config.stream.clear();
    config.dgram.clear();
    config.set_defaults();

    chdir("/");
    seteuid(getuid());
    setegid(getgid());
}

void config_data::parse_command_line(int count, char **args)
{
    int i;
    std::vector<std::string>::iterator j;

    for (i = 0; i < count; ++i)
        this->argv.push_back(std::string(args[i]));

    /* getopt is great and everything, but it's a pretty low-level C
     * function, and even the C++-wrapped version works the same.  We
     * want something that operates more in the C++ idiom.
     */
    for (j = this->argv.begin() + 1; j != this->argv.end(); ++j)
    {
        if ((*j) == "-f")
            this->read_config_file(*(++j));
        else
            std::clog << "WARNING: Unknown option " << *j;
    }
}

void config_data::read_config_file(std::string& fname)
{
    std::ifstream ifs(fname);
    std::string str;

    do
    {
        std::getline(ifs, str);
        this->parse_config_line(str);
    }
    while (!ifs.eof());
}

void config_data::parse_config_line(std::string& line)
{
    int i;
    std::string::size_type found;

    /* Throw away any comments to the end of the line */
    if ((found = line.find_first_of('#')) != std::string::npos)
        line.replace(found, std::string::npos, "");

    /* Throw away all extra white space at the beginning and end of the line. */
    if ((found = line.find_last_not_of(" \t")) != std::string::npos)
        line = line.substr(0, found + 1);
    if ((found = line.find_first_not_of(" \t")) != std::string::npos)
        line = line.substr(found);

    /* At this point we'll either have an empty string, a string of
     * only whitespace, or "<keyword> <value>"
     */
    if (line.find_first_not_of(" \t") == std::string::npos)
        return;

    /* We've got a real line */
    found = line.find_first_of(" \t");
    std::string keyword = line.substr(0, found);
    line.replace(0, found, "");
    found = line.find_first_not_of(" \t");
    std::string value = line.substr(found);

    for (i = 0; i < ENTRIES(handlers); ++i)
        if (keyword == handlers[i].keyword)
            (*(handlers[i].func))(keyword, value, handlers[i].ptr);

    /* Silently ignore anything we don't otherwise recognize. */
}

static void config_string_element(const std::string& key,
                                  const std::string& value,
                                  void *ptr)
{
    std::string *element = (std::string *)ptr;

    if (value.size() > 0)
        *element = value;
}

static void config_integer_element(const std::string& key,
                                   const std::string& value,
                                   void *ptr)
{
    int *element = (int *)ptr;

    *element = std::stoi(value);
}

static void config_float_element(const std::string& key,
                                 const std::string& value,
                                 void *ptr)
{
    float *element = (float *)ptr;

    *element = std::stof(value);
}

static void config_boolean_element(const std::string& key,
                                   const std::string& value,
                                   void *ptr)
{
    bool *element = (bool *)ptr;

    if (value == ""
        || value == "yes"
        || value == "true"
        || value == "on"
        || std::stoi(value) > 0)
        *element = true;
    else
        *element = false;
}

/* ARGSUSED */
static void config_user_element(const std::string& key,
                                const std::string& value,
                                void *ptr)
{
    struct passwd *up;

    if ((up = getpwnam(value.c_str())) != NULL)
        seteuid(up->pw_uid);
    else
        std::clog << "ERROR: couldn't find user " << value << std::endl;
}

/* ARGSUSED */
static void config_group_element(const std::string& key,
                                 const std::string& value,
                                 void *ptr)
{
    struct group *gp;

    if ((gp = getgrnam(value.c_str())) != NULL)
        setegid(gp->gr_gid);
    else
        std::clog << "ERROR: couldn't find group " << value << std::endl;
}

static void config_logfac_element(const std::string& key,
                                  const std::string& value,
                                  void *ptr)
{
    int *element = (int *)ptr;

    if (value == "auth")           *element = LOG_AUTH;
    else if (value == "authpriv")  *element = LOG_AUTHPRIV;
    else if (value == "cron")      *element = LOG_CRON;
    else if (value == "daemon")    *element = LOG_DAEMON;
    else if (value == "kern")      *element = LOG_KERN;
    else if (value == "local0")    *element = LOG_LOCAL0;
    else if (value == "local1")    *element = LOG_LOCAL1;
    else if (value == "local2")    *element = LOG_LOCAL2;
    else if (value == "local3")    *element = LOG_LOCAL3;
    else if (value == "local4")    *element = LOG_LOCAL4;
    else if (value == "local5")    *element = LOG_LOCAL5;
    else if (value == "local6")    *element = LOG_LOCAL6;
    else if (value == "local7")    *element = LOG_LOCAL7;
    else if (value == "lpr")       *element = LOG_LPR;
    else if (value == "mail")      *element = LOG_MAIL;
    else if (value == "news")      *element = LOG_NEWS;
    else if (value == "syslog")    *element = LOG_SYSLOG;
    else if (value == "user")      *element = LOG_USER;
    else if (value == "uucp")      *element = LOG_UUCP;
    else
        std::clog << "Unknown facility (" << value << ") for "
                  << key << std::endl;
}

/* ARGSUSED */
static void config_socket_element(const std::string& key,
                                  const std::string& value,
                                  void *ptr)
{
    std::vector<int> *element = (std::vector<int> *)ptr;

    element->push_back(std::stoi(value));
}

/* ARGSUSED */
static void config_location_element(const std::string& key,
                                    const std::string& value,
                                    void *ptr)
{
    location *element = (location *)ptr;
    std::istringstream iss(value);

    iss >> element->dim[0];
    iss >> element->dim[1];
    iss >> element->dim[2];
    iss >> element->steps[0];
    iss >> element->steps[1];
    iss >> element->steps[2];
}