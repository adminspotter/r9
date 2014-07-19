/* config.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 12 Sep 2013, 14:24:28 trinity
 *
 * Revision IX game client
 * Copyright (C) 2002  Trinity Annabelle Quirk
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
 * Changes
 *   31 Jul 2006 TAQ - Created the file.  Holy crap, these void pointers are
 *                     some crazy stuff.
 *   01 Aug 2006 TAQ - Added modified element to the config structure, to
 *                     keep track of whether we actually need to save or not.
 *   12 Sep 2013 TAQ - Added an include for USHRT_MAX.
 *
 * Things to do
 *
 * $Id: config.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <netdb.h>
#include <errno.h>

#include "client.h"

#define ENTRIES(x)  (sizeof(x) / sizeof(x[0]))
#define ELEMENT(x)  &(((char *)&config)[x])

#define CF_STRING   1
#define CF_INTEGER  2
#define CF_FLOAT    3
#define CF_BOOLEAN  4
#define CF_ADDR     5
#define CF_PORT     6

static int make_config_dirs(const char *);
static void setup_config_defaults(void);
static int parse_config_line(char *);
static void config_string_element(const char *, const char *, int, void *);
static void config_integer_element(const char *, const char *, int, void *);
static void config_float_element(const char *, const char *, int, void *);
static void config_boolean_element(const char *, const char *, int, void *);
static void config_addr_element(const char *, const char *, int, void *);
static void config_port_element(const char *, const char *, int, void *);

struct config_struct_tag config;

/* File-global variables */
const struct config_handlers
{
    char *keyword;
    int offset;
    int type;
    void *defval;
}
ctable[] =
{
#define off(x)  (((char *) (&(((struct config_struct_tag *)NULL)->x))) - ((char *) NULL))
    /* Rememeber:  floats are * 100 */
    { "ServerAddr", off(server_addr), CF_ADDR,   "192.168.100.2" },
    { "ServerPort", off(server_port), CF_PORT,   (void *)8500    },
    { "Username",   off(username),    CF_STRING, NULL            },
    { "Password",   off(password),    CF_STRING, NULL            }
#undef off
};

void load_settings(void)
{
    char fname[PATH_MAX], errstr[256];
    struct stat state;

    /* First check if there's a settings file that exists */
    strncpy(fname, getenv("HOME"), sizeof(fname));
    strncat(fname, "/.revision9", sizeof(fname) - strlen(fname) - 1);
    if (make_config_dirs(fname))
        /* There was some sort of error; just bail out now */
        return;
    /* The directory is there.  Now is the config file? */
    strncat(fname, "/config", sizeof(fname) - strlen(fname) - 1);
    if (stat(fname, &state) == -1)
    {
        if (errno == ENOENT)
        {
            mode_t oldmask;

            /* The file isn't there, so let's make a new one */
            snprintf(errstr, sizeof(errstr),
                     "Making a new config file %s", fname);
            /* Set a private mask for creation - there may be passwords
             * in this file.
             */
            oldmask = umask(S_IRWXG | S_IRWXO);
            setup_config_defaults();
            write_config_file(fname);
            umask(oldmask);
        }
        else
        {
            snprintf(errstr, sizeof(errstr),
                     "Error with config file %s: %s",
                     fname, strerror(errno));
            main_post_message(errstr);
            /* Set up the defaults anyway - we need *something* to work with */
            setup_config_defaults();
        }
    }
    else
        read_config_file(fname);
}

void save_settings(void)
{
    char fname[PATH_MAX];

    if (!config.modified)
        return;

    strncpy(fname, getenv("HOME"), sizeof(fname));
    strncat(fname, "/.revision9/config", sizeof(fname) - strlen(fname) - 1);
    write_config_file(fname);
}

static int make_config_dirs(const char *dirname)
{
    struct stat state;
    char subdirname[PATH_MAX], errstr[256];

    if (stat(dirname, &state) == -1)
    {
        if (errno == ENOENT)
        {
            /* The directory doesn't exist, so make it; we'll use it for
             * things other than just the config file.  And there may
             * be passwords stored in there, so let's give it a private
             * mask, for at least a little security.
             */
            if (mkdir(dirname, 0700) == -1)
            {
                /* We can't even create the dir.  I think we have to
                 * give up at this point.
                 */
                snprintf(errstr, sizeof(errstr),
                         "Can't make config directory %s: %s",
                         dirname, strerror(errno));
                main_post_message(errstr);
                return errno;
            }
        }
        else
        {
            /* It's some error that we can't deal with */
            snprintf(errstr, sizeof(errstr),
                     "Error with config dir %s: %s",
                     dirname, strerror(errno));
            main_post_message(errstr);
            return errno;
        }
    }

    /* Let's make sure we have a few other dirs we'll need */
    strncpy(subdirname, dirname, sizeof(subdirname));
    strncat(subdirname, "/texture",
            sizeof(subdirname) - strlen(dirname));
    if (stat(subdirname, &state) == -1 && errno == ENOENT
        && mkdir(subdirname, 0700) == -1)
    {
        snprintf(errstr, sizeof(errstr),
                 "Can't make config directory %s: %s",
                 subdirname, strerror(errno));
        main_post_message(errstr);
        return errno;
    }

    strncpy(subdirname, dirname, sizeof(subdirname));
    strncat(subdirname, "/geometry",
            sizeof(subdirname) - strlen(dirname));
    if (stat(subdirname, &state) == -1 && errno == ENOENT
        && mkdir(subdirname, 0700) == -1)
    {
        snprintf(errstr, sizeof(errstr),
                 "Can't make config directory %s: %s",
                 subdirname, strerror(errno));
        main_post_message(errstr);
        return errno;
    }

    strncpy(subdirname, dirname, sizeof(subdirname));
    strncat(subdirname, "/sound",
            sizeof(subdirname) - strlen(dirname));
    if (stat(subdirname, &state) == -1 && errno == ENOENT
        && mkdir(subdirname, 0700) == -1)
    {
        snprintf(errstr, sizeof(errstr),
                 "Can't make config directory %s: %s",
                 subdirname, strerror(errno));
        main_post_message(errstr);
        return errno;
    }

    return 0;
}

static void setup_config_defaults(void)
{
    int i;

    for (i = 0; i < ENTRIES(ctable); ++i)
    {
        void *element = &(((char *)&config)[ctable[i].offset]);

        switch (ctable[i].type)
        {
          case CF_STRING:
            *((char **)element) = (char *)(ctable[i].defval);
            break;

          case CF_INTEGER:
          case CF_BOOLEAN:
            *((int *)element) = (int)(ctable[i].defval);
            break;

          case CF_FLOAT:
            *((float *)element) = (float)((int)(ctable[i].defval) / 100.0);
            break;

          case CF_ADDR:
            inet_aton((char *)ctable[i].defval, (struct in_addr *)element);
            break;

          case CF_PORT:
            *((short int *)element) = htons((int)(ctable[i].defval));
            break;

          default:
            break;
        }
    }
    config.modified = 1;
}

void write_config_file(const char *fname)
{
    FILE *f;
    struct hostent *he;
    int i;

    if (!config.modified)
        /* If nothing is changed, don't bother saving */
        return;

    if ((f = fopen(fname, "w")) == NULL)
    {
        fprintf(stderr,
                 "Couldn't open config file %s for writing: %s\n",
                 fname, strerror(errno));
        return;
    }
    for (i = 0; i < ENTRIES(ctable); ++i)
    {
        void *element = &(((char *)&config)[ctable[i].offset]);

        if (ctable[i].type == CF_STRING && *(char **)element == NULL)
            continue;

        fprintf(f, "%s\t", ctable[i].keyword);
        switch (ctable[i].type)
        {
          case CF_STRING:
            fprintf(f, "%s", *(char **)element);
            break;

          case CF_INTEGER:
            fprintf(f, "%d", *((int *)element));
            break;

          case CF_FLOAT:
            fprintf(f, "%f", *((float *)element));
            break;

          case CF_BOOLEAN:
            fprintf(f, "%d", *((int *)element));
            break;

          case CF_ADDR:
            /* Convert to a hostname if possible before writing */
            if ((he = gethostbyaddr((struct in_addr *)element,
                                    sizeof(struct in_addr),
                                    AF_INET)) != NULL)
                fprintf(f, "%s", he->h_name);
            else
                /* Convert failed; just print the IP address */
                fprintf(f, "%s", inet_ntoa(*((struct in_addr *)element)));
            break;

          case CF_PORT:
            fprintf(f, "%d", ntohs((short int)*((int *)element)));
            break;

          default:
            break;
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

void read_config_file(const char *fname)
{
    FILE *f;
    char str[PATH_MAX];

    /* There is a config file, so let's read it */
    if ((f = fopen(fname, "r")) != NULL)
    {
        while (fgets(str, sizeof(str), f) != NULL)
            parse_config_line(str);
        fclose(f);
        config.modified = 0;
    }
    else
    {
        snprintf(str, sizeof(str),
                 "Couldn't open config file %s for reading: %s",
                 fname, strerror(errno));
        main_post_message(str);
    }
}

static int parse_config_line(char *line)
{
    char *head, *tail;
    int i, retval = 1;

    /* Throw away all extra white space at the beginning of the line. */
    head = line + strspn(line, " \t");
    if (*head != '#')
        for (i = 0; i < ENTRIES(ctable); ++i)
            if (!strncmp(ctable[i].keyword,
                         head,
                         strlen(ctable[i].keyword)))
            {
                /* Move past the option name and following whitespace. */
                head += strlen(ctable[i].keyword);
                head += strspn(head, " \t");
                if ((tail = strrchr(head, '\n')) != NULL)
                    *tail = '\0';
                switch (ctable[i].type)
                {
                  case CF_STRING:
                    config_string_element(head,
                                          ctable[i].keyword,
                                          ctable[i].offset,
                                          ctable[i].defval);
                    break;

                  case CF_INTEGER:
                    config_integer_element(head,
                                           ctable[i].keyword,
                                           ctable[i].offset,
                                           ctable[i].defval);
                    break;

                  case CF_BOOLEAN:
                    config_boolean_element(head,
                                           ctable[i].keyword,
                                           ctable[i].offset,
                                           ctable[i].defval);
                    break;

                  case CF_FLOAT:
                    config_float_element(head,
                                         ctable[i].keyword,
                                         ctable[i].offset,
                                         ctable[i].defval);
                    break;

                  case CF_ADDR:
                    config_addr_element(head,
                                        ctable[i].keyword,
                                        ctable[i].offset,
                                        ctable[i].defval);
                    break;

                  case CF_PORT:
                    config_port_element(head,
                                        ctable[i].keyword,
                                        ctable[i].offset,
                                        ctable[i].defval);
                    break;

                  default:
                    break;
                }
                retval = 0;
                break;
            }
    /* Silently ignore anything we don't otherwise recognize. */
    return retval;
}

static void config_string_element(const char *str,
                                  const char *item,
                                  int offset,
                                  void *defval)
{
    char **element = (char **)ELEMENT(offset);
    char errstr[PATH_MAX];

    if (str != NULL && strlen(str) > 0)
    {
        if (*element != NULL
            && (defval != NULL && *element != (char *)defval))
            free(*element);
        *element = strdup(str);
    }
    else
    {
        snprintf(errstr, sizeof(errstr),
                 "Null %s, using %s", item, (char *)defval);
        main_post_message(errstr);
        *element = (char *)defval;
    }
}

static void config_integer_element(const char *str,
                                   const char *item,
                                   int offset,
                                   void *defval)
{
    int *element = (int *)ELEMENT(offset);
    char errstr[PATH_MAX];

    if ((*element = atoi(str)) < 1 || *element > USHRT_MAX)
    {
        snprintf(errstr, sizeof(errstr),
                 "Invalid value (%d) for %s, using %d",
                 *element, item, (int)defval);
        main_post_message(errstr);
        *element = (int)defval;
    }
}

static void config_float_element(const char *str,
                                 const char *item,
                                 int offset,
                                 void *defval)
{
    float *element = (float *)ELEMENT(offset);
    float realdefval = (float)((int)defval / 100.0);
    char errstr[PATH_MAX];

    if ((*element = atof(str)) <= 0.0)
    {
        snprintf(errstr, sizeof(errstr),
                 "Invalid value (%f) for %s, using %f",
                 *element, item, realdefval);
        main_post_message(errstr);
        *element = realdefval;
    }
}

static void config_boolean_element(const char *str,
                                   const char *item,
                                   int offset,
                                   void *defval)
{
    int *element = (int *)ELEMENT(offset);

    if (str == NULL
        || strlen(str) == 0
        || !strcasecmp(str, "yes")
        || !strcasecmp(str, "true")
        || !strcasecmp(str, "on")
        || atoi(str) > 0)
        *element = 1;
    else
        *element = 0;
}

static void config_addr_element(const char *str,
                                const char *item,
                                int offset,
                                void *defval)
{
    struct in_addr *element = (struct in_addr *)ELEMENT(offset);
    struct hostent *he;
    char errstr[PATH_MAX];

    /* Our default value is a string */
    if ((he = gethostbyname(str)) == NULL)
    {
        snprintf(errstr, sizeof(errstr),
                 "Invalid host address (%s) for %s, using %s",
                 str, item, (char *)defval);
        main_post_message(errstr);
        he = gethostbyname((char *)defval);
    }
    memcpy(element, *(he->h_addr_list), sizeof(struct in_addr));
}

/* ARGSUSED */
static void config_port_element(const char *str,
                                const char *item,
                                int offset,
                                void *defval)
{
    short int *element = (short int *)ELEMENT(offset);

    *element = htons((short int)atoi(str));
}
