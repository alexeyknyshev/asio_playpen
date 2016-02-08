#include <iostream>

#include "serverconfig.h"
#include "server.h"

#include "client.h"

#include "uri.h"

class ConsoleWriter {
public:
    void write(const std::string &data)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        counter++;
        std::cout << counter << std::endl;
    }

private:
    std::uint64_t counter;
    boost::mutex mutex;
};

int main(int argc, char *argv[])
{
    auto conf = std::make_shared<ServerConfig>(argc, argv);
    if (conf->getShowHelp()) {
        conf->showHelp();
        return 0;
    }

    if (!*conf) {
        std::cerr << "cannot configure server" << std::endl;
        return 1;
    }

    boost::asio::io_service ioService;

    Server server(conf, ioService);

    ConsoleWriter writer;

    server.setHandlerFunc([&ioService, &writer](const Server::RequestPtr &req, Server::ResponsePtr &res,
                          Server::ResponseCallback resCallback)
    {
        //std::cout << req->type << " " << req->url << " " << req->version << std::endl;

        if (req->type != "GET") {
            res->httpCode = Server::Response::HttpCode_NotImplemented;
            return;
        }

        if (req->version != "HTTP/1.1") {
            res->httpCode = Server::Response::HttpCode_HTTPVersionNotSupported;
            return;
        }

        const std::string reqPrefix = "/?url=";
        if (req->url.substr(0, reqPrefix.size()) != reqPrefix) {
            res->httpCode = Server::Response::HttpCode_NotImplemented;
            return;
        }

        std::string urlString = req->url.substr(reqPrefix.size());

//        Uri uri(urlString);
//        std::cout << "Host: " << uri.getHost() << std::endl;
//        std::cout << "Port: " << uri.getPort() << std::endl;
//        std::cout << "Path: " << uri.getPath() << std::endl;
//        std::cout << "Protocol: " << uri.getProtocol() << std::endl;
//        std::cout << "Query: " << uri.getQuery() << std::endl;

        auto client = std::make_shared<Client>(ioService);
        client->sendRequest("GET", urlString, [resCallback, res, &writer](const Client::ResponsePtr &resCli) {
            if (resCli->httpCode != Server::Response::HttpCode_OK) {
                res->httpCode = resCli->httpCode;
            } else {
                writer.write(resCli->body);
            }
            resCallback(res);
        });
    });

    server.join();

    return 0;
}
