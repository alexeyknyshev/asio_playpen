#include "rfc882.h"

std::time_t RFC882::toUTC(const std::string &rfc882, bool &ok)
{
    tm time;

    char *r = strptime(rfc882.c_str(), "%a, %e %h %Y %H:%M:%S %z", &time);
    if (r == NULL) {
        ok = false;
    } else {
        ok = true;
    }

    return mktime(&time);
}
