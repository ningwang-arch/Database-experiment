#include "http_server.h"

HttpServer::HttpServer() {
  this->addr = "0.0.0.0:8080";
  opts.extra_headers = "Access-Control-Allow-Origin: *";
  opts.extra_headers = "Access-Control-Allow-Headers: Origin, "
                       "X-Requested-With, content-Type, Accept, Authorization";
}

HttpServer::HttpServer(const std::string addr) {
  this->addr = addr;
  opts.extra_headers = "Access-Control-Allow-Origin: *";
}

void HttpServer::start() {
  mg_mgr_init(&mgr);
  mg_connection *c =
      mg_http_listen(&mgr, addr.c_str(), OnHttpWebsocketEvent, &mgr);
  if (c == NULL) {
    LOG(LL_ERROR, ("bind to address %s", addr.c_str()));
    exit(-1);
  }

  while (true) {
    mg_mgr_poll(&mgr, 1000);
  }
}

void HttpServer::addAuthentication(const std::string &username,
                                   const std::string &password,
                                   const std::string &token) {

  authentication_info info = {
      .username = username, .password = password, .token = token};
  v.push_back(info);
}

void HttpServer::removeAuthentication(const std::string &username) {
  for (auto it = v.begin(); it != v.end(); it++) {
    if (it->username == username) {
      v.erase(it);
      break;
    }
  }
}

authentication_info *HttpServer::getAuthen(mg_http_message *hm) {
  char user[256], pass[256];
  mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass));
  if (user[0] != '\0' && pass[0] != '\0') {
    for (std::vector<authentication_info>::iterator it = v.begin();
         it != v.end(); it++) {
      if (it->username == user && it->password == pass) {
        return &(*it);
      }
    }
  } else if (user[0] == '\0') {
    for (std::vector<authentication_info>::iterator it = v.begin();
         it != v.end(); it++) {
      if (it->token == pass) {
        return &(*it);
      }
    }
  }
  return NULL;
}

void HttpServer::registerReqHandler(const std::string &path, ReqHandler handler,
                                    std::vector<std::string> method_list) {
  if (req_handler_map.find(path) != req_handler_map.end())
    return;
  url_conf uc = {.handler = handler, .method_list = method_list};

  req_handler_map.insert(std::make_pair(path, uc));
}

void HttpServer::deleteReqHandler(const std::string &path) {
  if (req_handler_map.find(path) != req_handler_map.end())
    req_handler_map.erase(path);
}

void HttpServer::SendHttpResponse(struct mg_connection *conn, const char *data,
                                  const char *extra_headers) {
  Json::Value root;
  strToJson(std::string(data), root);
  int status = root["status"].asInt();
  std::string message = root["message"].asString();
  std::string response_str =
      "HTTP/1.1 %d %s\r\nContent-Type: application/json\n%sCache-Control: "
      "no-cache\nContent-Length: %d\nAccess-Control-Allow-Origin: *\n\n\n%s";
  int length = strlen(data) + 1;
  boost::format formatter(response_str);
  std::string ret_data =
      (formatter % status % message % extra_headers % length % data).str();
  mg_printf(conn, ret_data.c_str());

  mg_send(conn, "", 0);
}

void HttpServer::stop() { mg_mgr_free(&mgr); }

void HttpServer::HandleHttpRequest(struct mg_connection *c,
                                   mg_http_message *http_req) {
  std::string url = std::string(http_req->uri.ptr, http_req->uri.len);
  std::string req_method =
      std::string(http_req->method.ptr, http_req->method.len);
  Json::Value res_body;
  Json::StreamWriterBuilder builder;
  authentication_info *authen;

  if (req_method == "OPTIONS") {
    res_body["status"] = HTTP_OK;
    res_body["message"] = "success";
    res_body["data"] = "";
    std::string extra_headers =
        "Access-Control-Allow-Headers: Origin, X-Requested-With, content-Type, "
        "Accept, Authorization\n";
    SendHttpResponse(c, Json::writeString(builder, res_body).c_str(),
                     extra_headers.c_str());
    return;
  }

  if (find(authentication_need.begin(), authentication_need.end(), url) !=
      authentication_need.end()) {
    authen = getAuthen(http_req);
    if (authen == NULL) {
      res_body["status"] = HTTP_FORBIDDEN;
      res_body["message"] = "Unauthorized";
      res_body["data"] = "";
      SendHttpResponse(c, Json::writeString(builder, res_body).c_str());
      return;
    }
  }
  // for images
  if (mg_http_match_uri(http_req, "/images/#")) {
    struct mg_http_serve_opts opt;
    opt = {.mime_types = "png=image/png,jpeg=image/jpeg,jpg=image/png"};
    mg_http_serve_file(c, http_req, ("." + url).c_str(), &opt); // Send file
    return;
  }
  auto it = req_handler_map.find(url);
  if (it != req_handler_map.end()) {
    int status = HTTP_OK;
    std::string reason = "success";
    ReqHandler handler = it->second.handler;
    std::vector<std::string> method_list = it->second.method_list;

    if (find(method_list.begin(), method_list.end(), req_method) ==
        method_list.end()) {
      status = HTTP_METHOD_NOT_ALLOWED;
      reason = "method not allow";
    }

    res_body["status"] = status;
    res_body["message"] = reason;
    if (status == HTTP_OK) {
      handler(http_req, c, &HttpServer::SendHttpResponse);
      return;
    } else {
      res_body["data"] = "";
    }
  } else {
    res_body["status"] = HTTP_NOT_FOUND;
    res_body["reason"] = "page not found";
    res_body["data"] = "";
  }
  SendHttpResponse(c, Json::writeString(builder, res_body).c_str());
  return;
}

void HttpServer::OnHttpWebsocketEvent(mg_connection *connection, int ev,
                                      void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    auto *http_req = (mg_http_message *)ev_data;
    HandleHttpRequest(connection, http_req);
  }
}

HttpServer::~HttpServer() { stop(); }
