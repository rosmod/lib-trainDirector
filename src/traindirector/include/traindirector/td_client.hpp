#ifndef TD_CLIENT_HPP
#define TD_CLIENT_HPP

#include <string>
#include <sstream>
#include <vector>

#include "ConnectionSubsys.hpp"

class TrainDirClient {
public:
  TrainDirClient();
  ~TrainDirClient();

  bool create_connection(int port, std::string host = "localhost");
  void close_connection();

private:
  Connection conn;
};

#endif
