#include "tools.h"

std::unordered_map<std::string, std::string>
parse_query(const std::string &query_body) {
  std::unordered_map<std::string, std::string> body;
  std::vector<std::string> vec;
  split(query_body, vec, "&");
  for (auto &it : vec) {
    auto pos = it.find("=");
    if (pos != std::string::npos) {
      body[it.substr(0, pos)] = it.substr(pos + 1);
    } else {
      break;
    }
  }
  return body;
}

void split(const std::string &str, std::vector<std::string> &tokens,
           const std::string delim) {
  tokens.clear();

  char *buffer = new char[str.size() + 1];
  std::strcpy(buffer, str.c_str());

  char *tmp;
  char *p = strtok_r(buffer, delim.c_str(), &tmp); // 第一次分割
  do {
    tokens.push_back(p); // 如果 p 为 nullptr，则将整个字符串作为结果
  } while ((p = strtok_r(nullptr, delim.c_str(), &tmp)) != nullptr);
  // strtok_r 为 strtok 的线程安全版本。
}

std::string get_crt_time() {
  time_t t = time(nullptr);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
  return std::string(buf);
}

std::string get_like_str(const std::string info, std::string value) {
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
                           std::string table_name) {
  std::vector<ResultSet> v_rs;
  int num = 0;
  for (auto i : filter) {
    std::string info = i.get("info", "").asString();
    Json::Value value = i.get("value", "");
    std::string str = "select * from " + table_name + " where ";
    for (auto item : value) {
      std::string value_str = item.asString();
      std::string sql =
          "select description from choices where choice='" + value_str + "'";
      ResultSet result = conn->query(sql);
      Result *r = result.next();
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

ResultSet get_all_intersection(std::vector<ResultSet> v_rs) {
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

void req_body_house(ResultSet rs, Json::Value &root) {
  Result *ret = nullptr;
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

void req_body_community(ResultSet rs, Json::Value &root) {
  Result *ret = nullptr;
  root.clear();
  while ((ret = rs.next()) != nullptr) {
    Json::Value json;
    json["id"] = ret->getInt("id");
    json["record_id"] = ret->getString("record_id");
    json["title"] = ret->getString("title");
    json["total"] = ret->getString("total");
    json["avaliable"] = ret->getString("avaliable");
    json["unit_price"] = ret->getString("unit_price") + "元/平";
    json["description"] = ret->getString("description");
    json["thumb"] = img_prefix + ret->getString("thumb_url");
    root["data"].append(json);
  }
}