#pragma once

#include <string>

namespace boost {
    namespace program_options {
        class options_description;
        class variables_map;
    }
}

class ServerConfig
{
public:
    ServerConfig(int argc, char *argv[]);
    ~ServerConfig();

    unsigned getPort() const { return mPort; }
    unsigned getThreadCount() const { return mThreadCount; }
    unsigned getRequestTimeout() const { return mRequestTimeout; }
    bool getShowHelp() const { return mShowHelp; }
    const std::string &getConfigFilePath() const { return mConfigFilePath; }

    void showHelp() const;
    void print() const;

    operator bool() const { return mOk; }


private:
    bool loadConfigFile(const std::string &path);
    void applyCmdLineOptions(const boost::program_options::variables_map &vars);

    unsigned mPort;
    unsigned mThreadCount;
    unsigned mRequestTimeout;
    std::string mConfigFilePath;

    bool mShowHelp;
    bool mOk;

    boost::program_options::options_description *mDesc;
};
