#include "server.h"
#include "serverconfig.h"

std::string Server::Response::getHttpCodeText() const
{
    switch (httpCode) {
    case HttpCode_OK: return "OK";
    case HttpCode_UnsupportedMediaType: return "Unsupported Media Type";
    case HttpCode_RequestedHostUnavailable: return "Requested host unavailable";
    case HttpCode_NotImplemented: return "Not Implemented";
    case HttpCode_HTTPVersionNotSupported: return "HTTP Version Not Supported";
    }

    return "";
}

std::ostream &operator <<(std::ostream &o, const Server::Response &res)
{
    o << "HTTP/1.1 " << res.httpCode << " " << res.getHttpCodeText() << "\r\n"
      << "Server: " << "MyHumbleRssProxy" << "\r\n"
      << "Connection: " << "close" << "\r\n";

    auto end = res.headers.cend();
    for (auto it = res.headers.begin(); it != end; it++) {
        o << it->first << ": " << it->second << "\r\n";
    }

    if (res.httpCode == Server::Response::HttpCode_OK) {
        o << "Access-Control-Allow-Origin: " << "*" << "\r\n";
        o << "\r\n" << res.body;
    } else {
        o << "\r\n";
    }

    return o;
}

Server::Request::Request(const Server::SocketPtr &socket)
    : mSocket(socket)
{ }

void Server::Request::parse()
{
    std::istream bufStream(&buf);

    std::string line;
    std::getline(bufStream, line);

    if (!line.empty()) {
        line.replace(line.size() - 1, 1, "");
    }

    std::stringstream ss(line);

    std::getline(ss, type, ' ');
    std::getline(ss, url, ' ');
    std::getline(ss, version, ' ');
}

Server::Server(std::shared_ptr<ServerConfig> config, boost::asio::io_service &ioService)
    : mConfig(config),
      mIOService(ioService)
{
    config->print();

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), config->getPort());

    mAcceptor.reset(new boost::asio::ip::tcp::acceptor(mIOService));
    mAcceptor->open(endpoint.protocol());
    mAcceptor->bind(endpoint);
    mAcceptor->listen();

    accept();

    mThreadGroup.reset(new boost::thread_group);
    for (int i = 0; i < config->getThreadCount(); i++) {
        mThreadGroup->create_thread([this]() {
            mIOService.run();
        });
    }
}

void Server::accept()
{
    std::shared_ptr<boost::asio::ip::tcp::socket> socket(new boost::asio::ip::tcp::socket(mIOService));
    mAcceptor->async_accept(*socket, [this, socket](const boost::system::error_code &err) {
        accept();

        if (!err) {
            socket->set_option(boost::asio::ip::tcp::no_delay(true));

            readDataFromSocket(socket);
        }
    });
}

void Server::setHandlerFunc(HandlerFunc handler)
{
    LockGuard g(mHandlerMutex);
    mHandler = handler;
}

void Server::readDataFromSocket(const SocketPtr &socket)
{/*
    // set request exp timeout
    std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(mIOService));
    timer->expires_from_now(boost::posix_time::milliseconds(mConfig->getRequestTimeout()));
    timer->async_wait([socket](const boost::system::error_code &err) {
        if (!err) {
            Response res;
            res.httpCode = Response::HttpCode_RequestedHostUnavailable;
            std::ostream o(&res.buf);
            o << res;
            boost::asio::write(*socket, res.buf);
            boost::system::error_code ec;
            socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket->lowest_layer().close();
        }
    });*/

    // read header data
    RequestPtr req(new Request(socket));
    boost::asio::async_read_until(*socket, req->buf, "\r\n\r\n",
    [socket, req, /*timer,*/ this](const boost::system::error_code &err, size_t) {
//        timer->cancel();

        if (err) {
            return;
        }

        req->parse();

        ResponsePtr res(new Response);

        LockGuard g(mHandlerMutex);
        if (mHandler) {
            mHandler(req, res, [socket](const ResponsePtr &res) {
                if (res->httpCode == 0) {
                    res->httpCode = (uint)Server::Response::HttpCode_OK;
                }

                std::ostream o(&res->buf);
                o << *res;

                boost::asio::async_write(*socket, res->buf, [socket, res](const boost::system::error_code &, std::size_t) {});
            });
        }
    });
}

void Server::join()
{
    mThreadGroup->join_all();
}
