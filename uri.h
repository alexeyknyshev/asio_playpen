#pragma once

#include <string>

class Uri {
public:
    Uri() { }
    Uri(const std::string &uriString);
    Uri(const Uri &other);

    const std::string &getProtocol() const { return mProtocol; }
    const std::string &getHost() const { return mHost; }
    const std::string &getPort() const { return mPort; }
    const std::string &getPath() const { return mPath; }
    const std::string &getQuery() const { return mQuery; }

    void setProtocol(const std::string &protocol) { mProtocol = protocol; }
    void setHost(const std::string &host) { mHost = host; }
    void setPort(const std::string &port) { mPort = port; }
    void setPath(const std::string &path) { mPath = path; }
    void setQuery(const std::string &query) { mQuery = query; }

    static std::string decode(const std::string &uriString);
    static std::string encode(const std::string &uriString);

private:
    void parse(const std::string &uriString);

private:
    std::string mProtocol, mHost, mPort, mPath, mQuery;
};
