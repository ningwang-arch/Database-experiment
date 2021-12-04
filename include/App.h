#include "CommonConnectionPool.h"
#include "http_server.h"
#include "tools.h"

class App
{
public:
    App();
    App(std::string addr);
    void init();
    ~App();
    void run();

private:
    std::string addr;
    const std::string img_prefix = "http://127.0.0.1:8000/";
    shared_ptr<HttpServer> server;
    ConnectionPool *cp;
};