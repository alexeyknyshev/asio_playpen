#include "serverconfig.h"
#include "boost/program_options.hpp"
#include <fstream>
#include <iostream>

#include "rapidjson/document.h"

ServerConfig::ServerConfig(int argc, char *argv[])
    : mPort(8080),
      mThreadCount(1),
      mRequestTimeout(1000),
      mShowHelp(false),
      mOk(true)
{
    using namespace boost::program_options;

    mDesc = new options_description("Allowed options");
    mDesc->add_options()
            ("help", "produce help message")
            ("port", value<int>(), "server port")
            ("threads", value<int>(), "count of server's' worker threads")
            ("timeout", value<int>(), "remote host timeout")
            ("config", value<std::string>(), "config file path");

    variables_map vars;

    try {
        store(parse_command_line(argc, argv, *mDesc), vars);
    } catch (const boost::program_options::error &err) {
        std::cerr << err.what() << std::endl;
        mOk = false;
        return;
    }

    if (vars.count("config") > 0) {
        mConfigFilePath = vars["config"].as<std::string>();
        mOk = loadConfigFile(mConfigFilePath);
    } else {
        std::cerr << "No such required cmd line arg: " << "config" << std::endl;
        std::cerr << std::endl;
        mOk = false;
    }

    if (!mOk) {
        return;
    }

    applyCmdLineOptions(vars);
}

ServerConfig::~ServerConfig()
{
    delete mDesc;
}

void ServerConfig::showHelp() const
{
   std::cout << "RSS proxy server" << std::endl
             << *mDesc << std::endl;
}

void ServerConfig::print() const
{
    std::cout << "port:\t\t" << mPort << std::endl
              << "threads:\t" << mThreadCount << std::endl
              << "timeout:\t" << mRequestTimeout << std::endl;
}

bool ServerConfig::loadConfigFile(const std::string &path)
{
    std::ifstream configFS;
    configFS.open(path);
    if (configFS.is_open()) {
        std::string configJsonData((std::istreambuf_iterator<char>(configFS)),
                                   std::istreambuf_iterator<char>());
        rapidjson::Document d;
        if (d.Parse(configJsonData.c_str()).HasParseError()) {
            std::cerr << "failed to parse config file json" << std::endl;
            return false;
        }

        if (!d.IsObject()) {
            std::cerr << "config json must be a json object" << std::endl;
            return false;
        }

        const std::string missingValueErr = "no such required config value: ";
        if (!d.HasMember("port")) {
            std::cerr << missingValueErr << "port" << std::endl;
            return false;
        }
        if (!d["port"].IsInt()) {
            std::cerr << "json field 'port'' must be int" << std::endl;
            return false;
        }

        mPort = d["port"].GetInt();

        if (!d.HasMember("threads")) {
            std::cerr << missingValueErr << "threads" << std::endl;
            return false;
        }
        if (!d["threads"].IsInt()) {
            std::cerr << "json field 'threads' must be int" << std::endl;
            return false;
        }
        mThreadCount = d["threads"].GetInt();

        if (!d.HasMember("timeout")) {
            std::cerr << missingValueErr << "timeout" << std::endl;
            return false;
        }
        if (!d["timeout"].IsInt()) {
            std::cerr << "json field 'timeout' must be int" << std::endl;
            return false;
        }
        mRequestTimeout = d["timeout"].GetInt();

        return true;
    }
    return false;
}

void ServerConfig::applyCmdLineOptions(const boost::program_options::variables_map &vars)
{
    if (vars.count("help") > 0) {
        mShowHelp = true;
        return;
    }

    if (vars.count("port") > 0) {
        int port = vars["port"].as<int>();
        if (port > 0) {
            mPort = port;
        } else {
            std::cerr << "Invalid server port cmd line arg" << std::endl;
        }
    }

    if (vars.count("threads") > 0) {
        int threads = vars["threads"].as<int>();
        if (threads > 0) {
            mThreadCount = threads;
        } else {
            std::cerr << "Invalid thread count cmd line arg" << std::endl;
        }
    }

    if (vars.count("timeout") > 0) {
        int timeout = vars["timeout"].as<int>();
        if (timeout > 0) {
            mRequestTimeout = timeout;
        } else {
            std::cerr << "Invalid request timeout cmd line arg" << std::endl;
        }
    }
}
