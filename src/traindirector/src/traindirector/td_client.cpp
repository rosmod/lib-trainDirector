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
    conn.setTransport(Connection::TCP);
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
TrainDirClient::sendCommand(std::string command)
{
  try {
    conn.Send(command.c_str(), strlen(command.c_str()));
  }
  catch (std::string& err) {
    std::cout << err << std::endl;
  }
}

void 
TrainDirClient::click(int x, int y)
{
  std::string command = "click " + 
    std::to_string(x) + ' ' + 
    std::to_string(y) + '\n';
  sendCommand(command);
}

void
TrainDirClient::run()
{
  std::string command = "run\n";
  sendCommand(command);
}
