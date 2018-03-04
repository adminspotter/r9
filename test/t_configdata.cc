#include <tap++.h>

using namespace TAP;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../client/configdata.h"

#include <string>
#include <fstream>
#include <stdexcept>

#include <gtest/gtest.h>

#define TMP_ROOT "/tmp"

std::string tmpdir;
int mkdir_fail_index = 5, mkdir_count;

char *getenv(const char *a)
{
    /* Cast away const, haha! */
    if (!strcmp(a, "HOME"))
        return (char *)tmpdir.c_str();
    return NULL;
}

int mkdir(const char *a, mode_t b)
{
    ++mkdir_count;
    if (mkdir_count == mkdir_fail_index)
    {
        errno = EACCES;
        return -1;
    }
    return 0;
}

int clean_dir(const char *dir)
{
    DIR *d;
    struct dirent *de;
    struct stat st;
    std::string fullpath;

    if ((d = opendir(dir)) == NULL)
        return -1;

    while ((de = readdir(d)) != NULL)
    {
        /* Skip the . and .. */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        fullpath = dir;
        fullpath += '/';
        fullpath += de->d_name;
        if (stat(fullpath.c_str(), &st))
            return -1;

        if (S_ISREG(st.st_mode))
        {
            if (unlink(fullpath.c_str()))
                return -1;
        }
        else if (S_ISDIR(st.st_mode))
        {
            if (clean_dir(fullpath.c_str()))
                return -1;
        }
        else
            /* Shouldn't be non-dir, non-file stuff in here. */
            return -1;
    }
    if (closedir(d))
        return -1;
    return rmdir(dir);
}

void create_temp_config_file(std::string& fname)
{
    std::ofstream ofs(fname.c_str(), std::fstream::out | std::fstream::trunc);

    ofs << "# comment line" << std::endl;
    ofs << "	         ServerAddr haha    # comment at the end" << std::endl;
    ofs << "ServerPort 12345   	       " << std::endl;
    ofs << "Username  		someuser" << std::endl;
    ofs << "   Charname    worstcharnameintheworld   " << std::endl;
    ofs << "FontPaths /a/b/c:~/d/e/f:/g/h/i" << std::endl;
    ofs.close();
}

/* The fixture, instead of creating an object and all that, will just
 * create and remove a temporary directory tree that we can use for
 * our "home directory".
 */
void setup_fixture(void)
{
    char temp_name[1024], *ret;

    strcpy(temp_name, TMP_ROOT "/configdata_testXXXXXX");
    /* Create temp dir tree */
    ret = mkdtemp(temp_name);
    not_ok(ret == NULL, "fixture: created temp dir");

    tmpdir = temp_name;
}

void cleanup_fixture(void)
{
    /* Remove temp dir tree */
    int ret = clean_dir(tmpdir.c_str());
    is(ret, 0, "fixture: removed temp dir tree");
    tmpdir = "";
}

class ConfigdataTest : public ::testing::Test
{
  protected:
    void SetUp()
        {
            setup_fixture();
        };

    void TearDown()
        {
            cleanup_fixture();
        };
};

class fake_ConfigData : public ConfigData
{
  public:
    fake_ConfigData() : ConfigData() {};
    virtual ~fake_ConfigData() {};

    using ConfigData::make_config_dirs;
};

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    ConfigData *conf;

    setup_fixture();
    try
    {
        conf = new ConfigData;
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    /* Test what defaults have been set. */
    std::string expected = tmpdir + "/.r9";
    is(conf->config_dir, expected, test + "expected config dir");
    expected += "/config";
    is(conf->config_fname, expected, test + "expected config fname");
    is(conf->server_addr, ConfigData::SERVER_ADDR, test + "expected addr");
    is(conf->server_port, ConfigData::SERVER_PORT, test + "expected port");
    ok(conf->font_paths.size() > 0, "contents in font path");

    try
    {
        delete conf;
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
    cleanup_fixture();
}

void test_make_config_dirs(void)
{
    std::string test = "make config dirs: ", st;
    int ret;
    char *args[] = {};

    fake_ConfigData *conf;

    setup_fixture();
    try
    {
        conf = new fake_ConfigData;
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    st = "success: ";
    mkdir_count = 0;
    mkdir_fail_index = 5;
    conf->make_config_dirs();

    is(mkdir_count, 4, test + st + "no failures");

    st = "failures: ";
    for (mkdir_fail_index = 1; mkdir_fail_index <= 4; ++mkdir_fail_index)
    {
        mkdir_count = 0;
        try
        {
            conf->make_config_dirs();
        }
        catch (std::runtime_error& e)
        {
            std::string err(e.what());
            isnt(err.find("directory"), std::string::npos,
                 test + st + "failed with correct error");
        }
        catch (...)
        {
            fail(test + "wrong exception type");
        }
    }
    mkdir_count = 0;
    mkdir_fail_index = 5;

    try
    {
        delete conf;
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
    cleanup_fixture();
}

TEST_F(ConfigdataTest, ParseCommandLine)
{
    int ret;
    std::string conf_dir = tmpdir + "/haha";
    std::string conf_file = tmpdir + "/fake.conf";
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    char *args[7] = { "r9client", "-c", (char *)conf_dir.c_str(),
                      "-f", (char *)conf_file.c_str(), "-q", NULL};
    struct stat st;

    ConfigData *conf;

    ASSERT_NO_THROW(
        {
            conf = new ConfigData;
        });

    mkdir_count = 0;
    mkdir_fail_index = 5;
    conf->parse_command_line(6, args);

    ASSERT_EQ(conf->argv.size(), 6);

    std::string expected = tmpdir + "/haha";
    ASSERT_EQ(conf->config_dir, expected);
    ASSERT_EQ(mkdir_count, 4);

    expected = tmpdir + "/fake.conf";
    ASSERT_EQ(conf->config_fname, expected);

    ASSERT_EQ(new_clog.str(), "WARNING: Unknown option -q");
    std::clog.rdbuf(old_clog_rdbuf);

    ASSERT_NO_THROW(
        {
            delete conf;
        });
}

TEST_F(ConfigdataTest, ReadConfigFile)
{
    int ret;
    ConfigData *conf;

    ASSERT_NO_THROW(
        {
            conf = new ConfigData;
        });

    conf->config_dir = tmpdir;
    conf->config_fname = tmpdir + "/config";
    create_temp_config_file(conf->config_fname);

    conf->read_config_file();

    std::string expected = "haha";
    ASSERT_EQ(conf->server_addr, expected);
    ASSERT_EQ(conf->server_port, 12345);
    expected = "someuser";
    ASSERT_EQ(conf->username, expected);
    expected = "worstcharnameintheworld";
    ASSERT_EQ(conf->charname, expected);
    ASSERT_TRUE(conf->font_paths.size() == 3);
    expected = "/a/b/c";
    ASSERT_EQ(conf->font_paths[0], expected);
    expected = "~/d/e/f";
    ASSERT_EQ(conf->font_paths[1], expected);
    expected = "/g/h/i";
    ASSERT_EQ(conf->font_paths[2], expected);

    ASSERT_NO_THROW(
        {
            delete conf;
        });
}

/* Previous test proved that read_config_file works, so we'll use
 * write_config_file to write one out, and then read_config_file into
 * a new object to make sure the file was written as expected.
 */
TEST_F(ConfigdataTest, WriteConfigFile)
{
    int ret;
    ConfigData *conf;

    ASSERT_NO_THROW(
        {
            conf = new ConfigData;
        });

    conf->server_addr = "whoa";
    conf->server_port = 9876;
    conf->username = "howdy";
    conf->charname = "anotherreallybadcharname";
    conf->font_paths.clear();
    conf->font_paths.push_back("/a/b/c");
    conf->font_paths.push_back("~/d/e/f");

    conf->config_dir = tmpdir;
    conf->config_fname = tmpdir + "/the_big_test.conf";

    conf->write_config_file();

    ASSERT_NO_THROW(
        {
            delete conf;
        });

    /* Now for the new object */
    ASSERT_NO_THROW(
        {
            conf = new ConfigData;
        });

    conf->config_dir = tmpdir;
    conf->config_fname = tmpdir + "/the_big_test.conf";

    conf->read_config_file();

    std::string expected = "whoa";
    ASSERT_EQ(conf->server_addr, expected);
    ASSERT_EQ(conf->server_port, 9876);
    expected = "howdy";
    ASSERT_EQ(conf->username, expected);
    expected = "anotherreallybadcharname";
    ASSERT_EQ(conf->charname, expected);
    ASSERT_TRUE(conf->font_paths.size() == 2);
    expected = "/a/b/c";
    ASSERT_EQ(conf->font_paths[0], expected);
    expected = "~/d/e/f";
    ASSERT_EQ(conf->font_paths[1], expected);

    ASSERT_NO_THROW(
        {
            delete conf;
        });
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    plan(14);

    int gtests = RUN_ALL_TESTS();

    test_create_delete();
    test_make_config_dirs();
    return gtests & exit_status();
}
