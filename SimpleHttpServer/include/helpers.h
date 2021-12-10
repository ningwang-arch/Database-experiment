#ifndef HELPERS_H
#define HELPERS_H

#include "mongoose.h"
#include <functional>
#include <iostream>
#include <json/json.h>
#include <list>
#include <mbedtls/aes.h>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#define GET(server, route, handler) \
    server->registerReqHandler(route, handler, { "GET" })
#define POST(server, route, handler) \
    server->registerReqHandler(route, handler, { "POST" })

#define GET_POST(server, route, handler) \
    server->registerReqHandler(route, handler, { "GET", "POST" })

typedef void OnRspCallBack(struct mg_connection* conn, const char* data,
    const char* extra_headers);

using ReqHandler = std::function<void(mg_http_message*, mg_connection* conn, OnRspCallBack)>;

const std::vector<std::string> authentication_need = { "/updatepwd", "/book",
    "/get_order", "/get_info", "/ihouse", "/icommunity", "/m_logout" };

struct url_conf {
    ReqHandler handler;
    std::vector<std::string> method_list;
};

struct authentication_info {
    std::string username;
    std::string password;
    std::string token;
};

enum {
    AUTH_FAILED = 0,
    BOOK_FAILED = 1,
    HTTP_OK = 200,
    HTTP_FORBIDDEN = 403,
    HTTP_MOVE_PERMANENTLY = 301,
    HTTP_MOVED_TEMPORARILY = 302,
    HTTP_BAD_REQUEST = 400,
    HTTP_NOT_FOUND = 404,
    HTTP_PRECONDITION_FAILED = 412,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_METHOD_NOT_ALLOWED = 405,
};

bool strToJson(std::string body, Json::Value& root);

std::string bauth_get(const std::string user, const std::string pass);

std::string token_generated(int length = 6);

std::string pass_generated(const std::string user, const std::string token);

std::string pass_decode(const std::string pass, const std::string token);

#endif