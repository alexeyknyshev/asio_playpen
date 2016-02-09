#include "uri.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

Uri::Uri(const std::string &uriString)
{
    parse(decode(uriString));
}

Uri::Uri(const Uri &other)
    : mProtocol(other.mProtocol),
      mHost(other.mHost),
      mPort(other.mPort),
      mPath(other.mPath),
      mQuery(other.mQuery)
{ }

void Uri::parse(const std::string &uriString)
{
    using namespace std;
    const string prot_end("://");
    string::const_iterator prot_i = search(uriString.begin(), uriString.end(),
                                           prot_end.begin(), prot_end.end());
    mProtocol.reserve(distance(uriString.begin(), prot_i));
    transform(uriString.begin(), prot_i,
              back_inserter(mProtocol),
              ptr_fun<int,int>(tolower)); // protocol is icase
    if (prot_i == uriString.end()) {
        return;
    }
    advance(prot_i, prot_end.length());
    string::const_iterator path_i = find(prot_i, uriString.end(), '/');
    mHost.reserve(distance(prot_i, path_i));
    transform(prot_i, path_i,
              back_inserter(mHost),
              ptr_fun<int,int>(tolower)); // host is icase
    size_t port_i = mHost.find(':');
    if (port_i != string::npos) {
        if (port_i + 1 < mHost.size()) {
            mPort = mHost.substr(port_i + 1);
        }
        mHost.erase(port_i);
    }

    string::const_iterator query_i = find(path_i, uriString.end(), '?');
    mPath.assign(path_i, query_i);
    if( query_i != uriString.end() ) {
        ++query_i;
    }
    mQuery.assign(query_i, uriString.end());
}

std::string Uri::decode(const std::string &uriString)
{
    std::string ret;
    for (int i = 0; i < uriString.size(); i++) {
        if (int(uriString[i]) == 37) {
            int ii;
            sscanf(uriString.substr(i + 1, 2).c_str(), "%x", &ii);
            char ch = static_cast<char>(ii);
            ret+=ch;
            i=i+2;
        } else {
            ret += uriString[i];
        }
    }
    return ret;
}

std::string Uri::encode(const std::string &uriString)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    const auto end = uriString.cend();
    for (auto it = uriString.cbegin(); it != end; ++it) {
        std::string::value_type c = *it;

        // Keep alphanumeric and other accepted characters intact
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}
