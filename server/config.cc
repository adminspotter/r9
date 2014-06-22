/* config.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 21 Jun 2014, 17:19:07 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2014  Trinity Annabelle Quirk
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
 *   ConHistFile <string>   filename of the console history
 *   ConHistLen <num>       length of the console history file
 *   ConPort <num>          port of the inet console
 *   DBDatabase <dbname>    the name of the database to use
 *   DBHost <host>          database server hostname
 *   DBPassword <password>  the unencrypted password to get into the database
 *   DBType <type>          which database to use - mysql, pgsql, etc.
 *   DBUser <username>      the database username
 *   DgramPort <number>     a UDP port the server will listen to
 *   GeomThreads <num>      number of geometry threads to start
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
 *   [XYZ]Dimension <num>   size of the basic chunk of space, in meters
 *   [XYZ]Steps <num>       number of chunks of space in a given direction
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
 * Changes
 *   28 Mar 1998 TAQ - Created the file.
 *   29 Mar 1998 TAQ - Modified the NUM_ELEMENTS macro.  Added UseLinger.
 *                     Fleshed out the Server[UG]ID and ServerPort routines.
 *                     ServerRoot is no longer saved, just used and thrown
 *                     away.  Same with Server[UG]ID.  Error checking in
 *                     config_server_port.
 *   11 Apr 1998 TAQ - Moved the chunk allocation stuff into a header file.
 *                     Added a few variables and a header file.
 *   12 Apr 1998 TAQ - Reset use_* variables in cleanup_configuration.
 *   17 Apr 1998 TAQ - Miscellaneous fixes so that it compiles.
 *   10 May 1998 TAQ - Added CVS ID string.
 *   21 Oct 1998 TAQ - Added some load-balancing config options.  Changed
 *                     the linger option to actually take an argument.
 *   13 Feb 1999 TAQ - Improved the parsing of the config files.  We now
 *                     ignore leading whitespace, and ignore any amount
 *                     and kind of whitespace between the option identi-
 *                     fier and the option data.  Moved all the config
 *                     data into a struct.
 *   20 Feb 1999 TAQ - Fixed improper initialization/reinitialization of
 *                     the config struct.  Improved some comments.
 *   23 Feb 1999 TAQ - Added database configuration options.
 *   30 May 1999 TAQ - Tweaked process_config_file a bit; we now explicitly
 *                     ignore comments; a few other minor changes.  Added
 *                     a log action for unrecognized command-line options.
 *   06 Oct 1999 TAQ - Added stream/datagram selection in the config.
 *   16 Apr 2000 TAQ - Reset the CVS ID string.
 *   21 Apr 2000 TAQ - stdio.h wasn't included before?  How bizarre!
 *                     string.h too?
 *   06 Jun 2000 TAQ - Added NumBuffers for number of receive buffers.
 *   29 Jun 2000 TAQ - Added (Min|Max)OctreeDepth.
 *   12 Jul 2000 TAQ - Added MaxOctreePolys.  Changed min_octree_depth
 *                     to depend on a defined constant instead of a
 *                     hardcoded one.
 *   13 Jul 2000 TAQ - Added log_prefix option.
 *   26 Jul 2000 TAQ - All the boolean options now support actual boolean
 *                     values, thanks to parse_bool.  Added use of defined
 *                     constants as compiled-in defaults instead of hard
 *                     coded values.  Error reporting on config_server_root.
 *                     Default values in most of the DB functions.  When
 *                     these routines are run, we have neither opened the
 *                     syslog or detached from the controlling terminal yet,
 *                     so we now print all errors and warnings to stderr.
 *   27 Jul 2000 TAQ - Fixed cleanup so that it'll free all allocated
 *                     memory.  Small setup tweak, which may have problems
 *                     on reinitialization.  Reordered some comments.  Moved
 *                     cleanup_configuration above process_config_file.
 *   24 Aug 2000 TAQ - Fixed a typo in config_use_reuse.  Added
 *                     setup_config_defaults routine to set default values,
 *                     so we don't have to do bunches of checks when we
 *                     actually try to use the values.
 *   27 Aug 2000 TAQ - When parsing the file, the input string is now
 *                     PATH_MAX long, instead of 256 bytes.  Freeing static
 *                     strings generates seg-faults, so we now check all
 *                     config strings against the compiled-in defaults.
 *                     We now break out of the inner file processing loop
 *                     once we process the command we found.
 *   29 Sep 2000 TAQ - Added [xyz]_dim, [xyz]_steps, and zone_fname processing.
 *                     Added (action|control)_(lib_dir|register_func) and
 *                     libpath processing.
 *   02 Oct 2000 TAQ - Added lib_suffix processing.
 *   28 Oct 2000 TAQ - Added action and control unregistration function names
 *                     and options for the zone file and un/register function.
 *   04 Nov 2000 TAQ - Added console option processing.  There will be no
 *                     default values for those options; if they aren't
 *                     configured explicitly, they won't be used at all.
 *   29 Jan 2002 TAQ - Moved the line-by-line processing of the config files
 *                     into its own routine, since the console driver will
 *                     need to be able to configure things as well.  Also
 *                     added some routines for the console to be able to access
 *                     our static routines.  Altered user and group routines
 *                     to use getpwnam and getgrnam instead of doing it the
 *                     long way.  Adde log facility config options.
 *   14 Feb 2002 TAQ - Fixed the confusion surrounding the Zone-related
 *                     config options.
 *   30 Jun 2002 TAQ - Added frame sequence and polygon limit config options.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   14 Jun 2006 TAQ - Added thread config options.
 *   06 Jul 2006 TAQ - Changed dimensions to be u_int64_t values.
 *   26 Jul 2006 TAQ - Added access_threads and geom_threads.
 *   30 Jul 2006 TAQ - Since we had that nice structure, I modified it a little
 *                     and made it the complete controller of the config
 *                     process.  We no longer have config functions for each
 *                     element, but only for each type.  Removed the console
 *                     related stuff for the time being.
 *   01 Aug 2006 TAQ - Figured out how to get rid of those different-sized
 *                     pointer/int errors in the bigint stuff - we double cast.
 *                     It's weird, but I think the compiler understands it.
 *   13 Aug 2006 TAQ - Added DBType option.  Removed all the unused stuff.
 *   14 Aug 2006 TAQ - Removed unused stuff from the comments.
 *   17 Aug 2006 TAQ - Added ActionLib.
 *   21 Jul 2007 TAQ - I moved all the default values into defaults.h, so
 *                     we need to include it here.
 *   21 Aug 2007 TAQ - Added NumUsers.
 *   23 Aug 2007 TAQ - Made the server ports open-ended; added DgramPort and
 *                     StreamPort keywords and handling for them.  Removed
 *                     UseDatagram and UseStream, since they're no longer
 *                     meaningful.
 *   17 Sep 2007 TAQ - Fixed memory funk in socket config.
 *   23 Sep 2007 TAQ - Removed num_users and num_buffers and jumptable
 *                     elements.  Removed UseDatagram and UseStream jumptable
 *                     elements.
 *   14 Oct 2007 TAQ - Added [xyz]_dim and [xyz]_steps.  Added
 *                     config_int16_element.  Renamed config_bigint_element
 *                     to config_int64_element and CF_BIGINT to CF_INT64.
 *   20 Oct 2007 TAQ - Added syslog.h include.  Added ZoneSize and SpawnPoint
 *                     config elements.  Added CF_LOC config type.  Removed
 *                     [XYZ]Dim and [XYZ]Steps elements.
 *   19 Sep 2013 TAQ - Added the console directives: ConFilename, ConPort,
 *                     ConHistFile, and ConHistLen.
 *   20 May 2014 TAQ - For some reason, the int16 and int64 stuff was all
 *                     commented out.
 *   21 Jun 2014 TAQ - C++-ification has begun, starting with the syslog.
 *
 * Things to do
 *   - Consider how we might move module-specific configuration items
 *     into the modules themselves.  Perhaps an ini-like format, with the
 *     module in question (zone, database, etc.) being the section title.
 *     That way, we wouldn't need the huge array here, and each module
 *     that actually needs configuration items can just use the parsing
 *     primitives here, and handle everything it needs by itself.
 *   - Create command-line options for all the config file elements.  When
 *     parsing these, make them supercede the config file settings.  Probably
 *     a second conf structure is needed for this trick.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <errno.h>

#include "config.h"
#include "server.h"
#include "defaults.h"
#include "log.h"

#define ENTRIES(x)  (sizeof(x) / sizeof(x[0]))
#define CF_STRING   1
#define CF_INTEGER  2
#define CF_FLOAT    3
#define CF_BOOLEAN  4
#define CF_USER     5
#define CF_GROUP    6
#define CF_LOGFAC   7
#define CF_SOCKET   8
#define CF_LOC      9
#define CF_INT16    10
#define CF_INT64    11

#ifndef FALSE
#define FALSE       0
#define TRUE        1
#endif /* FALSE */

/* Function prototypes */
static void setup_config_defaults(void);
static void read_config_file(const char *);
static int parse_config_line(char *);
static void config_string_element(const char *, const char *, int, void *);
static void config_integer_element(const char *, const char *, int, void *);
static void config_int16_element(const char *, const char *, int, void *);
static void config_int64_element(const char *, const char *, int, void *);
static void config_float_element(const char *, const char *, int, void *);
static void config_boolean_element(const char *, const char *, int, void *);
static void config_user_element(const char *, const char *, int, void *);
static void config_group_element(const char *, const char *, int, void *);
static void config_logfac_element(const char *, const char *, int, void *);
static void config_socket_element(const char *, const char *, int, void *);
static void config_location_element(const char *, const char *, int, void *);

/* Global variables */
config_data config;

/* File-global variables */
const struct config_handlers
{
    char *keyword;
    int offset;
    int type;
    void *defval;
}
handlers[] =
{
#define off(x)  (((char *) (&(((struct config_struct *)NULL)->x))) - ((char *) NULL))
    { "AccessThreads", off(access_threads), CF_INTEGER, (void *)NUM_THREADS  },
    { "ActionLib",     off(action_lib),     CF_STRING,  ACTION_LIB           },
    { "ActionThreads", off(action_threads), CF_INTEGER, (void *)NUM_THREADS  },
    { "ConFilename",   off(console_fname),  CF_STRING,  NULL                 },
    { "ConHistFile",   off(console_hist),   CF_STRING,  HIST_FILE            },
    { "ConHistLen",    off(history_len),    CF_INTEGER, (void *)1000         },
    { "ConPort",       off(console_inet),   CF_INTEGER, (void *)0            },
    { "DBDatabase",    off(db_name),        CF_STRING,  DB_NAME              },
    { "DBHost",        off(db_host),        CF_STRING,  DB_HOST              },
    { "DBPassword",    off(db_pass),        CF_STRING,  NULL                 },
    { "DBType",        off(db_type),        CF_STRING,  DB_TYPE              },
    { "DBUser",        off(db_user),        CF_STRING,  NULL                 },
    { "DgramPort",     off(dgram),          CF_SOCKET,  NULL                 },
    { "GeomThreads",   off(geom_threads),   CF_INTEGER, (void *)NUM_THREADS  },
    { "LogFacility",   off(log_facility),   CF_LOGFAC,  (void *)LOG_FACILITY },
    { "LogPrefix",     off(log_prefix),     CF_STRING,  LOG_PREFIX           },
    { "MaxSubservers", off(max_subservers), CF_INTEGER, (void *)MAX_SUBSRV   },
    { "MinSubservers", off(min_subservers), CF_INTEGER, (void *)MIN_SUBSRV   },
    { "MotionThreads", off(motion_threads), CF_INTEGER, (void *)NUM_THREADS  },
    { "PidFile",       off(pid_fname),      CF_STRING,  PID_FNAME            },
    { "SendThreads",   off(send_threads),   CF_INTEGER, (void *)NUM_THREADS  },
    { "ServerGID",     0,                   CF_GROUP,   NULL                 },
    { "StreamPort",    off(stream),         CF_SOCKET,  NULL                 },
    { "ServerRoot",    off(server_root),    CF_STRING,  SERVER_ROOT          },
    { "ServerUID",     0,                   CF_USER,    NULL                 },
    { "SpawnPoint",    off(spawn),          CF_LOC,     NULL                 },
    { "UpdateThreads", off(update_threads), CF_INTEGER, (void *)NUM_THREADS  },
    { "UseBalance",    off(load_threshold), CF_FLOAT,   (void *)LOAD_THRESH  },
    { "UseKeepAlive",  off(use_keepalive),  CF_BOOLEAN, (void *)FALSE        },
    { "UseLinger",     off(use_linger),     CF_INTEGER, (void *)LINGER_LEN   },
    { "UseNonBlock",   off(use_nonblock),   CF_BOOLEAN, (void *)FALSE        },
    { "UseReuse",      off(use_reuse),      CF_BOOLEAN, (void *)TRUE         },
    { "ZoneSize",      off(size),           CF_LOC,     NULL                 }
#undef off
};

void setup_configuration(int argc, char **argv)
{
    int c;
    char *tmp;

    /* We only want a total init when we pass an actual argv/argc in. */
    if (argc && argv)
    {
	memset(&config, 0, sizeof(config));
	config.argv = argv;
	config.argc = argc;
	setup_config_defaults();
    }

    /* Not much here; check into adding CLOs for the config options. */
    /* This might not work properly the second time around. */
    while ((c = getopt(config.argc, config.argv, "f:")) != EOF)
    {
	switch (c)
	{
	  case 'f':
	    tmp = optarg + strspn(optarg, " \t");
	    read_config_file(tmp);
	    break;

	  case '?':
	  default:
            std::clog << "WARNING: Unknown option " << c;
	    break;
	}
    }
}

void cleanup_configuration(void)
{
    int argc = config.argc;
    char **argv = config.argv;
    int i;

    for (i = 0; i < ENTRIES(handlers); ++i)
    {
	if (handlers[i].type == CF_STRING)
	{
	    char **element = (char **)&(((char *)&config)[handlers[i].offset]);

	    /* Free strings, but only if they're dynamically-allocated. */
	    if ((*element) != NULL && *element != (char *)(handlers[i].defval))
	    {
		free(*element);
		*element = NULL;
	    }
	}
	if (handlers[i].type == CF_SOCKET)
	{
	    ports *element = (ports *)&(((char *)&config)[handlers[i].offset]);

	    if (element->port_nums != NULL)
	    {
		free(element->port_nums);
		element->port_nums = NULL;
	    }
	    element->num_ports = 0;
	}
    }

    /* The rest of the cleanup. */
    memset(&config, 0, sizeof(config));
    chdir("/");
    seteuid(getuid());
    setegid(getgid());

    /* Save the initial arguments. */
    config.argc = argc;
    config.argv = argv;
}

static void setup_config_defaults(void)
{
    int i;

    for (i = 0; i < ENTRIES(handlers); ++i)
    {
	void *element = &(((char *)&config)[handlers[i].offset]);

	switch (handlers[i].type)
	{
	  case CF_STRING:
	    *((char **)element) = (char *)(handlers[i].defval);
	    break;

	  case CF_INTEGER:
	  case CF_BOOLEAN:
	  case CF_LOGFAC:
	    *((int *)element) = (int)(handlers[i].defval);
	    break;

	  case CF_INT16:
	    *((u_int16_t *)element) = (u_int16_t)(int)(handlers[i].defval);
	    break;

	  case CF_INT64:
	    *((u_int64_t *)element) = (u_int64_t)(int)(handlers[i].defval);
	    break;

	  case CF_FLOAT:
	    *((float *)element) = (float)((int)(handlers[i].defval) / 100.0);
	    break;

	  case CF_SOCKET:
	    ((ports *)element)->num_ports = 0;
	    ((ports *)element)->port_nums = NULL;
	    break;

	  case CF_USER:
	  case CF_GROUP:
	  case CF_LOC:
	  default:
	    break;
	}
    }
}

static void read_config_file(const char *fname)
{
    FILE *fp;
    char str[PATH_MAX];

    /* Open configuration files and do whatever is necessary. */
    if ((fp = fopen(fname, "r")) == NULL)
    {
        std::clog << "ERROR: couldn't open configuration file "
                  << fname << ": "
                  << strerror(errno) << " (" << errno << ")" << std::endl;
	exit(1);
    }

    while (fgets(str, sizeof(str), fp) != NULL)
	parse_config_line(str);
    fclose(fp);
}

static int parse_config_line(char *line)
{
    char *head, *tail;
    int i, retval = 1;

    /* Throw away all extra white space at the beginning of the line. */
    head = line + strspn(line, " \t");
    if (*head != '#')
	for (i = 0; i < ENTRIES(handlers); ++i)
	    if (!strncmp(handlers[i].keyword,
			 head,
			 strlen(handlers[i].keyword)))
	    {
		/* Move past the option name and following whitespace. */
		head += strlen(handlers[i].keyword);
		head += strspn(head, " \t");
		if ((tail = strrchr(head, '\n')) != NULL)
		    *tail = '\0';
		switch (handlers[i].type)
		{
		  case CF_STRING:
		    config_string_element(head,
					  handlers[i].keyword,
					  handlers[i].offset,
					  handlers[i].defval);
		    break;

		  case CF_INTEGER:
		    config_integer_element(head,
					   handlers[i].keyword,
					   handlers[i].offset,
					   handlers[i].defval);
		    break;

                  case CF_INT16:
		    config_int16_element(head,
					 handlers[i].keyword,
					 handlers[i].offset,
					 handlers[i].defval);
		    break;

		  case CF_INT64:
		    config_int64_element(head,
					 handlers[i].keyword,
					 handlers[i].offset,
					 handlers[i].defval);
		    break;

		  case CF_FLOAT:
		    config_float_element(head,
					 handlers[i].keyword,
					 handlers[i].offset,
					 handlers[i].defval);
		    break;

		  case CF_BOOLEAN:
		    config_boolean_element(head,
					   handlers[i].keyword,
					   handlers[i].offset,
					   handlers[i].defval);
		    break;

		  case CF_USER:
		    config_user_element(head,
					handlers[i].keyword,
					handlers[i].offset,
					handlers[i].defval);
		    break;

		  case CF_GROUP:
		    config_group_element(head,
					 handlers[i].keyword,
					 handlers[i].offset,
					 handlers[i].defval);
		    break;

		  case CF_LOGFAC:
		    config_logfac_element(head,
					  handlers[i].keyword,
					  handlers[i].offset,
					  handlers[i].defval);
		    break;

		  case CF_SOCKET:
		    config_socket_element(head,
					  handlers[i].keyword,
					  handlers[i].offset,
					  handlers[i].defval);
		    break;

		  case CF_LOC:
		    config_location_element(head,
					    handlers[i].keyword,
					    handlers[i].offset,
					    handlers[i].defval);
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
    char **element = (char **)&(((char *)&config)[offset]);

    if (str != NULL && strlen(str) > 0)
    {
	if (*element != NULL
	    && (defval != NULL && *element != (char *)defval))
	    free(*element);
	*element = strdup(str);
    }
    else
    {
        std::clog << "WARNING: null " << item << ", using " << (char *)defval
                  << std::endl;
	*element = (char *)defval;
    }
}

static void config_integer_element(const char *str,
				   const char *item,
				   int offset,
				   void *defval)
{
    int *element = (int *)&(((char *)&config)[offset]);

    if ((*element = atoi(str)) < 1 || *element > USHRT_MAX)
    {
        std::clog << "WARNING: invalid value (" << *element
                  << ") for " << item << ", using " << (int)defval << std::endl;
	*element = (int)defval;
    }
}

static void config_int16_element(const char *str,
				 const char *item,
				 int offset,
				 void *defval)
{
    u_int16_t *element = (u_int16_t *)&(((char *)&config)[offset]);

    if ((*element = (u_int16_t)atoi(str)) == 0)
    {
        std::clog << "WARNING: invalid value (" << *element
                  << ") for " << item << ", using " << (int)defval << std::endl;
	*element = (u_int16_t)(int)defval;
    }
}

static void config_int64_element(const char *str,
				 const char *item,
				 int offset,
				 void *defval)
{
    u_int64_t *element = (u_int64_t *)&(((char *)&config)[offset]);

    if ((*element = strtoull(str, NULL, 10)) == 0)
    {
        std::clog << "WARNING: invalid value (" << *element
                  << ") for " << item << ", using " << (int)defval << std::endl;
	*element = (u_int64_t)(int)defval;
    }
}

static void config_float_element(const char *str,
				 const char *item,
				 int offset,
				 void *defval)
{
    float *element = (float *)&(((char *)&config)[offset]);
    float realdefval = (float)((int)defval / 100.0);

    if ((*element = atof(str)) <= 0.0)
    {
        std::clog << "WARNING: invalid value (" << *element
                  << ") for " << item << ", using " << realdefval << std::endl;
	*element = realdefval;
    }
}

static void config_boolean_element(const char *str,
				   const char *item,
				   int offset,
				   void *defval)
{
    int *element = (int *)&(((char *)&config)[offset]);

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

/* ARGSUSED */
static void config_user_element(const char *str,
				const char *item,
				int offset,
				void *defval)
{
    struct passwd *up;

    if (str != NULL && strlen(str) > 0)
    {
	if ((up = getpwnam(str)) != NULL)
	    seteuid(up->pw_uid);
	else
            std::clog << "ERROR: couldn't find user " << str << std::endl;
    }
}

/* ARGSUSED */
static void config_group_element(const char *str,
				 const char *item,
				 int offset,
				 void *defval)
{
    struct group *gp;

    if (str != NULL && strlen(str) > 0)
    {
	if ((gp = getgrnam(str)) != NULL)
	    setegid(gp->gr_gid);
	else
            std::clog << "ERROR: couldn't find group " << str << std::endl;
    }
}

static void config_logfac_element(const char *str,
				  const char *item,
				  int offset,
				  void *defval)
{
    int *element = (int *)&(((char *)&config)[offset]);

    if (!strcasecmp(str, "auth"))           *element = LOG_AUTH;
    else if (!strcasecmp(str, "authpriv"))  *element = LOG_AUTHPRIV;
    else if (!strcasecmp(str, "cron"))      *element = LOG_CRON;
    else if (!strcasecmp(str, "daemon"))    *element = LOG_DAEMON;
    else if (!strcasecmp(str, "kern"))      *element = LOG_KERN;
    else if (!strcasecmp(str, "local0"))    *element = LOG_LOCAL0;
    else if (!strcasecmp(str, "local1"))    *element = LOG_LOCAL1;
    else if (!strcasecmp(str, "local2"))    *element = LOG_LOCAL2;
    else if (!strcasecmp(str, "local3"))    *element = LOG_LOCAL3;
    else if (!strcasecmp(str, "local4"))    *element = LOG_LOCAL4;
    else if (!strcasecmp(str, "local5"))    *element = LOG_LOCAL5;
    else if (!strcasecmp(str, "local6"))    *element = LOG_LOCAL6;
    else if (!strcasecmp(str, "local7"))    *element = LOG_LOCAL7;
    else if (!strcasecmp(str, "lpr"))       *element = LOG_LPR;
    else if (!strcasecmp(str, "mail"))      *element = LOG_MAIL;
    else if (!strcasecmp(str, "news"))      *element = LOG_NEWS;
    else if (!strcasecmp(str, "syslog"))    *element = LOG_SYSLOG;
    else if (!strcasecmp(str, "user"))      *element = LOG_USER;
    else if (!strcasecmp(str, "uucp"))      *element = LOG_UUCP;
    else
    {
        std::clog << "Unknown facility (" << str << ") for "
                << item << ", using " << (int)defval << std::endl;
	*element = (int)defval;
    }
}

/* ARGSUSED */
static void config_socket_element(const char *str,
				  const char *item,
				  int offset,
				  void *defval)
{
    ports *element = (ports *)&(((char *)&config)[offset]);
    int new_num, *new_ports = NULL;

    new_num = element->num_ports + 1;
    if ((new_ports = realloc(element->port_nums,
			     sizeof(int) * new_num)) == NULL)
    {
        std::clog << "ERROR: couldn't allocate memory for ports: "
                  << strerror(errno) << " (" << errno << ")" << std::endl;
	return;
    }
    element->port_nums = new_ports;
    element->port_nums[element->num_ports] = atoi(str);
    element->num_ports = new_num;
}

/* ARGSUSED */
static void config_location_element(const char *str,
				    const char *item,
				    int offset,
				    void *defval)
{
    location *element = (location *)&(((char *)&config)[offset]);
    char *ptr = (char *)str;
    int i;

    for (i = 0; i < 6; ++i)
    {
	errno = 0;
	if (i < 3)
	    element->dim[i] = strtoull(ptr, &ptr, 10);
	else
	    element->steps[i - 3] = (u_int16_t)strtoul(ptr, &ptr, 10);
	if (errno != 0)
	{
            std::clog << "ERROR: parsing location element \"" << ptr << "\": "
                strerror(errno) << " (" << errno << ")" << std::endl;
	    break;
	}
    }
}
