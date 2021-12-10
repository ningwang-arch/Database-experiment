#include "App.h"

App::App(std::string addr)
{
    this->addr = addr;

    server = make_shared<HttpServer>(addr);
    cp = ConnectionPool::getConnectionPool();
}

App::~App() { server->stop(); }

void App::init()
{

    GET_POST(this->server, "/",
        [](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            Json::Value root;
            root["status"] = HTTP_OK;
            root["message"] = "success";
            root["data"] = "Hello World";
            Json::StreamWriterBuilder builder;
            cb(conn, Json::writeString(builder, root).c_str(), "");
        });

    POST(this->server, "/get_house",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            int page = root["page"].asInt();
            Json::Value filter = root["filter"];
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            if (filter.size() == 0) {
                ResultSet rs = ResultSet(
                    conn_ptr->query("select * from houses limit " + std::to_string((page - 1) * 20) + ",20"));
                req_body_house(rs, root);
            } else {
                ResultSet rs = get_query_result(conn_ptr, filter, "houses");
                if (rs.size() == 0) {
                    root["status"] = HTTP_OK;
                    root["message"] = "success";
                    root["data"] = "";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                    return;
                }
                ResultSet ret = rs.offset((page - 1) * 20, 20);
                req_body_house(ret, root);
                root["status"] = HTTP_OK;
                root["message"] = "success";
            }
            Json::StreamWriterBuilder builder;
            cb(conn, Json::writeString(builder, root).c_str(), "");
        });

    POST(this->server, "/get_community",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            int page = root["page"].asInt();
            Json::Value filter = root["filter"];
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            if (filter.size() == 0) {
                ResultSet rs = ResultSet(
                    conn_ptr->query("select * from communities limit " + std::to_string((page - 1) * 20) + ",20"));
                req_body_community(rs, root);
            } else {
                ResultSet rs = get_query_result(conn_ptr, filter, "communities");
                if (rs.size() == 0) {
                    root["status"] = HTTP_OK;
                    root["message"] = "success";
                    root["data"] = "";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                    return;
                }
                ResultSet ret = rs.offset((page - 1) * 20, 20);
                req_body_community(ret, root);
                root["status"] = HTTP_OK;
                root["message"] = "success";
            }
            Json::StreamWriterBuilder builder;
            cb(conn, Json::writeString(builder, root).c_str(), "");
        });

    POST(this->server, "/get_details",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            std::string id = root["id"].asString();
            root.clear();
            Result* ret = nullptr;
            ResultSet rs = conn_ptr->query(
                "select * from houses where record_id='" + id + "';");
            while ((ret = rs.next()) != nullptr) {
                Json::Value json;
                json["title"] = ret->getString("title");
                json["normal_url"] = this->img_prefix + ret->getString("normal_url");
                json["record_id"] = ret->getString("record_id");
                json["area_name"] = ret->getString("area_name");
                json["community"] = ret->getString("community");
                json["main_info"] = ret->getString("main_info");
                json["total_price"] = ret->getString("total_price") + "万";
                json["unit_price"] = ret->getString("unit_price") + "元/平";
                json["position"] = ret->getString("direction");
                json["booked"] = ret->getString("booked");
                json["area"] = ret->getString("area") + "平";
                json["prepayment"] = ret->getString("prepayment");
                root["data"] = json;
            }
            root["status"] = HTTP_OK;
            root["message"] = "success";
            Json::StreamWriterBuilder builder;
            cb(conn, Json::writeString(builder, root).c_str(), "");
        });

    POST(this->server, "/login",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            std::string username = root["username"].asString();
            std::string password = root["password"].asString();

            for (std::vector<authentication_info>::iterator it = this->server->v.begin();
                 it != this->server->v.end(); it++) {
                if (it->username == username) {
                    root.clear();
                    root["status"] = AUTH_FAILED;
                    root["message"] = "fail";
                    root["error"] = "用户已登陆";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                    return;
                }
            }

            root.clear();
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            ResultSet rs = conn_ptr->query("select * from users where username='" + username + "' and password='" + password + "';");
            int size = rs.size();
            std::string extra_headers = "";
            if (size == 0) {
                root["status"] = AUTH_FAILED;
                root["message"] = "fail";
                root["error"] = "用户名或密码错误";
            } else {
                root["status"] = HTTP_OK;
                root["message"] = "success";
                root["data"] = "登录成功";
                std::string token = token_generated();
                std::string pass = pass_generated(username, token);
                std::string bauth = bauth_get(username, pass);
                std::string set = "Access-Control-Expose-Headers: Authorization\n";
                extra_headers = set + bauth;
                this->server->addAuthentication(username, pass, token);
            }
            cb(conn, Json::writeString(Json::StreamWriterBuilder(), root).c_str(),
                extra_headers.c_str());
        });

    POST(this->server, "/register",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            std::string username = root["username"].asString();
            std::string password = root["password"].asString();
            std::string telephone = root["telephone"].asString();
            std::string id_num = root["id_num"].asString();
            std::string crt_time = get_crt_time();
            root.clear();
            ResultSet rs = conn_ptr->query("select * from users where username='" + username + "';");
            if (rs.size() > 0) {
                root["status"] = AUTH_FAILED;
                root["message"] = "fail";
                root["error"] = "用户名已存在";
                Json::StreamWriterBuilder builder;
                cb(conn, Json::writeString(builder, root).c_str(), "");
            } else {
                bool ret = conn_ptr->update(
                    "insert into users(username,password,telephone,id_num,crt_time) "
                    "values('"
                    + username + "','" + password + "','" + telephone + "','" + id_num + "','" + crt_time + "');");
                if (ret) {
                    root["status"] = HTTP_OK;
                    root["message"] = "success";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                } else {
                    root["status"] = HTTP_INTERNAL_SERVER_ERROR;
                    root["message"] = "fail";
                    root["error"] = "服务器错误";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                }
            }
        });

    POST(this->server, "/logout",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            std::string username = root["username"].asString();
            root.clear();
            this->server->removeAuthentication(username);
            root["status"] = HTTP_OK;
            root["message"] = "success";
            Json::StreamWriterBuilder builder;
            cb(conn, Json::writeString(builder, root).c_str(), "");
        });

    POST(this->server, "/updatepwd",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            std::string username = root["username"].asString();
            std::string pld_pwd = root["old_pwd"].asString();
            std::string new_pwd = root["new_pwd"].asString();
            root.clear();
            ResultSet rs = conn_ptr->query("select * from users where username='" + username + "' and password='" + pld_pwd + "';");
            if (rs.size() > 0) {
                bool ret = conn_ptr->update("update users set password='" + new_pwd + "' where username='" + username + "';");
                if (ret) {
                    root["status"] = HTTP_OK;
                    root["message"] = "success";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                } else {
                    root["status"] = HTTP_INTERNAL_SERVER_ERROR;
                    root["message"] = "fail";
                    root["error"] = "服务器错误";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                }
            } else {
                root["status"] = AUTH_FAILED;
                root["message"] = "fail";
                root["error"] = "原密码错误";
                Json::StreamWriterBuilder builder;
                cb(conn, Json::writeString(builder, root).c_str(), "");
            }
        });

    POST(this->server, "/book",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            std::string username = root["username"].asString();
            std::string record_id = root["record_id"].asString();
            std::string prepayment = root["prepayment"].asString();
            std::string crt_time = get_crt_time();
            root.clear();
            Result* r = nullptr;
            ResultSet rs = conn_ptr->query(
                "select booked from houses where record_id='" + record_id + "';");
            r = rs.next();
            if (r->getString("booked") == "1") {
                root["status"] = BOOK_FAILED;
                root["message"] = "fail";
                root["error"] = "该房源已被预定";
                Json::StreamWriterBuilder builder;
                cb(conn, Json::writeString(builder, root).c_str(), "");
            } else {
                bool ret_o = conn_ptr->update(
                    "insert into orders(username,record_id,prepayment,crt_time) "
                    "values('"
                    + username + "','" + record_id + "','" + prepayment + "','" + crt_time + "');");
                bool ret_h = conn_ptr->update(
                    "update houses set booked='1' where record_id='" + record_id + "';");
                if (ret_o && ret_h) {
                    root["status"] = HTTP_OK;
                    root["message"] = "success";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                } else {
                    root["status"] = HTTP_INTERNAL_SERVER_ERROR;
                    root["message"] = "fail";
                    root["error"] = "服务器错误";
                    Json::StreamWriterBuilder builder;
                    cb(conn, Json::writeString(builder, root).c_str(), "");
                }
            }
        });

    POST(this->server, "/get_order",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            std::string username = root["username"].asString();
            root.clear();
            Result* r = nullptr;
            ResultSet rs = conn_ptr->query(
                "select * from orders where username='" + username + "';");
            while ((r = rs.next()) != nullptr) {
                Json::Value order;
                order["id"] = r->getString("id");
                order["record_id"] = r->getString("record_id");
                order["prepayment"] = r->getString("prepayment");
                order["crt_time"] = r->getString("crt_time");
                root["data"].append(order);
            }
            root["status"] = HTTP_OK;
            root["message"] = "success";
            Json::StreamWriterBuilder builder;
            cb(conn, Json::writeString(builder, root).c_str(), "");
        });

    POST(this->server, "/condition",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            std::string body = std::string(hm->body.ptr, hm->body.len);
            Json::Value root;
            strToJson(body, root);
            std::string con_type = root["con_type"].asString();
            root.clear();
            Result* r = nullptr;
            ResultSet rs = conn_ptr->query("select * from codes where belong='" + con_type + "';");
            while ((r = rs.next()) != nullptr) {
                Json::Value code;
                code["id"] = r->getString("id");
                code["name"] = r->getString("type");
                code["info"] = r->getString("info");
                std::string code_id = r->getString("code");
                ResultSet rst = conn_ptr->query(
                    "select * from choices where code =" + code_id + ";");
                while ((r = rst.next()) != nullptr) {
                    code["value"].append(r->getString("choice"));
                }
                root["data"].append(code);
            }
            root["status"] = HTTP_OK;
            root["message"] = "success";
            Json::StreamWriterBuilder builder;
            cb(conn, Json::writeString(builder, root).c_str(), "");
        });

    GET(this->server, "/random",
        [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
            shared_ptr<Connection> conn_ptr = cp->getConnection();
            ResultSet rs = conn_ptr->query("select count(*) as count from houses");
            Result* r = rs.next();
            int count = r->getInt("count");
            std::string sql = "select * from houses where id in(";
            std::random_device rd;
            std::mt19937 mt(rd());
            std::string ret_str;
            std::uniform_int_distribution<int> dis(1, count);
            for (int i = 0; i < 4; i++) {
                sql += std::to_string(dis(mt));
                if (i != 3) {
                    sql += ",";
                }
            }
            sql += ");";
            ResultSet rst = conn_ptr->query(sql);
            Result* rt = nullptr;
            Json::Value root;
            while ((rt = rst.next()) != nullptr) {
                Json::Value json;
                json["id"] = rt->getString("id");
                json["record_id"] = rt->getString("record_id");
                json["area_name"] = rt->getString("area_name");
                json["main_info"] = rt->getString("main_info");
                json["area"] = rt->getString("area") + "平米";
                json["thumb_url"] = this->img_prefix + rt->getString("thumb_url");
                json["total_price"] = rt->getString("total_price") + "万";
                root["data"].append(json);
            }
            root["status"] = HTTP_OK;
            root["message"] = "success";
            Json::StreamWriterBuilder builder;
            cb(conn, Json::writeString(builder, root).c_str(), "");
        });

    POST(this->server, "/admin", [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
        std::string body = std::string(hm->body.ptr, hm->body.len);
        Json::Value root;
        strToJson(body, root);
        std::string username = root["username"].asString();
        std::string password = root["password"].asString();
        root.clear();

        for (std::vector<authentication_info>::iterator it = this->server->v.begin();
             it != this->server->v.end(); it++) {
            if (it->username == username) {
                root.clear();
                root["status"] = AUTH_FAILED;
                root["message"] = "fail";
                root["error"] = "用户已登陆";
                Json::StreamWriterBuilder builder;
                cb(conn, Json::writeString(builder, root).c_str(), "");
                return;
            }
        }
        shared_ptr<Connection> conn_ptr = cp->getConnection();
        Result* r = nullptr;
        ResultSet rs = conn_ptr->query("select * from managers where username='" + username + "' and password='" + password + "';");
        std::string extra_headers = "";
        if ((r = rs.next()) != nullptr) {
            root["status"] = HTTP_OK;
            root["message"] = "success";
            std::string token = token_generated();
            std::string pass = pass_generated(username, token);
            std::string bauth = bauth_get(username, password);
            std::string set = "Access-Control-Expose-Headers: Authorization\n";
            extra_headers = set + bauth;
            this->server->addAuthentication(username, pass, token);
        } else {
            root["status"] = HTTP_OK;
            root["message"] = "fail";
        }
        cb(conn, Json::writeString(Json::StreamWriterBuilder(), root).c_str(), extra_headers.c_str());
    });

    GET(this->server, "/get_info", [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
        std::string username = get_username(hm);
        Json::Value root;
        shared_ptr<Connection> conn_ptr = cp->getConnection();
        if (username != "") {
            ResultSet rs = conn_ptr->query("select * from managers where username='" + username + "'");
            Result* r = nullptr;
            if ((r = rs.next()) != nullptr) {
                root["status"] = HTTP_OK;
                root["message"] = "success";
                Json::Value json;
                json["username"] = username;
                json["tele"] = r->getString("telephone");
                json["crt_time"] = r->getString("create_time");
                root["data"] = json;
            } else {
                root["status"] = HTTP_INTERNAL_SERVER_ERROR;
                root["message"] = "fail";
            }
        } else {
            root["status"] = AUTH_FAILED;
            root["message"] = "fail";
        }
        cb(conn, Json::writeString(Json::StreamWriterBuilder(), root).c_str(), "");
    });

    POST(this->server, "/ihouse", [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
        std::string body = std::string(hm->body.ptr, hm->body.len);
        Json::Value root;
        strToJson(body, root);
        shared_ptr<Connection> conn_ptr = cp->getConnection();
        std::string sql = get_sql(root, conn_ptr);
        root.clear();
        if (conn_ptr->update(sql)) {
            root["status"] = HTTP_OK;
            root["message"] = "success";
        } else {
            root["status"] = HTTP_INTERNAL_SERVER_ERROR;
            root["message"] = "fail";
        }
        cb(conn, Json::writeString(Json::StreamWriterBuilder(), root).c_str(), "");
    });

    POST(this->server, "/icommunity", [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
        std::string body = std::string(hm->body.ptr, hm->body.len);
        Json::Value root;
        strToJson(body, root);
        shared_ptr<Connection> conn_ptr = cp->getConnection();
        std::string sql = get_comm_sql(root, conn_ptr);
        root.clear();
        if (conn_ptr->update(sql)) {
            root["status"] = HTTP_OK;
            root["message"] = "success";
        } else {
            root["status"] = HTTP_INTERNAL_SERVER_ERROR;
            root["message"] = "fail";
        }
        cb(conn, Json::writeString(Json::StreamWriterBuilder(), root).c_str(), "");
    });

    GET(this->server, "/m_logout", [&](mg_http_message* hm, mg_connection* conn, OnRspCallBack cb) {
        Json::Value root;
        std::string username = get_username(hm);
        this->server->removeAuthentication(username);
        root["status"] = HTTP_OK;
        root["message"] = "success";
        Json::StreamWriterBuilder builder;
        cb(conn, Json::writeString(builder, root).c_str(), "");
    });
}

void App::run() { server->start(); }