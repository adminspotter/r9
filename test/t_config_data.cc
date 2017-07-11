#include "../server/config_data.h"

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>

#include <fstream>

#include <gtest/gtest.h>

extern void setup_configuration(int, char **);
extern void cleanup_configuration(void);

struct passwd pw;
struct group gr;
int getpwnam_count, getgrnam_count, seteuid_count, setegid_count, chdir_count;

struct passwd *getpwnam(const char *a)
{
    ++getpwnam_count;
    if (!strncmp(a, "correct", 7))
    {
        pw.pw_uid = 987;
        return &pw;
    }
    return NULL;
}

struct group *getgrnam(const char *a)
{
    ++getgrnam_count;
    if (!strncmp(a, "correct", 7))
    {
        gr.gr_gid = 987;
        return &gr;
    }
    return NULL;
}

int seteuid(uid_t a)
{
    ++seteuid_count;
    return 0;
}

int setegid(gid_t a)
{
    ++setegid_count;
    return 0;
}

int chdir(const char *a)
{
    ++chdir_count;
    return 0;
}

/* There is a global config_data object that comes in with
 * config_data.cc, but we'll use it as little as we can.  We'll make
 * our own new ones for each test, so we don't have to worry about
 * previous state.
 *
 * The setup_configuration and cleanup_configuration functions do work
 * on the global object, as does the parse_config_line method, so
 * we'll worry about it for those two tests.
 */

TEST(ConfigDataTest, CreateDelete)
{
    config_data *conf;

    ASSERT_NO_THROW(
        {
            conf = new config_data;
        });

    ASSERT_EQ(conf->server_root, config_data::SERVER_ROOT);
    ASSERT_EQ(conf->log_prefix, config_data::LOG_PREFIX);
    ASSERT_EQ(conf->pid_fname, config_data::PID_FNAME);
    ASSERT_EQ(conf->db_type, config_data::DB_TYPE);
    ASSERT_EQ(conf->db_host, config_data::DB_HOST);
    ASSERT_EQ(conf->db_name, config_data::DB_NAME);
    ASSERT_EQ(conf->action_lib, config_data::ACTION_LIB);
    ASSERT_EQ(conf->daemonize, true);
    ASSERT_EQ(conf->use_keepalive, false);
    ASSERT_EQ(conf->use_nonblock, false);
    ASSERT_EQ(conf->use_reuse, true);
    ASSERT_EQ(conf->use_linger, config_data::LINGER_LEN);
    ASSERT_EQ(conf->log_facility, config_data::LOG_FACILITY);
    ASSERT_EQ(conf->load_threshold, config_data::LOAD_THRESH);
    ASSERT_EQ(conf->min_subservers, config_data::MIN_SUBSERV);
    ASSERT_EQ(conf->max_subservers, config_data::MAX_SUBSERV);
    ASSERT_EQ(conf->access_threads, config_data::NUM_THREADS);
    ASSERT_EQ(conf->motion_threads, config_data::NUM_THREADS);
    ASSERT_EQ(conf->send_threads, config_data::NUM_THREADS);
    ASSERT_EQ(conf->update_threads, config_data::NUM_THREADS);
    ASSERT_EQ(conf->size.dim[0], config_data::ZONE_SIZE);
    ASSERT_EQ(conf->size.dim[1], config_data::ZONE_SIZE);
    ASSERT_EQ(conf->size.dim[2], config_data::ZONE_SIZE);
    ASSERT_EQ(conf->size.steps[0], config_data::ZONE_STEPS);
    ASSERT_EQ(conf->size.steps[1], config_data::ZONE_STEPS);
    ASSERT_EQ(conf->size.steps[2], config_data::ZONE_STEPS);

    delete conf;
}

TEST(ConfigDataTest, SetupCleanup)
{
    std::string fname = "./t_config_data.fake";
    char prog[] = "r9d", f_arg[] = "-f";
    char *args[3] = { prog, f_arg, (char *)fname.c_str() };

    std::ofstream ofs(fname);
    ofs << "Port dgram:0.0.0.0:9870" << std::endl;
    ofs << "Console unix:./t_config_data.test.sock" << std::endl;
    ofs << "ZoneSize 3000 4000 5000 3 4 5" << std::endl;
    ofs.close();

    ASSERT_EQ(config.size.dim[0], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.dim[1], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.dim[2], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.steps[0], config_data::ZONE_STEPS);
    ASSERT_EQ(config.size.steps[1], config_data::ZONE_STEPS);
    ASSERT_EQ(config.size.steps[2], config_data::ZONE_STEPS);
    ASSERT_TRUE(config.listen_ports.size() == 0);
    ASSERT_TRUE(config.consoles.size() == 0);

    seteuid_count = setegid_count = chdir_count = 0;

    setup_configuration(3, args);
    unlink(fname.c_str());

    ASSERT_EQ(config.size.dim[0], 3000);
    ASSERT_EQ(config.size.dim[1], 4000);
    ASSERT_EQ(config.size.dim[2], 5000);
    ASSERT_EQ(config.size.steps[0], 3);
    ASSERT_EQ(config.size.steps[1], 4);
    ASSERT_EQ(config.size.steps[2], 5);
    ASSERT_TRUE(config.listen_ports.size() == 1);
    ASSERT_TRUE(config.consoles.size() == 1);
    ASSERT_EQ(seteuid_count, 0);
    ASSERT_EQ(setegid_count, 0);
    ASSERT_EQ(chdir_count, 0);

    cleanup_configuration();

    ASSERT_EQ(config.size.dim[0], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.dim[1], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.dim[2], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.steps[0], config_data::ZONE_STEPS);
    ASSERT_EQ(config.size.steps[1], config_data::ZONE_STEPS);
    ASSERT_EQ(config.size.steps[2], config_data::ZONE_STEPS);
    ASSERT_TRUE(config.listen_ports.size() == 0);
    ASSERT_TRUE(config.consoles.size() == 0);
    ASSERT_EQ(seteuid_count, 1);
    ASSERT_EQ(setegid_count, 1);
    ASSERT_EQ(chdir_count, 1);
}

TEST(ConfigDataTest, ParseCommandLine)
{
    std::string fname = "./t_config_data.fake";
    char prog[] = "r9d", d_arg[] = "-d", f_arg[] = "-f", n_arg[] = "-n";
    char *args[5] = { prog, d_arg, f_arg, (char *)fname.c_str(), n_arg };
    config_data *conf = new config_data;

    std::ofstream ofs(fname);
    ofs << std::endl;
    ofs.close();

    conf->daemonize = true;
    ASSERT_NO_THROW(
        {
            conf->parse_command_line(5, args);
        });
    ASSERT_TRUE(conf->argv.size() == 5);
    ASSERT_EQ(conf->daemonize, false);

    unlink(fname.c_str());
    delete conf;
}

/* This test will operate on the global, so we won't make our own.
 * All the individual parsing functions are static within the .cc
 * file, so we can't call them directly.  We have access to the
 * read_config_file method, so we can tailor the lines in our file to
 * test each of the specific functions.
 */
TEST(ConfigDataTest, ParseConfigLine)
{
    std::string fname = "./t_config_data.fake";
    std::ofstream ofs(fname);
    ofs << std::endl;
    ofs << "# This is a comment" << std::endl;
    ofs << "   Leading    spaces" << std::endl;
    ofs << "Trailing  spaces        " << std::endl;
    ofs << "PidFile some_file  # string" << std::endl;
    ofs << "AccessThreads 987  # integer" << std::endl;
    ofs << "UseBalance 9.0     # float" << std::endl;
    ofs << "UseKeepAlive no    # negative bool" << std::endl;
    ofs << "UseKeepAlive yes   # yes bool" << std::endl;
    ofs << "UseKeepAlive true  # true bool" << std::endl;
    ofs << "UseKeepAlive on    # on bool" << std::endl;
    ofs << "ServerUID wrong    # bad user" << std::endl;
    ofs << "ServerUID correct  # good user" << std::endl;
    ofs << "ServerGID wrong    # bad group" << std::endl;
    ofs << "ServerGID correct  # good group" << std::endl;
    ofs << "LogFacility auth" << std::endl;
    ofs << "LogFacility authpriv" << std::endl;
    ofs << "LogFacility cron" << std::endl;
    ofs << "LogFacility daemon" << std::endl;
    ofs << "LogFacility kern" << std::endl;
    ofs << "LogFacility local0" << std::endl;
    ofs << "LogFacility local1" << std::endl;
    ofs << "LogFacility local2" << std::endl;
    ofs << "LogFacility local3" << std::endl;
    ofs << "LogFacility local4" << std::endl;
    ofs << "LogFacility local5" << std::endl;
    ofs << "LogFacility local6" << std::endl;
    ofs << "LogFacility local7" << std::endl;
    ofs << "LogFacility lpr" << std::endl;
    ofs << "LogFacility mail" << std::endl;
    ofs << "LogFacility news" << std::endl;
    ofs << "LogFacility syslog" << std::endl;
    ofs << "LogFacility user" << std::endl;
    ofs << "LogFacility uucp" << std::endl;
    ofs << "LogFacility bogus" << std::endl;
    ofs << "Port broken" << std::endl;
    ofs << "Port bogus:whatever" << std::endl;
    ofs << "Port unix:thing" << std::endl;
    ofs << "Port dgram:1.2.3.4" << std::endl;
    ofs << "Port dgram:1.2.3.4:9876" << std::endl;
    ofs << "Port stream:9876" << std::endl;
    ofs << "Port stream:f00f::abcd:9876" << std::endl;
    ofs << "Port stream:[f00f::abcd:9876" << std::endl;
    ofs << "Port stream:[f00f::abcd]:9876" << std::endl;
    ofs << "ZoneSize 10 15 20 25 30 35" << std::endl;
    ofs.close();

    ASSERT_EQ(config.pid_fname, config_data::PID_FNAME);
    ASSERT_EQ(config.access_threads, config_data::NUM_THREADS);
    ASSERT_EQ(config.load_threshold, config_data::LOAD_THRESH);
    ASSERT_EQ(config.use_keepalive, false);
    ASSERT_EQ(config.log_facility, config_data::LOG_FACILITY);
    ASSERT_TRUE(config.listen_ports.size() == 0);
    ASSERT_EQ(config.size.dim[0], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.dim[1], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.dim[2], config_data::ZONE_SIZE);
    ASSERT_EQ(config.size.steps[0], config_data::ZONE_STEPS);
    ASSERT_EQ(config.size.steps[1], config_data::ZONE_STEPS);
    ASSERT_EQ(config.size.steps[2], config_data::ZONE_STEPS);

    getpwnam_count = seteuid_count = 0;
    getgrnam_count = setegid_count = 0;

    config.read_config_file(fname);
    unlink(fname.c_str());

    ASSERT_EQ(config.pid_fname, "some_file");
    ASSERT_EQ(config.access_threads, 987);
    ASSERT_EQ(config.load_threshold, 9.0);
    ASSERT_EQ(config.use_keepalive, true);
    ASSERT_EQ(getpwnam_count, 2);
    ASSERT_EQ(seteuid_count, 1);
    ASSERT_EQ(getgrnam_count, 2);
    ASSERT_EQ(setegid_count, 1);
    ASSERT_EQ(config.log_facility, LOG_UUCP);
    ASSERT_TRUE(config.listen_ports.size() == 3);
    ASSERT_EQ(config.size.dim[0], 10);
    ASSERT_EQ(config.size.dim[1], 15);
    ASSERT_EQ(config.size.dim[2], 20);
    ASSERT_EQ(config.size.steps[0], 25);
    ASSERT_EQ(config.size.steps[1], 30);
    ASSERT_EQ(config.size.steps[2], 35);
}
