#include <tap++.h>

using namespace TAP;

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>

#include "../server/classes/log.h"

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

void test_opened(void)
{
    is(opened, true, "opened: expected value");
}

void test_identity(void)
{
    is(strncmp(identity, "testing", 8), 0, "identity: expected value");
    free(identity);
}

void test_options(void)
{
    is(options, LOG_PID, "options: expected value");
}

void test_facility(void)
{
    is(facility, 1, "facility: expected value");
}

void test_priority(void)
{
    is(priority, syslogErr, "priority: expected value");
}

void test_format(void)
{
    is(strncmp(format, "%s", 3), 0, "format: expected value");
    free(format);
}

void test_logmsg(void)
{
    is(strncmp(str, "success\n", 8), 0, "logmsg: expected value");
    free(str);
}

void test_closed(void)
{
    is(closed, true, "closed: expected value");
}

int main(int argc, char **argv)
{
    plan(8);

    std::clog.rdbuf(new Log("testing", 1));
    std::clog << syslogErr << "success" << std::endl;
    dynamic_cast<Log *>(std::clog.rdbuf())->close();

    test_opened();
    test_identity();
    test_options();
    test_facility();
    test_priority();
    test_format();
    test_logmsg();
    test_closed();

    return exit_status();
}
