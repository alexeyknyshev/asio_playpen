#include <iostream>

#include <boost/date_time.hpp>

#include <pugixml.hpp>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "serverconfig.h"
#include "server.h"

#include "client.h"

#include "uri.h"

#include "rfc882/rfc882.h"

std::string convertRssToJson(const std::string &rssString, bool &ok)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(rssString.c_str(), rssString.size());

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

        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> w(s);

        w.StartObject();
        w.String("channel");
        w.StartObject();
        w.String("title");
        if (title) {
            w.String(title.text().as_string());
        } else {
            w.Null();
        }
        w.String("description");
        if (desc) {
            w.String(desc.text().as_string());
        } else {
            w.Null();
        }
        w.String("items");
        w.StartArray();
        for (pugi::xml_node item = channel.child("item"); item; item = item.next_sibling("item")) {
            pugi::xml_node itemTitle = item.child("title");
            pugi::xml_node itemLink = item.child("link");
            pugi::xml_node itemDesc = item.child("description");
            pugi::xml_node itemPubDate = item.child("pubDate");

            w.StartObject();
            w.String("title");
            if (itemTitle) {
                w.String(itemTitle.text().as_string());
            } else {
                w.Null();
            }
            w.String("link");
            if (itemLink) {
                w.String(itemLink.text().as_string());
            } else {
                w.Null();
            }
            w.String("description");
            if (itemDesc) {
                w.String(itemDesc.text().as_string());
            } else {
                w.Null();
            }
            w.String("pubDate");
            if (itemPubDate) {
                const std::string pubDate = itemPubDate.text().as_string();
                std::time_t utc = RFC882::toUTC(pubDate, ok);
                if (ok) {
                    w.Int64(utc);
                } else {
                    return "";
                }
            } else {
                w.Null();
            }
            w.EndObject();
        }
        w.EndArray();
        w.EndObject();
        w.EndObject();

        ok = true;
        return s.GetString();
    } else {
        ok = false;
    }
    return "";
}
/*
class ConsoleWriter {
public:
    void write(const std::string &data)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        counter++;
        std::cout << counter << std::endl;
        std::cout << data << std::endl;
    }

private:
    std::uint64_t counter;
    boost::mutex mutex;
};
*/

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

//    ConsoleWriter writer;

    unsigned timeout = conf->getRequestTimeout();
    server.setHandlerFunc([&ioService, /*&writer,*/ timeout](const Server::RequestPtr &req, Server::ResponsePtr &res,
                          Server::ResponseCallback resCallback)
    {
        //std::cout << req->type << " " << req->url << " " << req->version << std::endl;

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
            resCallback(res);
            return;
        }

        std::string urlString = req->url.substr(reqPrefix.size());

        auto client = std::make_shared<Client>(ioService);

        client->sendRequest("GET", urlString, timeout, [resCallback, res, &writer](const Client::ResponsePtr &resCli) {
            if (resCli->httpCode != Server::Response::HttpCode_OK) {
                res->httpCode = resCli->httpCode;
            } else {
                bool ok = false;
//                writer.write(resCli->body);
                res->body = convertRssToJson(resCli->body, ok);
                res->headers["Content-Type"] = "application/json; charset=utf-8";
                if (!ok) {
                    res->httpCode = 415;
                }
//                writer.write(json);
            }
            resCallback(res);
        });
    });

    server.join();

    return 0;
}
