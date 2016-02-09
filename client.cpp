#include "client.h"

#include <iostream>
#include <boost/bind.hpp>

#include "uri.h"

#define SUPPORTED_HTTP_VERSION "HTTP/1.1"

namespace {

std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

} // namespace

std::ostream &operator <<(std::ostream &o, const Client::Request &req)
{
    const std::string path = req.path.empty() ? "/" : req.path;
    o << req.type << " " << path << " " << SUPPORTED_HTTP_VERSION << "\r\n"
      << "Host: " << req.host << "\r\n"
      << "Accept: " << "*/*" << "\r\n"
      << "Connection: " << "close" << "\r\n\r\n";

    return o;
}

void Client::Response::parseHeaders()
{
    std::istream bufStream(&buf);

    std::string line;
    std::getline(bufStream, line);

    if (!line.empty()) {
        line.replace(line.size() - 1, 1, "");
    }

    std::stringstream ss(line);

    std::getline(ss, version, ' ');

    std::string codeStr;
    std::getline(ss, codeStr, ' ');

    std::stringstream sc(codeStr);
    sc >> httpCode;

    while (std::getline(bufStream, line)) {
        if (!line.empty()) {
            line.replace(line.size() - 1, 1, "");
            if (line.empty()) {
                break;
            }
            auto delim = line.find_first_of(':');
            if (delim == std::string::npos) {
                continue;
            }

            const std::string key = line.substr(0, delim);
            line.erase(0, delim + 1);

            line = ::trim(line);

            headers[key] = line;
        }
    }
}

/// ==========================================================================

Client::Client(boost::asio::io_service &service)
    : mIOService(service),
      mResolver(service),
      mStrand(service)
{
    mSocket = std::make_shared<boost::asio::ip::tcp::socket>(service);
}

void Client::sendRequest(const std::string &reqType, const std::string &url, unsigned timeout, HandlerFunc func)
{
    mUri = Uri(url);
    mRequestType = reqType;

    const std::string &port = mUri.getPort();
    if (port.empty()) {
        mUri.setPort("80");
    }

    auto thisPtr = shared_from_this();

    mFinished = false;
    mWithTimeout = false;

    if (timeout > 0) {
        mWithTimeout = true;
        mTimer = std::make_shared<boost::asio::deadline_timer>(mIOService);
        mTimer->expires_from_now(boost::posix_time::milliseconds(timeout));
        mTimer->async_wait(mStrand.wrap([thisPtr, func](const boost::system::error_code &err) {
            if (!err) {
                if (!thisPtr->mFinished) {
                    thisPtr->mFinished = true;

                    boost::system::error_code ec;
                    thisPtr->mSocket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    thisPtr->mSocket->lowest_layer().close();

                    ResponsePtr res(new Response);
                    res->httpCode = 434;
                    res->version = SUPPORTED_HTTP_VERSION;
                    func(res);
                }
            }
        }));
    }

    boost::asio::ip::tcp::resolver::query query(mUri.getHost(), mUri.getPort());
    //socket->set_option(boost::asio::ip::tcp::no_delay(true));

    mResolver.async_resolve(query, mStrand.wrap(
    [func, thisPtr](const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator it) {
        if (err) {
            if (!thisPtr->mFinished) {
                thisPtr->finished();

                ResponsePtr res(new Response);
                res->httpCode = 434;
                res->version = SUPPORTED_HTTP_VERSION;
                func(res);
            }
            return;
        }

        if (!thisPtr->mFinished) {
            boost::asio::ip::tcp::endpoint endpoint = *it;
            thisPtr->mSocket->async_connect(endpoint, thisPtr->mStrand.wrap(boost::bind(&Client::onConnect, thisPtr, func,
                                                                            boost::asio::placeholders::error, ++it)));
        }
    }));
}

void Client::onConnect(HandlerFunc func, const boost::system::error_code &err, boost::asio::ip::tcp::resolver::iterator it)
{
    if (!err) {
        RequestPtr req(new Request);
        req->type = mRequestType;
        req->host = mUri.getHost();
        req->path = mUri.getPath();

        std::ostream s(&req->buf);
        s << *req;

        auto thisPtr = shared_from_this();

        if (mFinished) {
            return;
        }

        boost::asio::async_write(*thisPtr->mSocket, req->buf, mStrand.wrap(
        [thisPtr, req, func](const boost::system::error_code &err, std::size_t) {
            if (err) {
                if (!thisPtr->mFinished) {
                    thisPtr->finished();

                    ResponsePtr res(new Response);
                    res->httpCode = 434;
                    res->version = SUPPORTED_HTTP_VERSION;
                    func(res);
                }
                return;
            }

            if (thisPtr->mFinished) {
                return;
            }

            ResponsePtr res(new Response);

            boost::asio::async_read_until(*thisPtr->mSocket, res->buf, "\r\n\r\n", thisPtr->mStrand.wrap(
            [thisPtr, res, func](const boost::system::error_code &err, size_t) {
                if (thisPtr->mFinished) {
                    return;
                }

                if (err) {
                    thisPtr->finished();
                    ResponsePtr res(new Response);
                    res->httpCode = 434;
                    res->version = SUPPORTED_HTTP_VERSION;
                    func(res);
                    return;
                }

                res->parseHeaders();

                if (res->version != SUPPORTED_HTTP_VERSION) {
                    thisPtr->finished();

                    res->httpCode = 434;
                    res->version = SUPPORTED_HTTP_VERSION;

                    func(res);
                } else if (res->httpCode == 200 && !thisPtr->mFinished) {

                    auto it = res->headers.find("Content-Length");
                    if (it != res->headers.end()) {
                        std::stringstream ss(it->second);
                        size_t length = 0;
                        ss >> length;

                        if (length > 0) {
                            boost::asio::async_read(*thisPtr->mSocket, res->buf, boost::asio::transfer_at_least(1),
                                                    thisPtr->mStrand.wrap(boost::bind(&Client::onDataRead, thisPtr, func, res,
                                                                          boost::asio::placeholders::error)));
                        } else {
                            thisPtr->finished();

                            func(res);
                        }
                    } else {
                        it = res->headers.find("Transfer-Encoding");
                        if (it != res->headers.end() && it->second == "chunked") {
                            boost::asio::async_read(*thisPtr->mSocket, res->buf, boost::asio::transfer_at_least(1),
                                                    thisPtr->mStrand.wrap(boost::bind(&Client::onDataRead, thisPtr, func, res,
                                                                          boost::asio::placeholders::error)));
                        } else {
                            // malformed http response
                            thisPtr->finished();

                            res->httpCode = 434;
                            res->version = SUPPORTED_HTTP_VERSION;

                            func(res);
                        }
                    }
                } else {
                    thisPtr->finished();
                    func(res);
                }
            }));
        }));
    } else if (it != boost::asio::ip::tcp::resolver::iterator()) {
        mSocket->close();
        boost::asio::ip::tcp::endpoint endpoint = *it;
        mSocket->async_connect(endpoint, mStrand.wrap(boost::bind(&Client::onConnect, shared_from_this(), func,
                                                      boost::asio::placeholders::error, ++it)));
    } else {
        //std::cout << err.message() << std::endl;
        if (mFinished) {
            return;
        }

        finished();

        ResponsePtr res(new Response);
        res->httpCode = 434;
        res->version = SUPPORTED_HTTP_VERSION;

        func(res);
    }
}

void Client::onDataRead(HandlerFunc func, ResponsePtr res, const boost::system::error_code &err)
{
    if (!err) {
        std::istream ss(&res->buf);
        std::copy(std::istreambuf_iterator<char>(ss), {}, std::back_inserter(res->body));

        boost::asio::async_read(*mSocket, res->buf, boost::asio::transfer_at_least(1),
                                mStrand.wrap(boost::bind(&Client::onDataRead, shared_from_this(), func,
                                             res, boost::asio::placeholders::error)));
    } else if (err != boost::asio::error::eof) {
        finished();
        ResponsePtr res(new Response);

        res->httpCode = 434;
        res->version = SUPPORTED_HTTP_VERSION;

        func(res);
    } else {
        finished();
        func(res);
    }
}

void Client::finished()
{
    if (!mFinished) {
        mFinished = true;
        if (mWithTimeout) {
            mTimer->cancel();
        }
    }
}
