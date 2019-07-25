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
#include <sstream>
#include <stdexcept>

#include "../proto/ec.h"
#include "../proto/key.h"

#define TMP_ROOT "/tmp"

std::string tmpdir;
bool getenv_error = false;
int mkdir_fail_index = 5, mkdir_count;

char *getenv(const char *a)
{
    if (!strcmp(a, "HOME"))
    {
        if (getenv_error == true)
            return NULL;
        return (char *)tmpdir.c_str();
    }
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
    ofs << "FontPaths /bin:~:/tmp:/does_not_exist" << std::endl;
    ofs << "KeyFile " << fname << ".key" << std::endl;
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
    ok(conf->priv_key == NULL, "expected key");

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

void test_parse_command_line(void)
{
    std::string test = "parse command line: ";
    int ret;

    setup_fixture();
    std::string conf_dir = tmpdir + "/haha";
    std::string conf_file = tmpdir + "/fake.conf";
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    const char *args[7] = { "r9client", "-c", (char *)conf_dir.c_str(),
                            "-f", (char *)conf_file.c_str(), "-q", NULL};
    struct stat st;

    ConfigData *conf;

    try
    {
        conf = new ConfigData;
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    mkdir_count = 0;
    mkdir_fail_index = 5;
    conf->parse_command_line(6, args);

    is(conf->argv.size(), 6, test + "expected arg count");

    std::string expected = tmpdir + "/haha";
    is(conf->config_dir, expected, test + "expected config dir");
    is(mkdir_count, 4, test + "expected mkdir count");

    expected = tmpdir + "/fake.conf";
    is(conf->config_fname, expected, test + "expected config fname");

    is(new_clog.str(), "WARNING: Unknown option -q", test + "expected warning");
    std::clog.rdbuf(old_clog_rdbuf);

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

void test_read_config_file(void)
{
    std::string test = "read config file: ";
    int ret;
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

    conf->config_dir = tmpdir;
    conf->config_fname = tmpdir + "/config";
    create_temp_config_file(conf->config_fname);
    std::string key_fname = conf->config_fname + ".key";

    conf->read_config_file();

    std::string expected = "haha";
    is(conf->server_addr, expected, test + "expected server addr");
    is(conf->server_port, 12345, test + "expected server port");
    expected = "someuser";
    is(conf->username, expected, test + "expected username");
    expected = "worstcharnameintheworld";
    is(conf->charname, expected, test + "expected charname");
    is(conf->font_paths.size(), 3, test + "expected font path size");
    expected = "/bin";
    is(conf->font_paths[0], expected, test + "expected font path 1");
    expected = tmpdir;
    is(conf->font_paths[1], expected, test + "expected font path 2");
    expected = "/tmp";
    is(conf->font_paths[2], expected, test + "expected font path 3");
    is(conf->key_fname, key_fname, test + "expected key filename");

    test = "read crypto key: ";
    unsigned char passphrase[4] = { 'a', 'b', 'c', 0 };
    EVP_PKEY *key = generate_ecdh_key();
    pkey_to_file(key, key_fname.c_str(), passphrase);

    bool result = conf->read_crypto_key("abc");

    is(result, true, test + "expected key result");
    ok(conf->priv_key != NULL, test + "expected private key");

    unlink(key_fname.c_str());

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

void test_bad_getenv_home(void)
{
    std::string test = "getenv failure: ";
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

    conf->config_fname = tmpdir + "/config";
    std::ofstream conf_file(conf->config_fname,
                            std::fstream::out | std::fstream::trunc);
    conf_file << "FontPaths ~:/whatever" << std::endl;
    conf_file.close();

    getenv_error = true;

    try
    {
        conf->read_config_file();
        fail(test + "no exception");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("Could not find home directory"), std::string::npos,
             test + "expected exception");
    }

    getenv_error = false;

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

/* Previous test proved that read_config_file works, so we'll use
 * write_config_file to write one out, and then read_config_file into
 * a new object to make sure the file was written as expected.
 */
void test_write_config_file(void)
{
    std::string test = "write config file: ", st;
    int ret;
    ConfigData *conf;

    setup_fixture();
    st = "initial object: ";
    try
    {
        conf = new ConfigData;
    }
    catch (...)
    {
        fail(test + st + "constructor exception");
    }

    conf->server_addr = "whoa";
    conf->server_port = 9876;
    conf->username = "howdy";
    conf->charname = "anotherreallybadcharname";
    conf->font_paths.clear();
    conf->font_paths.push_back("/bin");
    conf->font_paths.push_back("~");
    conf->key_fname = "somefileorother";

    conf->config_dir = tmpdir;
    conf->config_fname = tmpdir + "/the_big_test.conf";

    conf->write_config_file();

    try
    {
        delete conf;
    }
    catch (...)
    {
        fail(test + st + "destructor exception");
    }

    /* Now for the new object */
    st = "reloaded object: ";
    try
    {
        conf = new ConfigData;
    }
    catch (...)
    {
        fail(test + st + "constructor exception");
    }

    conf->config_dir = tmpdir;
    conf->config_fname = tmpdir + "/the_big_test.conf";

    conf->read_config_file();

    std::string expected = "whoa";
    is(conf->server_addr, expected, test + st + "expected server addr");
    is(conf->server_port, 9876, test + st + "expected server port");
    expected = "howdy";
    is(conf->username, expected, test + st + "expected username");
    expected = "anotherreallybadcharname";
    is(conf->charname, expected, test + st + "expected charname");
    is(conf->font_paths.size(), 2, test + st + "expected font path size");
    expected = "/bin";
    is(conf->font_paths[0], expected, test + st + "expected font path 1");
    expected = tmpdir;
    is(conf->font_paths[1], expected, test + st + "expected font path 2");
    expected = "somefileorother";
    is(conf->key_fname, expected, test + st + "expected key filename");

    try
    {
        delete conf;
    }
    catch (...)
    {
        fail(test + st + "destructor exception");
    }
    cleanup_fixture();
}

int main(int argc, char **argv)
{
    plan(48);

#if OPENSSL_API_COMPAT < 0x10100000
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();
#endif /* OPENSSL_API_COMPAT */
    test_create_delete();
    test_make_config_dirs();
    test_parse_command_line();
    test_read_config_file();
    test_bad_getenv_home();
    test_write_config_file();
    return exit_status();
}
