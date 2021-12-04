#include "App.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

mg_http_serve_opts HttpServer::opts;
std::unordered_map<std::string, url_conf> HttpServer::req_handler_map;
std::vector<authentication_info> HttpServer::v;

const std::string host = "0.0.0.0";

#ifndef PORT
#define PORT 8000
#endif

int main(int argc, char const *argv[]) {
  int port = PORT;
  po::options_description desc("options");
  desc.add_options()("help,h", "help message")(
      "port,p", po::value<int>(&port)->default_value(PORT), "port");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }
  if (vm.count("port")) {
    port = vm["port"].as<int>();
  }
  std::string addr = host + ":" + std::to_string(port);
  App app(addr);
  app.init();
  app.run();
  return 0;
}
