#include "ResultSet.h"
#include <unordered_map>

Result::Result(MYSQL_RES* result, MYSQL_ROW row)
{
    int num_fields = mysql_num_fields(result);
    MYSQL_FIELD* field;
    for (int i = 0; i < num_fields; i++) {
        field = mysql_fetch_field_direct(result, i);
        um.insert(pair<string, string>(field->name, row[i]));
    }
}

Result::Result() { }

bool Result::equals(const Result* r)
{
    if (um.size() != r->um.size())
        return false;
    unordered_map<string, string>::const_iterator it = um.begin();
    while (it != um.end()) {
        unordered_map<string, string>::const_iterator it2 = r->um.find(it->first);
        if (it2 != r->um.end()) {
            if (it2->second != it->second)
                return false;
        } else {
            return false;
        }
        it++;
    }
    return true;
}

Result::Result(const Result* r)
{
    for (unordered_map<string, string>::const_iterator it = r->um.begin();
         it != r->um.end(); it++) {
        um.insert(pair<string, string>(it->first, it->second));
    }
}

string Result::getValue(const string key)
{
    if (um.find(key) == um.end()) {
        string msg = "key '" + key + "' not found";
        LOG(LL_INFO, (msg.c_str()));
        return "";
    }
    return um.find(key)->second;
}

int Result::getInt(const string key)
{
    if (getValue(key) == "") {
        return NO_SUCH_KEY;
    }
    return stoi(getValue(key));
}

double Result::getDouble(const string key)
{
    if (getValue(key) == "") {
        return NO_SUCH_KEY;
    }
    return stod(getValue(key));
}

string Result::getString(const string key) { return getValue(key); }

bool Result::getBool(const string key)
{
    int value = getInt(key);
    if (value == NO_SUCH_KEY) {
        return false;
    }
    return value == 1;
}

Result::~Result() { um.clear(); }

ResultSet::ResultSet(MYSQL_RES* result)
{
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Result* r = new Result(result, row);
        v.push_back(r);
    }
    it = v.begin();
}

ResultSet::ResultSet() { it = v.begin(); }

ResultSet::~ResultSet()
{
    for (it = v.begin(); it != v.end(); it++) {
        delete *it;
    }
    v.clear();
}

Result* ResultSet::next()
{
    if (it == v.end()) {
        return nullptr;
    }
    return *it++;
}
int ResultSet::size() { return v.size(); }

void ResultSet::reset() { it = v.begin(); }

bool ResultSet::isElement(Result* r)
{
    for (it = v.begin(); it != v.end(); it++) {
        if (r->equals(*it)) {
            return true;
        }
    }
    return false;
}

void ResultSet::getIntersection(ResultSet& rs)
{
    ResultSet result;
    result.v.clear();
    result.it = result.v.begin();
    Result* r = nullptr;
    while ((r = rs.next()) != nullptr) {
        if (this->isElement(r)) {
            result.insert(r);
        }
    }

    this->v.clear();
    this->it = this->v.begin();
    for (auto it = result.v.begin(); it != result.v.end(); it++) {
        this->insert(*it);
    }
    result.v.clear();
    result.it = result.v.begin();
}

void ResultSet::insert(Result* r)
{
    Result* r2 = new Result(r);
    v.push_back(r2);
}

ResultSet ResultSet::offset(int start, int cnt)
{
    int size = v.size();
    ResultSet result;
    result.v.clear();
    result.it = result.v.begin();

    if (start >= size) {
        return result;
    }
    if (start + cnt > size) {
        cnt = size - start;
    }
    for (int i = start; i < start + cnt; i++) {
        result.insert(v[i]);
    }
    return result;
}

ResultSet::ResultSet(const ResultSet& rs)
{
    for (it = rs.v.begin(); it != rs.v.end(); it++) {
        Result* r = new Result(*it);
        v.push_back(r);
    }
    it = v.begin();
}

ResultSet& ResultSet::operator=(const ResultSet& rs)
{
    for (it = v.begin(); it != v.end(); it++) {
        delete *it;
    }
    for (it = rs.v.begin(); it != rs.v.end(); it++) {
        Result* r = new Result(*it);
        v.push_back(r);
    }
    it = v.begin();
    return *this;
}