// ==============================================================================
//                          Asynchronous TCP Server
// ==============================================================================
//
// Description: This header provides an asynchronous TCP server using the POSIX
// socket API and the poll() function to monitor file descriptors for events.
// Users can create a server, accept client connections, and poll the server for
// incoming data.
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

#ifndef EVENT_SERVER_H_
#define EVENT_SERVER_H_

    // --- Standard Libraries --- //

    #include <stddef.h>     // For NULL definition and size_t type.
    #include <stdio.h>      // For standard I/O operations, e.g. printf(3), perror(3).
    #include <stdlib.h>     // For memory allocation oeprations, e.g. malloc(3), free(3).
    #include <string.h>     // For string operations, e.g. memset(3), memcpy(3).
    #include <assert.h>     // For debugging, e.g. assert(3).
    #include <stdbool.h>    // For boolean data type.

    // --- Project Libraries --- //

    #include "tcpserver.h"
    #include "htable.h"

    // --- Constants and Macros --- //

    #define BUFFER_SIZE 1024UL

    // --- Type Definitions --- //

    typedef void (*event_callback_t)(void *context, int event, void *data);

    /// @brief Structure to track file descriptors' events.
    struct pollfds {
        struct pollfd   *fds;       // Array of pollfd structs.
        unsigned int    polled;     // Number of file descriptors being polled.
        unsigned int    length;     // Total number of file descriptors.
        int             timeout;    // Timeout for poll(2) in milliseconds.
    };

    /// @brief Structure to store client incoming/outcoming data.
    /// @note The buffer is a circular buffer.
    struct io_buffer {
        char* buffer;       // Pointer to the buffer.
        size_t size;        // Size of the buffer.
        size_t head;        // Offset to write data.
        size_t tail;        // Offset to read data.
    };

    /// @brief Structure to store client context information.
    struct client_context {
        struct client_info  *info;              // Client info.
        struct io_buffer    *input;             // Input buffer.
        struct io_buffer    *output;            // Output buffer.
        event_callback_t    event_handler;      // Event callback function.
        unsigned int        status;             // Connection status, user-defined.
        void                *user_data;         // User data (optional).
    };

    struct server_context {
        struct server_info  info;           // Server information.
        struct pollfds      *polled;        // Pollfds struct to monitor file descriptors.
        htable_t            *contexts;      // Hash table to store client contexts.
        event_callback_t    event_handler;  // Event callback function.
        void                *user_data;     // User data (optional).
    };

    #ifdef __cplusplus
    extern "C" {
    #endif // __cplusplus

    // --- Function Prototypes, pollfd wrapper --- //

    /// @brief Create a pollfds struct for polling.
    /// @param len The length of the pollfd array.
    /// @return Pointer to the allocated pollfds struct.
    struct pollfds *create_pollfds (size_t max_descs);

    /// @brief Destroy the pollfd array.
    /// @param pollfds The pollfds struct to destroy, i.e. deallocate.
    void destroy_pollfds (struct pollfds *pollfds);

    /// @brief Add (or update) a file descriptor event.
    /// @param pollfds The pollfds struct to add the event to.
    /// @param fd The file descriptor to add.
    /// @param events The events to monitor on the file descriptor.
    /// @return 0 on success, -1 on failure.
    int add_event (struct pollfds *pollfds, int fd, short events);

    /// @brief Remove a file descriptor event from the pollfd array.
    /// @note This function does not close the file descriptor.
    /// @param fds The pollfds struct to remove the event from.
    /// @param fd The file descriptor to remove.
    void remove_event (struct pollfds *pollfds, int fd);

    // --- Function Prototypes, iobuffer --- //

    /// @brief Allocate memory for the iobuffer.
    /// @param size Size of the iobuffer.
    /// @return Pointer to the allocated iobuffer.
    struct io_buffer *iobuff_alloc (size_t size);

    /// @brief Free the memory allocated for the buffer.
    /// @param buffer The iobuffer struct to free.
    void iobuff_free (struct io_buffer *buffer);

    /// @brief Append the new data to the iobuffer.
    /// @param buffer The iobuffer struct.
    /// @param data The data to append.
    /// @param length The length of the data to append.
    /// @param can_reallocate Flag to reallocate the iobuffer if full.
    /// @return The number of bytes appended to the iobuffer.
    size_t iobuff_append (struct io_buffer *buffer, const char *data, size_t length, bool can_reallocate);

    /// @brief Send all available data to the client from the iobuffer.
    /// @param client The client context.
    /// @param buffer The iobuffer to send.
    /// @return The number of bytes sent, -1 on failure.
    ssize_t iobuff_send (struct client_context *client, struct io_buffer *buffer);

    // --- Function Prototypes, asynchronnous server --- //

    /// @brief Create a listener socket on the specified port, TCP/IPv4 protocol.
    /// @param port The port to listen on.
    /// @return 0 on success, -1 on failure.
    int as_bind (struct server_context *server, const char* ipv4, event_callback_t handler);

    /// @brief Accept a connection from a client for the listener socket.
    /// @param server The server struct to accept the connection on.
    /// @param handler The event handler for the client connection.
    /// @return Pointer to the client context on success, NULL on failure.
    struct client_context *as_accept (struct server_context *server, event_callback_t handler);

    /// @brief Disconnect the client and free the associated resources.
    /// @param server The server struct.
    /// @param client The client context to disconnect.
    void as_disconnect (struct server_context* server, struct client_context *client);

    /// @brief Main loop to poll file descriptors for events and process connections.
    /// @return 0 on success, -1 on failure.
    int as_poll (struct server_context *server, void* data);

    /// @brief Cleanup function to free resources.
    void as_cleanup (struct server_context *server);

    // --- Function Definitions, pollfd wrapper --- //

    /// @brief Number of file descriptors being polled.
    /// @param pollfds The pollfds struct.
    /// @return The number of file descriptors being polled.
    inline unsigned int polled_fds (const struct pollfds *pollfds) {
        return pollfds->polled;
    }

    /// @brief Total number of file descriptors.
    /// @param pollfds The pollfds struct.
    /// @return The total number of file descriptors.
    inline unsigned int total_fds (const struct pollfds *pollfds) {
        return pollfds->length;
    }

    /// @brief Set the timeout for poll(2) in milliseconds.
    /// @param pollfds The pollfds struct.
    /// @param timeout The timeout in milliseconds.
    inline void set_timeout (struct pollfds *pollfds, int timeout) {
        pollfds->timeout = timeout;
    }

    /// @brief Wrapper for the poll(2) system call.
    /// @param pollfds The pollfds struct to poll.
    /// @return The number of file descriptors with events, or -1 on error.
    inline int poll_events (struct pollfds *pollfds) {
        return poll(pollfds->fds, pollfds->polled, pollfds->timeout);
    }

    /// @brief Check if a polled file descriptor has a specific event flag set.
    /// @param pollfds The pollfds struct.
    /// @param idx The index of the pollfd array.
    /// @param flag The event flag to check, e.g. POLLIN, POLLOUT.
    /// @return 1 if the flag is set, 0 if not.
    inline int check_flag (const struct pollfds *pollfds, size_t idx, short flag) {
        return !!((pollfds->fds[idx].revents & flag) == flag);
    }

    // --- Function Definitions, iobuffer --- //

    /// @brief Check if the rbuffer is empty.
    /// @param buffer The iobuffer struct.
    /// @return 1 if the buffer is empty, 0 otherwise.
    inline int iobuff_empty (const struct io_buffer *buffer) {
        assert(buffer);
        return (buffer->head == buffer->tail);
    }

    /// @brief Check if the buffer is full.
    /// @param buffer The iobuffer struct.
    /// @return 1 if the buffer is full, 0 otherwise.
    inline int iobuff_full (const struct io_buffer *buffer) {
        assert(buffer);
        return ((buffer->head - buffer->tail) == buffer->size);
    }

    /// @brief Get the number of bytes available in the buffer, non-linear.
    /// @param buffer The iobuffer struct.
    /// @return The number of bytes available in the buffer.
    inline int iobuff_space (const struct io_buffer *buffer) {
        assert(buffer);
        return (buffer->head >= buffer->tail) ? buffer->size - (buffer->head - buffer->tail) : buffer->tail - buffer->head;
    }

    /// @brief Get the pointer to the head of the buffer.
    /// @param buffer The iobuffer struct.
    /// @return Pointer to the head of the buffer.
    inline char *iobuff_headptr (const struct io_buffer *buffer) {
        assert(buffer);
        return buffer->buffer + buffer->head;
    }

    /// @brief Get the pointer to the tail of the buffer.
    /// @param buffer The iobuffer struct.
    /// @return Pointer to the tail of the buffer.
    inline char *iobuff_tailptr (const struct io_buffer *buffer) {
        assert(buffer);
        return buffer->buffer + buffer->tail;
    }

    // --- Function Definitions, asynchronnous server --- //

    #ifdef __cplusplus
    }
    #endif // __cplusplus

#endif // EVENT_SERVER_H_