#pragma once

#include <memory>
#include <functional>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

class ServerConfig;

class Server
{
public:
    Server(std::shared_ptr<ServerConfig> config, boost::asio::io_service &ioService);

    void join();
    void accept();

    typedef std::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;

    struct Request {
        friend class Server;

        std::string type;
        std::string url;
        std::string version;

        void parse();

        mutable boost::asio::streambuf buf;

    private:
        Request(const SocketPtr &ptr);
        const SocketPtr mSocket;
    };
    typedef std::shared_ptr<Request> RequestPtr;

    struct Response {
        Response()
            : httpCode(0)
        { }

        enum HttpCode : uint {
            HttpCode_OK  = 200,
            HttpCode_UnsupportedMediaType = 415,
            HttpCode_RequestedHostUnavailable = 434,
            HttpCode_NotImplemented = 501,
            HttpCode_HTTPVersionNotSupported = 505,
        };

        uint httpCode;
        std::string data;

        std::string getHttpCodeText() const;

        boost::asio::streambuf buf;        
    };
    typedef std::shared_ptr<Response> ResponsePtr;

    typedef std::function<void(const ResponsePtr &res)> ResponseCallback;
    typedef std::function<void(const RequestPtr &req, ResponsePtr &res, ResponseCallback callback)> HandlerFunc;
    void setHandlerFunc(HandlerFunc handler) { mHandler = handler; }

private:
    void readDataFromSocket(const SocketPtr &socket);

    std::shared_ptr<ServerConfig> mConfig;
    std::unique_ptr<boost::thread_group> mThreadGroup;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> mAcceptor;

    boost::asio::io_service &mIOService;

    HandlerFunc mHandler;
};

std::ostream &operator<< (std::ostream &o, const Server::Response &res);
