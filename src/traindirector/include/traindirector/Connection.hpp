#ifndef CONNECTION_SUBSYS_HPP
#define CONNECTION_SUBSYS_HPP

#include <sys/ioctl.h>
#include <linux/sockios.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#include <cstring>
#include <algorithm>
#include <memory>
#include <string>

// IPV4 and IPV6 Sender & Receiver connection management

// Currently only UDP (SOCK_DGRAM) is fully supported

class Connection {

public:
  enum { UDP=SOCK_DGRAM, TCP=SOCK_STREAM };
  enum { IPv4=AF_INET, IPv6=AF_INET6 };

private:
  bool initialized;
  int sockfd;
  struct sockaddr* local_addr;
  struct sockaddr* remote_addr;

public:
  bool isServer;
  std::string serverIp;
  int serverPort;
  int receiveTimeout;
  int socketFamily;
  int transport;
  int maxConnRequests;
  
  Connection ()
    : initialized(false),
      isServer(false),
      serverIp("127.0.0.1"),
      serverPort(80),
      receiveTimeout (1),
      socketFamily(Connection::IPv4),
      transport(Connection::UDP),
      maxConnRequests(5)
  {
  }

  Connection (const Connection &s)
    : initialized(false),
      isServer (s.isServer),
      serverIp (s.serverIp),
      serverPort (s.serverPort),
      receiveTimeout (s.receiveTimeout),
      socketFamily(s.socketFamily),
      transport(s.transport),
      maxConnRequests(s.maxConnRequests)
  {
  }

  ~Connection() {
    Close();
  }

  Connection & operator= (const Connection &s) 
  {
    if (&s != this)
      {
	Connection tmp(s);
	swap (tmp);
      }
    return *this;
  }

  void swap (Connection &s) 
  {
    std::swap (isServer, s.isServer);
    std::swap (serverIp, s.serverIp);
    std::swap (serverPort, s.serverPort);
    std::swap (receiveTimeout, s.receiveTimeout);
    std::swap (socketFamily, s.socketFamily);
    std::swap (transport, s.transport);
    std::swap (maxConnRequests, s.maxConnRequests);
  }

  Connection* clone() const 
  {
    return new Connection( *this );
  }

  void setFamily(int f) {
    if (initialized)
      throw std::string("Cannot change socket family while connection is open!");
    switch (f) {
    case Connection::IPv4:
    case Connection::IPv6:
      socketFamily = f;
      break;
    default:
      throw std::string("Socket family not supported!");
    }
  }

  void setTransport(int t) {
    if (initialized)
      throw std::string("Cannot change transport while connection is open!");
    switch (t) {
    case Connection::UDP:
    case Connection::TCP:
      transport = t;
      break;
    default:
      throw std::string("Transport type not supported!");
    }
  }

  int Initialize(std::string ip, int port, bool server=false) {
    if (initialized)
      throw std::string("Connection has already been initialized!");

    serverIp = ip;
    serverPort = port;
    isServer = server;

    if (serverIp.find("::") != std::string::npos) {
      // ipv6
      socketFamily = Connection::IPv6;
      local_addr = (struct sockaddr *)new struct sockaddr_in6;
      ((struct sockaddr_in6 *)local_addr)->sin6_family = socketFamily;
      ((struct sockaddr_in6 *)local_addr)->sin6_port = htons(serverPort);
      ((struct sockaddr_in6 *)local_addr)->sin6_scope_id = 0;
      remote_addr = (struct sockaddr *)new struct sockaddr_in6;
      ((struct sockaddr_in6 *)remote_addr)->sin6_family = socketFamily;
      ((struct sockaddr_in6 *)remote_addr)->sin6_port = htons(serverPort);
      ((struct sockaddr_in6 *)remote_addr)->sin6_scope_id = 0;
    }
    else {
      // ipv4
      socketFamily = Connection::IPv4;
      local_addr = (struct sockaddr *)new struct sockaddr_in;
      ((struct sockaddr_in *)local_addr)->sin_family = socketFamily;
      ((struct sockaddr_in *)local_addr)->sin_port = htons(serverPort);
      remote_addr = (struct sockaddr *)new struct sockaddr_in;
      ((struct sockaddr_in *)remote_addr)->sin_family = socketFamily;
      ((struct sockaddr_in *)remote_addr)->sin_port = htons(serverPort);
    }

    // create the socket
    if ((sockfd = socket(socketFamily, transport, 0)) < 0) {
      int errsv = errno;
      std::string err = "Couldn't open socket: " + std::string(strerror(errsv));
      throw err;
    }

    // allow address reuse
    int optval = 1;
    int retval = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(optval)) != 0) {
      int errsv = errno;
      std::string err = "Couldn't set REUSEADDR option: " + std::string(strerror(errsv));
      throw err;
    }
    
    // convert to binary
    if ( socketFamily == Connection::IPv6 ) {
      // ipv6
      if (inet_pton(socketFamily, serverIp.c_str(), (void *)&(((struct sockaddr_in6 *)local_addr)->sin6_addr.s6_addr)) != 1) {
	int errsv = errno;
	std::string err = "Couldn't convert ipv6 address: " + std::string(strerror(errsv));
	throw err;
      } 
    }
    else if ( socketFamily == Connection::IPv4 ) {
      // ipv4
      if (inet_pton(socketFamily, serverIp.c_str(), (void *)&(((struct sockaddr_in *)local_addr)->sin_addr.s_addr)) != 1) {
	int errsv = errno;
	std::string err = "Couldn't convert ipv4 address: " + std::string(strerror(errsv));
	throw err;
      }
    }

    // Configure the socket
    if (isServer) {
      // Server
      //   -- both tcp and udp
      if (bind(sockfd,(struct sockaddr *)local_addr, sizeof(*local_addr))<0) {
	Close();
	int errsv = errno;
	std::string err = "Couldn't bind: " + std::string(strerror(errsv));
	throw err;
      }
      if ( transport == Connection::TCP ) {
	// tcp
	if (listen(sockfd, maxConnRequests) < 0) {
	  Close();
	  int errsv = errno;
	  std::string err = "Couldn't listen: " + std::string(strerror(errsv));
	  throw err;
	}
      }
    }
    else {
      // Client
      if ( transport == Connection::TCP ) {
	// tcp
	if (connect(sockfd, (struct sockaddr *)local_addr, sizeof(*local_addr))<0) {
	  Close();
	  int errsv = errno;
	  std::string err = "Couldn't bind: " + std::string(strerror(errsv));
	  throw err;
	}
      }
    }
    
    // set the recv timeout on the socket
    struct timeval tv;
    tv.tv_sec = receiveTimeout;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      Close();
      int errsv = errno;
      std::string err = "Couldn't set timeout: " + std::string(strerror(errsv));
      throw err;
    }
    initialized = true;
  }

  void Close()
  {
    close(sockfd);
    if (local_addr)
      delete local_addr;
    if (remote_addr)
      delete remote_addr;
    initialized = false;
  }

  long Send(const char *buffer, long len) {
    if ( transport == Connection::UDP ) {
      return sendUDP(buffer, len);
    }
    else if ( transport == Connection::TCP ) {
      return sendTCP(buffer, len);
    }
  }

  long Receive(char *buffer, long len) {
    if ( transport == Connection::UDP ) {
      return receiveUDP(buffer, len);
    }
    else if ( transport == Connection::TCP ) {
      return receiveTCP(buffer, len);
    }
  }

private:
  
  long sendUDP(const char*buffer, long len) {
    long bytes;
    if ((bytes = sendto(sockfd, buffer, len, 0, remote_addr, sizeof(*remote_addr))) == -1 ) {
      int errsv = errno;
      std::string err = "Couldn't send: " + std::string(strerror(errsv));
      throw err;
    }
    return bytes;
  }
  
  long sendTCP(const char*buffer, long len) {
    long bytes;
    if ((bytes = send(sockfd, buffer, len, 0)) != len ) {
      int errsv = errno;
      std::string err = "Couldn't send: " + std::string(strerror(errsv));
      throw err;
    }
    return bytes;
  }

  long receiveUDP(char *buffer, long len) {
    socklen_t remote_addr_len = sizeof(*remote_addr);
    long bytes;
    if ((bytes = recvfrom(sockfd, buffer, len, 0, remote_addr, &remote_addr_len)) == -1) {
    }
    return bytes;
  }

  long receiveTCP(char *buffer, long len) {
    long bytes;
    if ((bytes = recv(sockfd, buffer, len, 0)) == -1) {
    }
    return bytes;
  }
};
#endif
