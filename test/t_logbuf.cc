#include <unistd.h>

#include <iostream>
#include <fstream>

#include "../client/logbuf.h"
#include "../client/ui/widget.h"
#include <gtest/gtest.h>

const char *fname = "./t_logbuf_test_file.txt";
int add_count = 0;

void add_log_entry(logbuf::lb_entry *lbe)
{
    ++add_count;
}

TEST(LogbufTest, OutputFileCount)
{
    std::clog.rdbuf(new logbuf(fname));

    std::clog << "Howdy there" << std::endl;
    std::clog << "This is another entry" << std::endl;
    std::clog << "Yet another one" << std::endl;
    std::clog << "OK, bye now" << std::endl;

    ASSERT_EQ(add_count, 4);

    dynamic_cast<logbuf *>(std::clog.rdbuf())->close();

    std::ifstream fs(fname, std::ios::in);
    ASSERT_TRUE(fs.is_open());
    int count = 0, reads = 0;
    std::string str;

    while (!fs.eof())
    {
        std::getline(fs, str);
        if (str.size() > 0)
            ++count;
        ++reads;
    }
    ASSERT_EQ(count, 4);
    ASSERT_EQ(reads, 5);
    fs.close();
    ASSERT_FALSE(fs.is_open());

    unlink(fname);
}
