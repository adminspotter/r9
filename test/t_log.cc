#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>

#include "../server/classes/log.h"
#include <gtest/gtest.h>

bool opened = false, closed = false;
int options = 0, facility = 0, priority = 0;
char *identity = NULL, *format = NULL, *str = NULL;

void openlog(const char *ident, int logopt, int fac)
{
    opened = true;
    identity = strdup(ident);
    options = logopt;
    facility = fac;
}

void syslog(int prio, const char *form, ...)
{
    va_list va;

    priority = prio;
    format = strdup(form);
    va_start(va, form);
    str = strdup(va_arg(va, char *));
    va_end(va);
}

void closelog(void)
{
    closed = true;
}

TEST(LogTest, Opened)
{
    ASSERT_EQ(opened, true);
}

TEST(LogTest, Identity)
{
    ASSERT_STREQ(identity, "testing");
    free(identity);
}

TEST(LogTest, Options)
{
    ASSERT_EQ(options, LOG_PID);
}

TEST(LogTest, Facility)
{
    ASSERT_EQ(facility, 1);
}

TEST(LogTest, Priority)
{
    ASSERT_EQ(priority, syslogErr);
}

TEST(LogTest, Format)
{
    ASSERT_STREQ(format, "%s");
    free(format);
}

TEST(LogTest, LogMsg)
{
    ASSERT_STREQ(str, "success\n");
    free(str);
}

TEST(LogTest, Closed)
{
    ASSERT_EQ(closed, true);
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    std::clog.rdbuf(new Log("testing", 1));
    std::clog << syslogErr << "success" << std::endl;
    dynamic_cast<Log *>(std::clog.rdbuf())->close();

    return RUN_ALL_TESTS();
}
