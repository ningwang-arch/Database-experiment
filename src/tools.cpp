#include "tools.h"

std::unordered_map<std::string, std::string>
parse_query(const std::string& query_body)
{
    std::unordered_map<std::string, std::string> body;
    std::vector<std::string> vec;
    split(query_body, vec, "&");
    for (auto& it : vec) {
        auto pos = it.find("=");
        if (pos != std::string::npos) {
            body[it.substr(0, pos)] = it.substr(pos + 1);
        } else {
            break;
        }
    }
    return body;
}

void split(const std::string& str, std::vector<std::string>& tokens,
    const std::string delim)
{
    tokens.clear();

    char* buffer = new char[str.size() + 1];
    std::strcpy(buffer, str.c_str());

    char* tmp;
    char* p = strtok_r(buffer, delim.c_str(), &tmp); // 第一次分割
    do {
        tokens.push_back(p); // 如果 p 为 nullptr，则将整个字符串作为结果
    } while ((p = strtok_r(nullptr, delim.c_str(), &tmp)) != nullptr);
    // strtok_r 为 strtok 的线程安全版本。
}

std::string get_crt_time()
{
    time_t t = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return std::string(buf);
}

std::string get_like_str(const std::string info, std::string value)
{
    std::string preffix = "identify like '";
    std::string middle = "";
    if (info == "total_price") {
        middle = value + "__'";
    } else if (info == "main_info") {
        middle = "_" + value + "_'";
    } else if (info == "area") {
        middle = "__" + value + "'";
    } else if (info == "unit_price") {
        middle = value + "'";
    }

    return preffix + middle;
}

ResultSet get_query_result(shared_ptr<Connection> conn, Json::Value filter,
    std::string table_name)
{
    std::vector<ResultSet> v_rs;
    int num = 0;
    for (auto i : filter) {
        std::string info = i.get("info", "").asString();
        Json::Value value = i.get("value", "");
        std::string str = "select * from " + table_name + " where ";
        for (auto item : value) {
            std::string value_str = item.asString();
            std::string sql = "select description from choices where choice='" + value_str + "'";
            ResultSet result = conn->query(sql);
            Result* r = result.next();
            std::string description = r->getString("description");
            std::string like_str = get_like_str(info, description);
            str = str + like_str + " or ";
        }
        ResultSet rs = conn->query(str.substr(0, str.size() - 4) + ";");
        v_rs.push_back(rs);
    }
    ResultSet rs = get_all_intersection(v_rs);
    return rs;
}

ResultSet get_all_intersection(std::vector<ResultSet> v_rs)
{
    ResultSet rs;
    if (v_rs.size() == 0 || v_rs.size() == 1) {
        return v_rs[0];
    }
    rs = v_rs[0];
    for (int i = 1; i < v_rs.size(); i++) {

        rs.getIntersection(v_rs[i]);
    }
    return rs;
}

void req_body_house(ResultSet rs, Json::Value& root)
{
    Result* ret = nullptr;
    root.clear();
    while ((ret = rs.next()) != nullptr) {
        Json::Value json;
        json["id"] = ret->getInt("id");
        json["thumb"] = img_prefix + ret->getString("thumb_url");
        json["title"] = ret->getString("title");
        json["community"] = ret->getString("community");
        json["total_price"] = ret->getString("total_price") + "万";
        json["unit_price"] = ret->getString("unit_price") + "元/平";
        json["description"] = ret->getString("description");
        json["record_id"] = ret->getString("record_id");
        root["data"].append(json);
    }
}

void req_body_community(ResultSet rs, Json::Value& root)
{
    Result* ret = nullptr;
    root.clear();
    while ((ret = rs.next()) != nullptr) {
        Json::Value json;
        json["id"] = ret->getInt("id");
        json["record_id"] = ret->getString("record_id");
        json["title"] = ret->getString("title");
        json["total"] = ret->getString("total") + "套";
        json["avaliable"] = ret->getString("avaliable") + "套";
        json["unit_price"] = ret->getString("unit_price") + "元/平";
        json["description"] = ret->getString("description");
        json["thumb"] = img_prefix + ret->getString("thumb_url");
        root["data"].append(json);
    }
}

std::string get_username(mg_http_message* hm)
{
    char user[256], pass[256];
    struct mg_str* v = mg_http_get_header(hm, "Authorization");
    user[0] = pass[0] = '\0';
    if (v != NULL && v->len > 6 && memcmp(v->ptr, "Basic ", 6) == 0) {
        char buf[256];
        int n = mg_base64_decode(v->ptr + 6, (int)v->len - 6, buf);
        const char* p = (const char*)memchr(buf, ':', n > 0 ? (size_t)n : 0);
        if (p != NULL) {
            snprintf(user, 256, "%.*s", (int)(p - buf), buf);
            snprintf(pass, 256, "%.*s", n - (int)(p - buf) - 1, p + 1);
            return std::string(user);
        }
    }
    return "";
}

std::string get_sql(Json::Value& root, shared_ptr<Connection> conn_ptr)
{
    /*
main_info: {
  info_1: '',
  info_2: ''
},
direction: '',
area: '',
community: '',
record_id: '',
prepayment: '',
unit_price: '',
desc: ''
*/
    std::string normal_url = "images/normal/default.jpeg";
    std::string thumb_url = "images/thumb/default.jpeg";
    Json::Value main_info = root.get("main_info", "");
    std::string info_1 = main_info.get("info_1", "").asString();
    std::string info_2 = main_info.get("info_2", "").asString();
    std::string info = info_1 + info_2;
    std::string direction = root["direction"].asString();
    double area = std::stod(root["area"].asString());
    area = (double)(((int)(area * 100)) / 100);
    std::string community = root["community"].asString();
    std::string record_id = root["record_id"].asString();
    std::string prepayment = root["prepayment"].asString();
    std::string desc = root["desc"].asString();
    int unit_price = std::stoi(root["unit_price"].asString());
    double total = area * unit_price / 10000;
    std::string identify = get_identify(info_1, area, total, conn_ptr);
    std::string sql = "insert into houses values(NULL,'" + normal_url + "','" + record_id + "','" + info + "','" + direction + "'," + std::to_string(area) + ",'default','default','" + community + "','" + desc + "'," + std::to_string(total) + "," + std::to_string(unit_price) + ",'" + thumb_url + "',0," + prepayment + ",'" + identify + "');";

    return sql;
}

std::string get_identify(std::string info_1, double area, double total, shared_ptr<Connection> conn_ptr)
{
    std::string identify = "";
    ResultSet total_rs = conn_ptr->query("select * from choices where code=1");
    Result* r = nullptr;

    // total_price
    while ((r = total_rs.next()) != nullptr) {
        int begin = r->getInt("begin");
        int end = r->getInt("end");
        if (end != -1) {
            if (total >= begin && total < end) {
                std::string item = r->getString("description");
                identify += item;
                break;
            }
        } else {
            if (total >= begin) {
                std::string item = r->getString("description");
                identify += item;
                break;
            }
        }
    }

    // 房型
    int flag = 0;
    ResultSet info = conn_ptr->query("select * from choices where code =2");
    while ((r = info.next()) != nullptr) {
        std::string item = r->getString("choice");
        if (info_1 == item) {
            flag = 1;
            item = r->getString("description");
            identify += item;
        }
    }
    if (flag == 0) {
        identify += "4";
    }

    // 面积

    ResultSet area_rs = conn_ptr->query("select * from choices where code=3");
    while ((r = area_rs.next()) != nullptr) {
        int begin = r->getInt("begin");
        int end = r->getInt("end");
        if (end != -1) {
            if (area >= begin && area < end) {
                std::string item = r->getString("description");
                identify += item;
                break;
            }
        } else {
            if (area >= begin) {
                std::string item = r->getString("description");
                identify += item;
                break;
            }
        }
    }

    return identify;
}

std::string get_comm_sql(Json::Value& root, shared_ptr<Connection> conn_ptr)
{
    /*
    form: {
        name: '',
        record_id: '',
        unit_price: '',
        total: '',
        desc: ''
      }
    */
    std::string thumb_url = "images/thumb/default.jpeg";
    std::string title = root["name"].asString();
    std::string record_id = root["record_id"].asString();
    std::string unit_price = root["unit_price"].asString();
    std::string total = root["total"].asString();
    std::string desc = root["desc"].asString();
    std::string identify;
    int tmp = std::stoi(unit_price);

    ResultSet rs = conn_ptr->query("select * from choices where code=4");
    Result* r = nullptr;
    while ((r = rs.next()) != nullptr) {
        int begin = r->getInt("begin");
        int end = r->getInt("end");
        if (end != -1) {
            if (tmp >= begin && tmp < end) {
                std::string item = r->getString("description");
                identify = item;
                break;
            }
        } else {
            if (tmp >= begin) {
                std::string item = r->getString("description");
                identify = item;
                break;
            }
        }
    }

    std::string sql = "insert into communities values(NULL,'" + record_id + "','" + thumb_url + "','" + title + "'," + total + "," + total + "," + unit_price + ",'" + desc + "','" + identify + "');";
    return sql;
}