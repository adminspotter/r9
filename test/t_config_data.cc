#include "../server/config_data.h"

#include <unistd.h>

#include <fstream>

#include <gtest/gtest.h>

/* There is a global config_data object that comes in with
 * config_data.cc, but we won't use it.  We'll make our own new ones
 * for each test, so we don't have to worry about previous state.
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
