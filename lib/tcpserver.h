// ==============================================================================
//                            TCP Server Library
// ==============================================================================
//
// Description: This header provides a simple TCP socket wrapper for creating a
// server using the POSIX socket API. This library is intended for educational
// purposes.
//
// MIT License
//
// Copyright (c) 2024 Erik A. Rapp
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ==============================================================================

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

    // --- Standard Libraries --- //

    #include <stddef.h>     // For NULL definition.
    #include <stdio.h>      // For standard I/O operations, e.g. printf(3), perror(3).
    #include <stdlib.h>     // For memory allocation oeprations, e.g. malloc(3), free(3).
    #include <string.h>     // For string operations, e.g. memset(3), memcpy(3).
    #include <assert.h>     // For debugging, e.g. assert(3).

    // --- POSIX Libraries --- //

    #include <fcntl.h>      // For file control operations, e.g. setting file descriptor flags.
    #include <poll.h>       // For polling file descriptors, e.g. poll(2) and pollfd struct.
    #include <sys/socket.h> // For socket operations, e.g. socket(2).
    #include <netinet/in.h> // For internet address operations, e.g. sockaddr_in struct.
    #include <arpa/inet.h>  // For internet operations, e.g. inet_ntop(3).
    #include <unistd.h>     // For POSIX system calls, e.g. close(2), read(2), write(2).

    // --- Project Libraries --- //

    #include "logging.h" // For logging functions, e.g. LOG_ERROR.

    // --- Constants and Macros --- //

    #define MAX_CLIENTS 1024UL  // Maximum number of clients to connect to the server.
    #define INVALID_FD -1       // Invalid file descriptor value.

    // --- TypeDefs --- //

    /// @brief Structure to store server information.
    /// @note If the file descriptor is -1, the rest of the struct is invalid.
    struct server_info {
        int                 fd;     // File descriptor of the client socket.
        struct sockaddr_in  addr;   // Address of the server socket.
    };

    /// @brief Structure to store client information.
    /// @note If the file descriptor is -1, the rest of the struct is invalid.
    struct client_info {
        int                         fd;         // File descriptor of the client socket.
        struct sockaddr_in          addr;       // Address of the client socket.
        const struct server_info    *listener;  // Server information.
    };

    // --- Function Prototypes --- //

    #ifdef __cplusplus
    extern "C" {
    #endif // __cplusplus

    /// @brief Create a listener socket on the specified port, TCP/IPv4 protocol.
    /// @warning If the function fails, the server should be closed manually using close_server().
    /// @param server The server struct to create.
    /// @param port The port to listen on, 0 for default port.
    /// @return Error code, 0 on success, -1 on failure.
    int server_bind (struct server_info *server, const char* ipv4);

    /// @brief Close the listener socket.
    /// @param server The server struct to close.
    void server_close (struct server_info *server);

    /// @brief Accept a connection from a client for the listener socket.
    /// @param server The server struct to accept the connection on.
    /// @param client The client struct to store the connection information.
    /// @return Error code, 0 on success, -1 on failure.
    int server_accept (const struct server_info *server, struct client_info *client);

    /// @brief Close the client socket.
    /// @param client The client struct to close.
    void client_close (struct client_info *client);

    // --- Function Prototypes, Common --- //

    /// @brief Get the port number from the server struct.
    /// @param server The server struct to get the port from.
    inline unsigned short get_port (const struct server_info *server) {
        return ntohs(server->addr.sin_port);
    }

    /// @brief Get the IP address from the server struct.
    /// @param server The server struct to get the IP address from.
    inline const char* get_addr (const struct server_info *server) {
        return inet_ntoa(server->addr.sin_addr);
    }

    #ifdef __cplusplus
    }
    #endif // __cplusplus

#endif // TCP_SERVER_H_