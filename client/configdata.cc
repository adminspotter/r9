/* configdata.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 25 Jul 2019, 08:39:13 tquirk
 *
 * Revision IX game client
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
 * This file contains the basic config-file handling for the Revision IX
 * client program.
 *
 * A good chunk of this is copied from the server program's configuration
 * setting stuff, with file writing added.  I thought of having a library,
 * but that's just too much work for now.
 *
 * To add a new configuration value, the only thing that needs to be updated
 * is the ctable variable.  Nice.
 *
 * Things to do
 *
 */

#include <config.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_STDDEF_H
#include <stddef.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#include <errno.h>

#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "configdata.h"
#include "l10n.h"

#include <proto/key.h>

#define ENTRIES(x)  (sizeof(x) / sizeof(x[0]))

const int ConfigData::SERVER_PORT    = 8500;
const char ConfigData::SERVER_ADDR[] = "127.0.0.1";
const char *ConfigData::FONT_PATHS[] = {
    /* MacOS paths */
    "~/Library/Fonts",
    "/Library/Fonts",
    "/Network/Library/Fonts",
    "/System/Library/Fonts",
    "/System/Folder/Fonts",
    /* Old-school Linux paths */
    "/usr/share/fonts",
    "/usr/share/fonts/default/Type1",
    "/usr/share/fonts/default/ttf",
    /* Debian paths */
    "/usr/share/fonts/opentype",
    "/usr/share/fonts/truetype",
    "/usr/share/fonts/type1",
    NULL
};

typedef void (*config_read_t)(const std::string&, const std::string&, void *);
typedef void (*config_write_t)(std::ostream&, void *);

static void read_string(const std::string&, const std::string&, void *);
static void read_paths(const std::string&, const std::string&, void *);
static void read_integer(const std::string&, const std::string&, void *);
static void write_string(std::ostream&, void *);
static void write_paths(std::ostream&, void *);
static void write_integer(std::ostream&, void *);

/* File-global variables */
const struct config_handlers
{
    const char *keyword;
    size_t offset;
    config_read_t rd_func;
    config_write_t wr_func;
}
handlers[] =
{
#define off(x)  offsetof(ConfigData, x)
    { "ServerAddr", off(server_addr), &read_string,  &write_string  },
    { "ServerPort", off(server_port), &read_integer, &write_integer },
    { "Username",   off(username),    &read_string,  &write_string  },
    { "Charname",   off(charname),    &read_string,  &write_string  },
    { "FontName",   off(font_name),   &read_string,  &write_string  },
    { "FontPaths",  off(font_paths),  &read_paths,   &write_paths   },
    { "KeyFile",    off(key_fname),   &read_string,  &write_string  }
#undef off
};

ConfigData::ConfigData()
    : argv(), config_dir(), config_fname(), key_fname(),
      server_addr(ConfigData::SERVER_ADDR), username(), charname()
{
    this->set_defaults();
}

ConfigData::~ConfigData()
{
    if (this->priv_key != NULL)
        EVP_PKEY_free(this->priv_key);
    this->argv.clear();
}

void ConfigData::set_defaults(void)
{
    char **ptr = (char **)&(ConfigData::FONT_PATHS[0]);
    std::string str;

    this->config_dir   = getenv("HOME");
    this->config_dir   += "/.r9";
    this->config_fname = this->config_dir + "/config";

    this->server_addr  = ConfigData::SERVER_ADDR;
    this->server_port  = ConfigData::SERVER_PORT;
    this->username     = "";
    this->charname     = "";

    this->font_paths.clear();
    while (*ptr != NULL)
    {
        struct stat state;

        str = *ptr;
        if (stat(str.c_str(), &state) == 0)
            this->font_paths.push_back(str);
        ++ptr;
    }

    this->key_fname = this->config_dir + "/key";
    this->priv_key = NULL;
    memset(this->pub_key, 0, sizeof(this->pub_key));
}

void ConfigData::parse_command_line(int count, const char **args)
{
    int i;
    std::vector<std::string>::iterator j;

    if (count > 0)
    {
        for (i = 0; i < count; ++i)
            this->argv.push_back(std::string(args[i]));

        /* getopt is great and everything, but it's a pretty low-level C
         * function, and even the C++-wrapped version works the same.  We
         * want something that operates more in the C++ idiom.
         */
        for (j = this->argv.begin() + 1; j != this->argv.end(); ++j)
        {
            if ((*j) == "-c")
            {
                this->config_dir = *(++j);
                this->config_fname = this->config_dir + "/config";
            }
            else if ((*j) == "-f")
                this->config_fname = *(++j);
            else
                std::clog << _("WARNING: Unknown option ") << *j;
        }
    }
    this->make_config_dirs();
    try { this->read_config_file(); }
    catch (std::ios_base::failure& e)
    {
#if HAVE_IOS_BASE_FAILURE_CODE
        /* No idea if we'll actually get errno values here */
        if (e.code().value() == ENOENT)
#else
        struct stat state;

        if (stat(this->config_fname.c_str(), &state) && errno == ENOENT)
#endif /* HAVE_IOS_BASE_FAILURE_CODE */
            /* Nothing there; write a default config file */
            this->write_config_file();
        else
            throw;
    }
}

void ConfigData::read_config_file(void)
{
    std::ifstream ifs(this->config_fname);
    std::string str, pristine_str;

    while (ifs.good())
    {
        std::getline(ifs, str);
        pristine_str = str;
        try { this->parse_config_line(str); }
        catch (std::out_of_range& e)
        {
            std::clog << _("Skipping bad config line: ")
                      << '"' << pristine_str << '"' << std::endl;
        }
    }
}

void ConfigData::write_config_file(void)
{
    /* We want to create the file without any group or world perms */
    mode_t oldmask = umask(S_IRWXG | S_IRWXO);
    std::ofstream ofs(this->config_fname,
                      std::ofstream::out | std::ofstream::trunc);
    int i;

    umask(oldmask);
    for (i = 0; i < ENTRIES(handlers); ++i)
    {
        char pchar = ofs.fill(' ');
        std::streamsize psize = ofs.width(30);
        std::ofstream::fmtflags pflag = ofs.setf(std::ofstream::left);
        ofs << handlers[i].keyword;
        ofs.fill(pchar);
        ofs.width(psize);
        ofs.setf(pflag);
        handlers[i].wr_func(ofs, ((char *)this) + handlers[i].offset);
        ofs << std::endl;
    }
    ofs.close();
}

bool ConfigData::read_crypto_key(const std::string& passphrase)
{
    if (this->key_fname.size())
    {
        int i = 0;
        unsigned char pp[passphrase.size() + 1];

        for (char c : passphrase)
            pp[i++] = (unsigned char)c;
        pp[i] = 0;
        this->priv_key = file_to_pkey(this->key_fname.c_str(), pp);
    }
    return this->priv_key != NULL;
}

void ConfigData::make_config_dirs(void)
{
    std::string dirname = this->config_dir;

    if (mkdir(dirname.c_str(), 0700) == -1 && errno != EEXIST)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << _("Error creating r9 preferences directory ") << dirname << ": "
          << err << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }

    /* Let's make sure we have a few other dirs we'll need */
    std::string subdirname(dirname + "/texture");
    if (mkdir(subdirname.c_str(), 0700) == -1 && errno != EEXIST)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << _("Can't make config directory ") << subdirname << ": "
          << err << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }

    subdirname = dirname + "/geometry";
    if (mkdir(subdirname.c_str(), 0700) == -1 && errno != EEXIST)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << _("Can't make config directory ") << subdirname << ": "
          << err << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }

    subdirname = dirname + "/sound";
    if (mkdir(subdirname.c_str(), 0700) == -1 && errno != EEXIST)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << _("Can't make config directory ") << subdirname << ": "
          << err << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }
}

void ConfigData::parse_config_line(std::string& line)
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
        {
            (*(handlers[i].rd_func))(keyword, value, ((char *)this) + handlers[i].offset);
            break;
        }

    /* Silently ignore anything we don't otherwise recognize. */
}

static void read_string(const std::string& key,
                        const std::string& value,
                        void *ptr)
{
    std::string *element = (std::string *)ptr;

    if (value.size() > 0)
        *element = value;
}

static void read_paths(const std::string& key,
                       const std::string& value,
                       void *ptr)
{
    std::vector<std::string> *element = (std::vector<std::string> *)ptr;
    std::string str = (std::string&)value;
    std::vector<std::string> paths;
    std::string::size_type pos;
    struct stat state;

    /* Split things on : */
    element->clear();
    boost::split(paths, str, [](char c){ return c == ':'; });
    for (auto path : paths)
    {
        if ((pos = path.find('~')) == 0)
        {
            char *home;

            if ((home = getenv("HOME")) == NULL)
                throw std::runtime_error("Could not find home directory");
            path.replace(pos, 1, home);
        }
        if (stat(path.c_str(), &state) != -1)
            element->push_back(path);
    }
}

static void read_integer(const std::string& key,
                         const std::string& value,
                         void *ptr)
{
    int *element = (int *)ptr;

    *element = std::stoi(value);
}

static void write_string(std::ostream& os, void *ptr)
{
    os << *((std::string *)ptr);
}

static void write_paths(std::ostream& os, void *ptr)
{
    std::vector<std::string> *element = (std::vector<std::string> *)ptr;
    std::vector<std::string>::iterator i;
    std::string str;

    for (i = element->begin(); i != element->end(); ++i)
    {
        if (str.size())
            str += ':';
        str += *i;
    }
    os << str;
}

static void write_integer(std::ostream& os, void *ptr)
{
    os << *((uint16_t *)ptr);
}
