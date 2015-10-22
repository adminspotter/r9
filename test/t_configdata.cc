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

#include <gtest/gtest.h>

#define TMP_ROOT "/tmp"

std::string tmpdir;

char *getenv(const char *a)
{
    /* Cast away const, haha! */
    if (!strcmp(a, "HOME"))
        return (char *)tmpdir.c_str();
    return NULL;
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
    ofs << "   Password    worstpasswordintheworld   " << std::endl;
    ofs.close();
}

/* The fixture, instead of creating an object and all that, will just
 * create and remove a temporary directory tree that we can use for
 * our "home directory".
 */
class ConfigdataTest : public ::testing::Test
{
  protected:
    void SetUp()
        {
            char temp_name[1024], *ret;

            strcpy(temp_name, TMP_ROOT "/configdata_testXXXXXX");
            /* Create temp dir tree */
            ret = mkdtemp(temp_name);
            ASSERT_TRUE(ret != NULL);

            tmpdir = temp_name;
        };

    void TearDown()
        {
            /* Remove temp dir tree */
            int ret = clean_dir(tmpdir.c_str());
            ASSERT_EQ(ret, 0);
            tmpdir = "";
        };
};

TEST_F(ConfigdataTest, BasicCreateDelete)
{
    ConfigData *conf;

    ASSERT_NO_THROW(
        {
            conf = new ConfigData;
        });

    /* Test what defaults have been set. */
    std::string expected = tmpdir + "/.r9";
    ASSERT_EQ(conf->config_dir, expected);
    expected += "/config";
    ASSERT_EQ(conf->config_fname, expected);
    ASSERT_EQ(conf->server_addr, ConfigData::SERVER_ADDR);
    ASSERT_EQ(conf->server_port, ConfigData::SERVER_PORT);

    ASSERT_NO_THROW(
        {
            delete conf;
        });
}

TEST_F(ConfigdataTest, MakeConfigDirs)
{
    int ret;
    char *args[] = {};
    struct stat st;

    ConfigData *conf;

    ASSERT_NO_THROW(
        {
            conf = new ConfigData;
        });

    /* If there's nothing to parse, that won't get done, and if there
     * is no config file, nothing will happen there either.
     */
    conf->parse_command_line(0, args);

    /* Verify that our expected directories are created */
    std::string expected_base = tmpdir + "/.r9";
    ret = stat(expected_base.c_str(), &st);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(S_ISDIR(st.st_mode), 1);

    std::string expected = expected_base + "/texture";
    ret = stat(expected.c_str(), &st);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(S_ISDIR(st.st_mode), 1);

    expected = expected_base + "/geometry";
    ret = stat(expected.c_str(), &st);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(S_ISDIR(st.st_mode), 1);

    expected = expected_base + "/sound";
    ret = stat(expected.c_str(), &st);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(S_ISDIR(st.st_mode), 1);

    ASSERT_NO_THROW(
        {
            delete conf;
        });
}

TEST_F(ConfigdataTest, ParseCommandLine)
{
    int ret;
    std::string conf_dir = tmpdir + "/haha";
    char *args[4] = { "r9client", "-c", (char *)conf_dir.c_str(), NULL};
    struct stat st;

    ConfigData *conf;

    ASSERT_NO_THROW(
        {
            conf = new ConfigData;
        });

    conf->parse_command_line(3, args);

    ASSERT_EQ(conf->argv.size(), 3);

    std::string expected = tmpdir + "/haha";
    ASSERT_EQ(conf->config_dir, expected);
    ret = stat(expected.c_str(), &st);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(S_ISDIR(st.st_mode), 1);

    expected += "/config";
    ASSERT_EQ(conf->config_fname, expected);

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

    ret = mkdir(conf->config_dir.c_str(), 0700);
    ASSERT_EQ(ret, 0);
    create_temp_config_file(conf->config_fname);

    conf->read_config_file();

    std::string expected = "haha";
    ASSERT_EQ(conf->server_addr, expected);
    ASSERT_EQ(conf->server_port, 12345);
    expected = "someuser";
    ASSERT_EQ(conf->username, expected);
    expected = "worstpasswordintheworld";
    ASSERT_EQ(conf->password, expected);

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
    conf->password = "anotherreallybadpassword";

    ret = mkdir(conf->config_dir.c_str(), 0700);
    ASSERT_EQ(ret, 0);

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

    conf->read_config_file();

    std::string expected = "whoa";
    ASSERT_EQ(conf->server_addr, expected);
    ASSERT_EQ(conf->server_port, 9876);
    expected = "howdy";
    ASSERT_EQ(conf->username, expected);
    expected = "anotherreallybadpassword";
    ASSERT_EQ(conf->password, expected);

    ASSERT_NO_THROW(
        {
            delete conf;
        });
}
