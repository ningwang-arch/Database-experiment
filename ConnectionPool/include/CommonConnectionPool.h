#pragma once

#include "Connection.h"
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <condition_variable>

using namespace std;

class ConnectionPool
{
public:
	static ConnectionPool *getConnectionPool();
	shared_ptr<Connection> getConnection();

private:
	ConnectionPool();
	bool loadConfigFile();

	void productConnectionTask();

	void scannerConnectionTask();

	~ConnectionPool();

	string _ip;
	unsigned short _port;
	string _username;
	string _password;
	string _dbname;
	int _initSize = 5;
	int _maxSize = 10;
	int _maxIdleTime = 20;
	int _connectionTimeout = 40;

	std::queue<Connection *> _connQueue;

	mutex _queueMutex;

	atomic_int _connCnt;

	condition_variable cv;
};
