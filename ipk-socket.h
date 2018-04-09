/**
 *  @file       ipk-socket.h
 *  @author     Andrej Nano (xnanoa00)
 *  @date       2018-04-09
 *  @version    1.0
 * 
 *  @brief IPK 2018, 2nd project - Bandwidth Measurement (Ryšavý). Socket object.
 */

#ifndef IPK_SOCKET_H_
#define IPK_SOCKET_H_

    #include <sys/socket.h> // Core socket functions and data structures.
    #include <sys/types.h> 
    #include <netinet/in.h>
    
    /**
     * @brief Socket data & operations wrapper
     * 
     * @desc Encapsulates all important socket data and operations
     * 
     * **This implementation uses UTP protocol as the transport protocol.**
     * 
     * Inspired by Socket class described in: "Designing and Implementing an 
     * Application Layer Network Protocol Using UNIX Sockets and TCP" by 
     * 'Ryan Lattrel', ECE 480 Design Team 2, 11/16/2012
     * [source](https://www.egr.msu.edu/classes/ece480/capstone/fall12/group02/documents/Ryan-Lattrel_App-Note.pdf)
     */  
    class SocketEntity
    {
        private:
            int socket_fd;
            struct sockaddr_in local;  // from server view -> its address
            struct sockaddr_in remote; // from clients view -> server address, from server view -> client address
            socklen_t local_length;
            socklen_t remote_length;
            hostent* hp;

        public:
            SocketEntity();
            
            inline ~SocketEntity() { close_socket(); }

            // returns socket file descriptor
            int get_fd();

            // closes the socket
            inline void close_socket() { close(socket_fd); }

            // interface for communication w/ other socket
            ssize_t send_message(char* buffer, size_t buf_size);
            ssize_t recv_message(char* buffer, size_t buf_size, bool save_connection = false);

            // setup and bind a server on this host:port
            int setup_server(unsigned short port);
            
            // prepare address 
            int setup_connection(const char* hostname, unsigned short port);
    };

#endif // IPK_SOCKET_H_