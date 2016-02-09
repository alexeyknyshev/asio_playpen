#pragma once

#include <ctime>
#include <string>

class RFC882
{
public:
    static std::time_t toUTC(const std::string &rfc882, bool &ok);
};
