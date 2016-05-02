#ifndef TD_CLIENT_HPP
#define TD_CLIENT_HPP

#include <string>
#include <sstream>
#include <vector>

#include "Connection.hpp"

class TrainDirClient {
public:
  TrainDirClient() {}
  ~TrainDirClient() {}

  bool create_connection(int port = 8900, std::string host = "127.0.0.1");
  void close_connection();

  void click(int x, int y);
  void run();

private:
  void sendCommand(std::string command);

  Connection conn;
};

#endif
