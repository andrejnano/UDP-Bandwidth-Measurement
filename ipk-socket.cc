/**
 *  @file       ipk-socket.cc
 *  @author     Andrej Nano (xnanoa00)
 *  @date       2018-04-09
 *  @version    1.0
 * 
 *  @brief IPK 2018, 2nd project - Bandwidth Measurement (Ryšavý). Socket wrapper implementation.
 *  
 *  @section Description
 *  
 *  This file implements a socket wrapper class for easier and more understandable usage of 
 *  the socket API.
 */

// std libraries
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdint.h>
#include <cstring>

// commonly used std objects
using std::cout;
using std::cerr;
using std::endl;
using std::string;

// socket libraries
#include <sys/socket.h> // Core socket functions and data structures.
#include <sys/types.h> 
#include <netinet/in.h> // AF_INET and AF_INET6
#include <arpa/inet.h>  // Functions for manipulating numeric IP addresses.
#include <netdb.h>      // DNS lookup

// socket wrapper declarations
#include "ipk-socket.h"
#include "ipk-mtrip.h"


/**
 * @brief Creates new Socket Entity for specified file descriptor
 * 
 * @param fd file descriptor for the socket attached
 */
SocketEntity::SocketEntity()
{
  local_length  = sizeof(local);
  remote_length = sizeof(remote);

  if( (socket_fd = socket(AF_INET, SOCK_DGRAM, 0) ) <= 0 )
  {
    cerr << "socket creation failed" << endl;
    return;
  }

}

/**
 * @brief Returns the file descriptor for this socket
 */
int SocketEntity::get_fd()
{
  return socket_fd;
}


/**
 * @brief Prepares the correct address format and binds the socket
 * 
 * @param port port number to be opened
 * @return exit code
 */
int SocketEntity::setup_server(unsigned short port)
{

  int optval = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const void *>(&optval) , sizeof(int));

  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  local.sin_port = htons(port);

  if ( bind(socket_fd, reinterpret_cast<struct sockaddr*>(&local), sizeof(local)) < 0)
  {
    close(socket_fd);
    cerr << "socket binding failed" << endl;
    return -1;
  }

  return EXIT_SUCCESS;
}


/**
 * @brief Prepares address of the foreign host.
 * 
 * @param hostname name of the remote host
 * @param port port number of the remote host
 * @return exit code
 */
int SocketEntity::setup_connection(const char* hostname, unsigned short port)
{
  // DNS find the host by name, return IP
  hp = gethostbyname(hostname); 
  if (!hp)
  {
    std::cerr << "[ERROR]: No such host as " << hostname << std::endl;
    return -1;
  }

  //setup socket address
  remote.sin_family = AF_INET;
  std::memcpy(&remote.sin_addr, hp->h_addr, hp->h_length);
  remote.sin_port = htons(port);

  // connect to udp remote socket -> then only use send instead of sendto..
  if ( connect(socket_fd, reinterpret_cast<sockaddr*>(&remote), remote_length) == -1 )
  {
    close_socket();
    cerr << "connect() error inside setup_connection method" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}


/**
 * @brief Sends a message and returns the number of bytes sent
 * 
 * @param buffer pointer to the data to send
 * @param buf_size size being sent
 * @return number of bytes sent
 */
ssize_t SocketEntity::send_message(char* buffer, size_t buf_size)
{
  return send(socket_fd, buffer, buf_size, 0);
}


/**
 * @brief Receives a message and returns the number of bytes received
 * 
 * @param buffer pointer to the data to send
 * @param buf_size size being received
 * @param save_connection option to add the host from which msg was received
 *        as a 'remote' and further just use simple send without passing addr.
 *        every time
 * @return number of bytes received
 */
ssize_t SocketEntity::recv_message(char* buffer, size_t buf_size, bool save_connection)
{
  if (save_connection)
  {
    // clear existing remote address 
    remote.sin_family = AF_UNSPEC;
    connect(socket_fd, reinterpret_cast<sockaddr*>(&remote), remote_length);
    memset(&remote, 0, sizeof(remote));
    
    int bytes_received = recvfrom(socket_fd, buffer, buf_size, 0, reinterpret_cast<sockaddr*>(&remote), &remote_length);

    // connect to udp remote socket -> then only use send instead of sendto..
    if ( connect(socket_fd, reinterpret_cast<sockaddr*>(&remote), remote_length) == -1 )
    {
      cerr << "connect() error inside setup_connection method" << endl;
    }

    return bytes_received;
  }
  else
  {
    return recv(socket_fd, buffer, buf_size, 0);
  }
}