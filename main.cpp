#include <iostream>

#include <pugixml.hpp>

#include "serverconfig.h"
#include "server.h"

#include "client.h"

#include "uri.h"

std::string convertRssToJson(const std::string &rssString, bool &ok)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(rssString.c_str(), rssString.size());

    std::string jsonStr;
    if (result) {
        pugi::xml_node rss = doc.child("rss");
        const std::string version = rss.attribute("version").as_string();
        if (version != "2.0") {
            ok = false;
            return "";
        }
        pugi::xml_node channel = rss.child("channel");
        pugi::xml_node title = channel.child("title");
        pugi::xml_node desc = channel.child("description");

        for (pugi::xml_node item = rss.child("item"); item; item.next_sibling("item")) {
            pugi::xml_node itemTitle = item.child("title");
            pugi::xml_node itemLink = item.child("link");
            pugi::xml_node itemDesc = item.child("description");
            pugi::xml_node itemPubDate = item.child("pubDate");
        }
    } else {
        ok = false;
    }
    return jsonStr;
}

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

    server.setHandlerFunc([&ioService](const Server::RequestPtr &req, Server::ResponsePtr &res,
                          Server::ResponseCallback resCallback)
    {
        std::cout << req->type << " " << req->url << " " << req->version << std::endl;

        if (req->type != "GET") {
            res->httpCode = Server::Response::HttpCode_NotImplemented;
            resCallback(res);
            return;
        }

        if (req->version != "HTTP/1.1") {
            res->httpCode = Server::Response::HttpCode_HTTPVersionNotSupported;
            resCallback(res);
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
        client->sendRequest("GET", urlString, [resCallback, res](const Client::ResponsePtr &resCli) {
            if (resCli->httpCode != Server::Response::HttpCode_OK) {
                res->httpCode = resCli->httpCode;
            } else {

            }
            resCallback(res);
        });
//        std::shared_ptr<boost::asio>
    });

    server.join();

    return 0;
}
