#include <tap++.h>

using namespace TAP;

#include "../server/classes/config_data.h"

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>

#include <fstream>

#include "../proto/ec.h"
#include "../proto/key.h"

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

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    config_data *conf;

    try
    {
        conf = new config_data;
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    is(conf->server_root, config_data::SERVER_ROOT,
       test + "expected server root");
    is(conf->log_prefix, config_data::LOG_PREFIX, test + "expected log prefix");
    is(conf->pid_fname, config_data::PID_FNAME, test + "expected pid fname");
    is(conf->db_type, config_data::DB_TYPE, test + "expected db type");
    is(conf->db_host, config_data::DB_HOST, test + "expected db host");
    is(conf->db_name, config_data::DB_NAME, test + "expected db name");
    is(conf->action_lib, config_data::ACTION_LIB, test + "expected action lib");
    is(conf->daemonize, true, test + "expected daemonize");
    is(conf->use_keepalive, false, test + "expected keepalive");
    is(conf->use_nonblock, false, test + "expected nonblock");
    is(conf->use_reuse, true, test + "expected reuse");
    is(conf->use_linger, config_data::LINGER_LEN, test + "expected linger");
    is(conf->log_facility, config_data::LOG_FACILITY,
       test + "expected log facility");
    is(conf->access_threads, config_data::NUM_THREADS,
       test + "expected access count");
    is(conf->motion_threads, config_data::NUM_THREADS,
       test + "expected motion count");
    is(conf->send_threads, config_data::NUM_THREADS,
       test + "expected send count");
    is(conf->update_threads, config_data::NUM_THREADS,
       test + "expected update count");
    is(conf->size.dim[0], config_data::ZONE_SIZE,
       test + "expected zone x size");
    is(conf->size.dim[1], config_data::ZONE_SIZE,
       test + "expected zone y size");
    is(conf->size.dim[2], config_data::ZONE_SIZE,
       test + "expected zone z size");
    is(conf->size.steps[0], config_data::ZONE_STEPS,
       test + "expected zone x steps");
    is(conf->size.steps[1], config_data::ZONE_STEPS,
       test + "expected zone y steps");
    is(conf->size.steps[2], config_data::ZONE_STEPS,
       test + "expected zone z steps");

    ok(conf->key.priv_key == NULL, test + "expected private key");
    uint8_t expected_pub_key[128];
    memset(expected_pub_key, 0, sizeof(expected_pub_key));
    is(memcmp(conf->key.pub_key, expected_pub_key, sizeof(conf->key.pub_key)),
       0,
       test + "expected public key");

    delete conf;
}

void test_setup_cleanup(void)
{
    std::string test = "setup/cleanup: ", st;
    std::string fname = "./t_config_data.fake";
    std::string key_fname = "./t_config_data.key";
    char prog[] = "r9d", f_arg[] = "-f";
    char *args[3] = { prog, f_arg, (char *)fname.c_str() };

    std::ofstream ofs(fname);
    ofs << "Port dgram:0.0.0.0:9870" << std::endl;
    ofs << "Console unix:./t_config_data.test.sock" << std::endl;
    ofs << "ZoneSize 3000 4000 5000 3 4 5" << std::endl;
    ofs << "KeyFile " << key_fname << std::endl;
    ofs.close();

    EVP_PKEY *key = generate_ecdh_key();
    pkey_to_file(key, key_fname.c_str(), NULL);

    st = "default values: ";
    is(config.size.dim[0], config_data::ZONE_SIZE,
       test + st + "expected zone x size");
    is(config.size.dim[1], config_data::ZONE_SIZE,
       test + st + "expected zone y size");
    is(config.size.dim[2], config_data::ZONE_SIZE,
       test + st + "expected zone z size");
    is(config.size.steps[0], config_data::ZONE_STEPS,
       test + st + "expected zone x steps");
    is(config.size.steps[1], config_data::ZONE_STEPS,
       test + st + "expected zone y steps");
    is(config.size.steps[2], config_data::ZONE_STEPS,
       test + st + "expected zone z steps");
    is(config.listen_ports.size(), 0, test + st + "expected port size");
    is(config.consoles.size(), 0, test + st + "expected console size");

    seteuid_count = setegid_count = chdir_count = 0;

    setup_configuration(3, args);
    unlink(fname.c_str());
    unlink(key_fname.c_str());

    st = "setup values: ";
    is(config.size.dim[0], 3000, test + st + "expected zone x size");
    is(config.size.dim[1], 4000, test + st + "expected zone y size");
    is(config.size.dim[2], 5000, test + st + "expected zone z size");
    is(config.size.steps[0], 3, test + st + "expected zone x steps");
    is(config.size.steps[1], 4, test + st + "expected zone x steps");
    is(config.size.steps[2], 5, test + st + "expected zone x steps");
    is(config.listen_ports.size(), 1, test + st + "expected port size");
    is(config.consoles.size(), 1, test + st + "expected console size");
    is(seteuid_count, 0, test + st + "expected seteuids");
    is(setegid_count, 0, test + st + "expected setegids");
    is(chdir_count, 0, test + st + "expected chdirs");
    ok(config.key.priv_key != NULL, test + st + "expected key");

    cleanup_configuration();

    st = "cleaned up values: ";
    is(config.size.dim[0], config_data::ZONE_SIZE,
       test + st + "expected zone x size");
    is(config.size.dim[1], config_data::ZONE_SIZE,
       test + st + "expected zone y size");
    is(config.size.dim[2], config_data::ZONE_SIZE,
       test + st + "expected zone z size");
    is(config.size.steps[0], config_data::ZONE_STEPS,
       test + st + "expected zone x steps");
    is(config.size.steps[1], config_data::ZONE_STEPS,
       test + st + "expected zone y steps");
    is(config.size.steps[2], config_data::ZONE_STEPS,
       test + st + "expected zone z steps");
    is(config.listen_ports.size(), 0, test + st + "expected port size");
    is(config.consoles.size(), 0, test + st + "expected console size");
    is(seteuid_count, 1, test + st + "expected seteuids");
    is(setegid_count, 1, test + st + "expected setegids");
    is(chdir_count, 1, test + st + "expected chdirs");
    ok(config.key.priv_key == NULL, test + st + "expected key");
}

void test_parse_command_line(void)
{
    std::string test = "parse command line: ";
    std::string fname = "./t_config_data.fake";
    char prog[] = "r9d", d_arg[] = "-d", f_arg[] = "-f", n_arg[] = "-n";
    char *args[5] = { prog, d_arg, f_arg, (char *)fname.c_str(), n_arg };
    config_data *conf = new config_data;

    std::ofstream ofs(fname);
    ofs << std::endl;
    ofs.close();

    conf->daemonize = true;
    try
    {
        conf->parse_command_line(5, args);
    }
    catch (...)
    {
        fail(test + "parse exception");
        return;
    }
    is(conf->argv.size(), 5, test + "expected args size");
    is(conf->daemonize, false, test + "expected daemonize");

    unlink(fname.c_str());
    delete conf;
}

/* This test will operate on the global, so we won't make our own.
 * All the individual parsing functions are static within the .cc
 * file, so we can't call them directly.  We have access to the
 * read_config_file method, so we can tailor the lines in our file to
 * test each of the specific functions.
 */
void test_parse_config_file(void)
{
    std::string test = "parse config file: ", st;
    std::string fname = "./t_config_data.fake";
    std::ofstream ofs(fname);
    ofs << std::endl;
    ofs << "# This is a comment" << std::endl;
    ofs << "   Leading    spaces" << std::endl;
    ofs << "Trailing  spaces        " << std::endl;
    ofs << "PidFile some_file  # string" << std::endl;
    ofs << "AccessThreads 987  # integer" << std::endl;
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

    st = "default values: ";
    is(config.pid_fname, config_data::PID_FNAME,
       test + st + "expected pid fname");
    is(config.access_threads, config_data::NUM_THREADS,
       test + st + "expected access count");
    is(config.use_keepalive, false, test + st + "expected keepalive");
    is(config.log_facility, config_data::LOG_FACILITY,
       test + st + "expected log facility");
    is(config.listen_ports.size(), 0, test + st + "expcted port size");
    is(config.size.dim[0], config_data::ZONE_SIZE,
       test + st + "expected zone x size");
    is(config.size.dim[1], config_data::ZONE_SIZE,
       test + st + "expected zone y size");
    is(config.size.dim[2], config_data::ZONE_SIZE,
       test + st + "expected zone z size");
    is(config.size.steps[0], config_data::ZONE_STEPS,
       test + st + "expected zone x steps");
    is(config.size.steps[1], config_data::ZONE_STEPS,
       test + st + "expected zone y steps");
    is(config.size.steps[2], config_data::ZONE_STEPS,
       test + st + "expected zone z steps");

    getpwnam_count = seteuid_count = 0;
    getgrnam_count = setegid_count = 0;

    config.read_config_file(fname);
    unlink(fname.c_str());

    st = "read values: ";
    is(config.pid_fname, "some_file", test + st + "expected pid fname");
    is(config.access_threads, 987, test + st + "expected access count");
    is(config.use_keepalive, true, test + st + "expected keepalive");
    is(getpwnam_count, 2, test + st + "expected getpwnams");
    is(seteuid_count, 1, test + st + "expected seteuids");
    is(getgrnam_count, 2, test + st + "expected getgrnams");
    is(setegid_count, 1, test + st + "expected setegids");
    is(config.log_facility, LOG_UUCP, test + st + "expected log facility");
    is(config.listen_ports.size(), 3, test + st + "expected port size");
    is(config.size.dim[0], 10, test + st + "expected zone x size");
    is(config.size.dim[1], 15, test + st + "expected zone y size");
    is(config.size.dim[2], 20, test + st + "expected zone z size");
    is(config.size.steps[0], 25, test + st + "expected zone x steps");
    is(config.size.steps[1], 30, test + st + "expected zone y steps");
    is(config.size.steps[2], 35, test + st + "expected zone z steps");
}

int main(int argc, char **argv)
{
    plan(85);

    test_create_delete();
    test_setup_cleanup();
    test_parse_command_line();
    test_parse_config_file();
    return exit_status();
}
