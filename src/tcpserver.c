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

#include "tcpserver.h"

// --- Function Definitions --- //

/// @brief Parse the IPv4 address and port from a string and store it in a sockaddr_in->sin_addr.
static int parse_address (const char *ipv4, struct sockaddr_in *saddr) {

    assert(ipv4 && saddr);

    memset(saddr, 0, sizeof(*saddr));

    unsigned int octA, octB, octC, octD, port; // IPv4 address octets and port number.
    int length = 0;

    saddr->sin_family = AF_INET;

    if (sscanf(ipv4, "%u.%u.%u.%u:%u%n", &octA, &octB, &octC, &octD, &port, &length) == 5) {
        saddr->sin_addr.s_addr = htonl((octA << 24) | (octB << 16) | (octC << 8) | octD);
        saddr->sin_port = htons(port);
    }
    else if (sscanf(ipv4, "%u%n", &port, &length) == 1) {
        saddr->sin_addr.s_addr = htonl(INADDR_ANY);
        saddr->sin_port = htons(port);
    }
    else {
        LOG_ERROR("Error parsing address, unexpected format");
        return -1;
    }

    return length;

} // parse_address

/// @brief Create a listener socket on the specified port, TCP/IPv4 protocol.
int server_bind (struct server_info *server, const char* ipv4) {

    assert(server);

    int retval = 0;

    // Create a socket file descriptor for the TCP listener.
    if ((server->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        LOG_ERROR("Error creating server socket");
        retval = -1;
        goto error;
    }

    // Allow the socket to be reused if there is an existing instance of the listener.
    int reuse_addr = 1;

    if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        LOG_ERROR("Error setting server socket options");
        retval = -1;
        goto close_socket;
    }

    // Bind the socket to an address (possibly overwritable) and port.

    struct sockaddr_in server_addr;

    if (parse_address(ipv4, &server_addr) < 0) {
        LOG_ERROR("Error parsing server address");
        retval = -1;
        goto close_socket;
    }

    if (bind(server->fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Error binding server socket");
        retval = -1;
        goto close_socket;
    }

    // Bind was successful, store the server address.
    server->addr = server_addr;

    // Listen on the socket.
    if (listen(server->fd, MAX_CLIENTS) < 0) {
        LOG_ERROR("Error listening on server socket");
        retval = -1;
        goto close_socket;
    }

    return 0;

close_socket: // GOTO: Close the socket and return an error code.
    (void) close(server->fd);
    server->fd = INVALID_FD;

error: // GOTO: Return an error code.
    return retval;

} // open_server

/// @brief Close the listener socket.
void server_close (struct server_info *server) {

    assert(server && server->fd >= 0);

    // If the server socket is not open, there is nothing to close.
    if (close(server->fd) < 0) {
        LOG_ERROR("Error closing server socket");
        return;
    }
    server->fd = INVALID_FD;

} // close_server

/// @brief Accept a connection from a client for the listener socket.
int server_accept (const struct server_info *server, struct client_info *client) {

    assert(server && client);

    // Accept the client connection.
    socklen_t client_addr_len = sizeof(client->addr);

    if ((client->fd = accept(server->fd, (struct sockaddr*) &client->addr, &client_addr_len)) < 0) {
        LOG_ERROR("Error accepting connection");
        return -1;
    }

    // Store the server information in the client struct for reference.
    client->listener = server;

    return 0;

} // accept_connection

/// @brief Close the client socket.
void client_close (struct client_info *client) {

    assert(client && client->fd >= 0);

    // If the client socket is not open, there is nothing to close.
    if (close(client->fd) < 0) {
        LOG_ERROR("Error closing client socket");
        return;
    }
    client->fd = INVALID_FD;

} // close_client
