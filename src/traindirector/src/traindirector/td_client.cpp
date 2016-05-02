#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>

#include "traindirector/td_client.hpp"

bool
TrainDirClient::create_connection(int port, std::string host)
{
  try {
    conn.Initialize(host, port);
    return true;
  }
  catch (std::string &ex) {
    std::cout << ex;
    return false;
  }
}

void
TrainDirClient::close_connection()
{
  conn.Close();
}

void 
TrainDirClient::click(int x, int y)
{
  std::string command = "click " + 
    std::to_string(x) + ' ' + 
    std::to_string(y) + '\n';
  conn.Send(command.c_str(), command.length());
}

void
TrainDirClient::run()
{
  std::string command = "run\n";
  conn.Send(command.c_str(), command.length());
}
