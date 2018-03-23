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
  }
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

  // 1. UDP server.. needs to bind and use this kind of address struct
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
  hp = gethostbyname(hostname); // DNS find the host by name, return IP
  if (!hp)
  {
    std::cerr << "[ERROR]: No such host as " << hostname << std::endl;
    return -1;
  }

  //setup socket address
  remote.sin_family = AF_INET;
  std::memcpy(&remote.sin_addr, hp->h_addr, hp->h_length);
  remote.sin_port = htons(port);

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
  return sendto(socket_fd, buffer, buf_size, 0, reinterpret_cast<sockaddr*>(&remote), remote_length);
}


/**
 * @brief Receives a message and returns the number of bytes received
 * 
 * @param buffer pointer to the data to send
 * @param buf_size size being received
 * @return number of bytes received
 */
ssize_t SocketEntity::recv_message(char* buffer, size_t buf_size)
{
  return recvfrom(socket_fd, buffer, buf_size, 0, reinterpret_cast<sockaddr*>(&remote), &remote_length);
}
