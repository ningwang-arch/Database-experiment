#pragma once
#include <json/json.h>
#include <boost/format.hpp>
#include <list>
#include <vector>
#include "mongoose.h"
#include "helpers.h"

class HttpServer
{
public:
    HttpServer();
    HttpServer(std::string addr);
    ~HttpServer();

    void start();
    void stop();
    static authentication_info *getAuthen(mg_http_message *hm);
    void registerReqHandler(const std::string &path, ReqHandler handler, std::vector<std::string> method_list);
    void deleteReqHandler(const std::string &path);
    void addAuthentication(const std::string &username, const std::string &password, const std::string &token);
    void removeAuthentication(const std::string &username);

    static mg_http_serve_opts opts;
    static std::unordered_map<std::string, url_conf> req_handler_map;
    static std::vector<authentication_info> v;

private:
    static void OnHttpWebsocketEvent(mg_connection *connection, int ev, void *ev_data, void *fn_data);
    static void HandleHttpRequest(struct mg_connection *c, mg_http_message *http_req);
    static void SendHttpResponse(struct mg_connection *conn, const char *data, const char *extra_headers = "");

    std::string addr;
    mg_mgr mgr;
};