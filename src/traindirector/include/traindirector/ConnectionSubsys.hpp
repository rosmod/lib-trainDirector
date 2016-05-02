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

// IPV4 and IPV4 Sender & Receiver connection management

// Currently only UDP (SOCK_DGRAM) is fully supported

class Connection {
private:
  int sockfd;
  struct sockaddr* local_addr;
  struct sockaddr* remote_addr;

public:
  bool isServer;
  std::string serverIP;
  int serverPort;
  int receiveTimeout;
  int socketFamily;
  int transport;
  int maxConnRequests;
  
  Connection ()
    : isServer(false),
      serverIP("127.0.0.1"),
      serverPort(80),
      receiveTimeout (1),
      socketFamily(AF_INET),
      transport(SOCK_DGRAM),
      maxConnRequests(5)
  {
  }

  Connection (const Connection &s)
    : isServer (s.isServer),
      serverIP (s.serverIP),
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
    std::swap (serverIP, s.serverIP);
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

  int Initialize(std::string ip, int port, bool server=false) {
    serverIP = ip;
    serverPort = port;
    isServer = server;

    if (serverIP.find("::") != std::string::npos) {
      // ipv6
      socketFamily = AF_INET6;
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
      socketFamily = AF_INET;
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
    if ( socketFamily == AF_INET6 ) {
      // ipv6
      if (inet_pton(socketFamily, serverIP.c_str(), (void *)&(((struct sockaddr_in6 *)local_addr)->sin6_addr.s6_addr)) != 1) {
	int errsv = errno;
	std::string err = "Couldn't convert ipv6 address: " + std::string(strerror(errsv));
	throw err;
      } 
    }
    else if ( socketFamily == AF_INET ) {
      // ipv4
      if (inet_pton(socketFamily, serverIP.c_str(), (void *)&(((struct sockaddr_in *)local_addr)->sin_addr.s_addr)) != 1) {
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
	close(sockfd);
	int errsv = errno;
	std::string err = "Couldn't bind: " + std::string(strerror(errsv));
	throw err;
      }
      if ( transport == SOCK_STREAM ) {
	// tcp
	if (listen(sockfd, maxConnRequests) < 0) {
	  close(sockfd);
	  int errsv = errno;
	  std::string err = "Couldn't listen: " + std::string(strerror(errsv));
	  throw err;
	}
      }
    }
    else {
      // Client
      if ( transport == SOCK_STREAM ) {
	// tcp
	if (connect(sockfd, (struct sockaddr *)local_addr, sizeof(*local_addr))<0) {
	  close(sockfd);
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
      close(sockfd);
      int errsv = errno;
      std::string err = "Couldn't set timeout: " + std::string(strerror(errsv));
      throw err;
    }
  }

  void Close()
  {
    close(sockfd);
    delete local_addr;
    delete remote_addr;
  }

  long Send(const char *buffer, long len) {
    long bytes;
    if ((bytes = sendto(sockfd, buffer, len, 0, (struct sockaddr *) &remote_addr, sizeof(remote_addr))) == -1 ) {
      int errsv = errno;
      std::string err = "Couldn't send: " + std::string(strerror(errsv));
      throw err;
    }
    return bytes;
  }

  long Receive(char *buffer, long len) {
    socklen_t remote_addr_len = sizeof(remote_addr);
    long bytes;
    if ((bytes = recvfrom(sockfd, buffer, len,0,(struct sockaddr *)&remote_addr, &remote_addr_len)) == -1) {
    }
    return bytes;
  }
};
#endif
