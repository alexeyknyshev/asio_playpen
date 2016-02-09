#pragma once

#include <functional>
#include <memory>

#include <boost/asio.hpp>

#include "uri.h"

class Client : public std::enable_shared_from_this<Client>
{
public:
    Client(boost::asio::io_service &service);

    class Request {
        friend class Client;

    public:
        std::string type;
        std::string host;
        std::string path;

    private:
        boost::asio::streambuf buf;
    };
    typedef std::shared_ptr<Request> RequestPtr;

    class Response {
        friend class Client;

    public:
        std::string version;
        uint httpCode;

        std::string body;

        std::map<std::string, std::string> headers;

    private:
        void parseHeaders();

        boost::asio::streambuf buf;
    };
    typedef std::shared_ptr<Response> ResponsePtr;

    typedef std::function<void(const ResponsePtr &res)> HandlerFunc;
    void sendRequest(const std::string &reqType, const std::string &url, unsigned timeout, HandlerFunc func);

private:
    boost::asio::io_service &mIOService;
    std::shared_ptr<boost::asio::ip::tcp::socket> mSocket;
    std::shared_ptr<boost::asio::deadline_timer> mTimer;
    boost::asio::ip::tcp::resolver mResolver;
    bool mFinished;
    bool mWithTimeout;

    void finished();

    boost::asio::strand mStrand;

    Uri mUri;
    std::string mRequestType;

    void onConnect(HandlerFunc func,
                   const boost::system::error_code &err,
                   boost::asio::ip::tcp::resolver::iterator it);

    void onDataRead(HandlerFunc func,
                    ResponsePtr res,
                    const boost::system::error_code &err);
};

std::ostream &operator<< (std::ostream &o, const Client::Request &req);
