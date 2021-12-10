#pragma once
#include "CommonConnectionPool.h"
#include "helpers.h"
#include <cstring>
#include <ctime>
#include <iostream>
#include <json/json.h>
#include <string>
#include <unordered_map>
#include <vector>

const std::string img_prefix = "http://127.0.0.1:8000/";

std::unordered_map<std::string, std::string>
parse_query(const std::string& query_body);

void split(const std::string& str, std::vector<std::string>& tokens,
    const std::string delim = " ");

std::string get_crt_time();

std::string get_like_str(const std::string info, std::string value);

ResultSet get_all_intersection(std::vector<ResultSet> v_rs);

ResultSet get_query_result(shared_ptr<Connection> conn, Json::Value filter,
    std::string table_name);

void req_body_house(ResultSet rs, Json::Value& root);

void req_body_community(ResultSet rs, Json::Value& root);

std::string get_username(mg_http_message* hm);

std::string get_sql(Json::Value& root, shared_ptr<Connection> conn_ptr);

std::string get_identify(std::string info_1, double area, double total, shared_ptr<Connection> conn_ptr);

std::string get_comm_sql(Json::Value& root, shared_ptr<Connection> conn_ptr);