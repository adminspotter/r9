#include "../server/config_data.h"

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <fstream>

#include <gtest/gtest.h>

struct passwd pw;
struct group gr;
int getpwnam_count, getgrnam_count, seteuid_count, setegid_count;

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
    ofs << "ServerUID wrong    # bad user" << std::endl;
    ofs << "ServerUID correct  # good user" << std::endl;
    ofs << "ServerGID wrong    # bad group" << std::endl;
    ofs << "ServerGID correct  # good group" << std::endl;
    ofs.close();

    ASSERT_EQ(config.pid_fname, config_data::PID_FNAME);
    ASSERT_EQ(config.access_threads, config_data::NUM_THREADS);
    ASSERT_EQ(config.load_threshold, config_data::LOAD_THRESH);

    getpwnam_count = seteuid_count = 0;
    getgrnam_count = setegid_count = 0;

    config.read_config_file(fname);
    unlink(fname.c_str());

    ASSERT_EQ(config.pid_fname, "some_file");
    ASSERT_EQ(config.access_threads, 987);
    ASSERT_EQ(config.load_threshold, 9.0);
    ASSERT_EQ(getpwnam_count, 2);
    ASSERT_EQ(seteuid_count, 1);
    ASSERT_EQ(getgrnam_count, 2);
    ASSERT_EQ(setegid_count, 1);
}
