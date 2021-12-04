#include "CommonConnectionPool.h"
#include "Connection.h"
#include "Global.h"
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

ConnectionPool *ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool;
    return &pool;
}

bool ConnectionPool::loadConfigFile()
{
    FILE *fp = fopen("mysql.ini", "r");
    if (fp == nullptr)
    {
        LOG(LL_ERROR, ("File 'mysql.ini' is not existing"));
        return false;
    }

    while (!feof(fp))
    {
        char line[1024] = {0};
        fgets(line, 1024, fp);
        string str(line);
        int index = str.find('=', 0);

        if (index == -1)
        {
            continue;
        }

        int endIndex = str.find('\n', index);
        string key = str.substr(0, index);
        string value = str.substr(index + 1, endIndex - index - 1);

        if (key == "ip")
        {
            _ip = value;
        }
        else if (key == "port")
        {
            _port = atoi(value.c_str());
        }
        else if (key == "username")
        {
            _username = value;
        }
        else if (key == "password")
        {
            _password = value;
        }
        else if (key == "dbname")
        {
            _dbname = value;
        }
        else if (key == "maxSize")
        {
            _maxSize = atoi(value.c_str());
        }
        else if (key == "maxIdleTime")
        {
            _maxIdleTime = atoi(value.c_str());
        }
        else if (key == "connectionTimeout")
        {
            _connectionTimeout = atoi(value.c_str());
        }
        else if (key == "initSize")
        {
            _initSize = atoi(value.c_str());
        }
    }
    return true;
}

ConnectionPool::ConnectionPool()
{
    if (!loadConfigFile())
    {
        return;
    }

    for (int i = 0; i < _initSize; i++)
    {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbname);
        p->refreshAliveTime();
        _connQueue.push(p);
        _connCnt++;
    }

    thread produce(std::bind(&ConnectionPool::productConnectionTask, this));
    produce.detach();

    thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
    scanner.detach();
}

void ConnectionPool::productConnectionTask()
{
    for (;;)
    {
        unique_lock<mutex> lock(_queueMutex);
        while (!_connQueue.empty())
        {
            cv.wait(lock);
        }
        if (_connCnt < _maxSize)
        {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            _connQueue.push(p);
            _connCnt++;
        }
        cv.notify_all();
    }
}

shared_ptr<Connection> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(_queueMutex);
    while (_connQueue.empty())
    {
        if (cv_status::timeout == cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeout)))
            if (_connQueue.empty())
            {
                LOG(LL_ERROR, ("Failed to get connection: got idle connection timeout"));
                return nullptr;
            }
    }
    shared_ptr<Connection> sp(_connQueue.front(), [&](Connection *pcon)
                              {
                                  unique_lock<mutex> lock(_queueMutex);
                                  pcon->refreshAliveTime();
                                  _connQueue.push(pcon);
                              });
    _connQueue.pop();
    cv.notify_all();

    return sp;
}

void ConnectionPool::scannerConnectionTask()
{
    for (;;)
    {
        this_thread::sleep_for(chrono::seconds(_maxIdleTime));
        unique_lock<mutex> lock(_queueMutex);

        while (_connCnt > _initSize)
        {
            Connection *p = _connQueue.front();
            if (p->getAliveTime() >= (_maxIdleTime * 1000))
            {
                _connQueue.pop();
                _connCnt--;
                delete p;
            }
            else
            {
                break;
            }
        }
    }
}

ConnectionPool::~ConnectionPool()
{
    while (!_connQueue.empty())
    {
        Connection *p = _connQueue.front();
        _connQueue.pop();
        delete p;
    }
}
