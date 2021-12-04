#pragma once

#include <bits/types/clock_t.h>
#include <string>
#include <mysql/mysql.h>
#include <ctime>
#include "ResultSet.h"
#include "Global.h"

using namespace std;

#ifndef CONNECTION_H
#define CONNECTION_H

class Connection
{
private:
  MYSQL *_conn;
  clock_t _alivetime;

public:
  Connection();

  // connect to database
  bool connect(string ip, unsigned short port, string username, string password, string dbname);

  // insert delete update
  bool update(string sql);

  // select
  ResultSet query(string sql);

  void refreshAliveTime();

  clock_t getAliveTime();

  ~Connection();
};

#endif /* CONNECTION_H */
