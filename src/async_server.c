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

#include "async_server.h"

// --- Static function definitions --- //

/// @brief Hash function for the client file descriptor.
/// @note Since the client file descriptor is unique, the hash value is the key itself.
/// @param key (int *) The client file descriptor.
/// @return The hash value for the key.
static unsigned long client_hash (const void *key) {
    // Typecast the key to an integer value.
    const int value = *((int *) key);

    return (unsigned long)(value - 4U);
}

/// @brief Comparison function for the client file descriptor.
/// @param key1 (int *) The first client file descriptor.
/// @param key2 (int *) The second client file descriptor.
/// @return The difference between the two keys.
static int client_compare (const void *key1, const void *key2) {
    // Typecast the keys to integer values.
    const int valueA = *((int *) key1);
    const int valueB = *((int *) key2);

    return (valueA - valueB);
}

/// @brief Round up the size to the next power of two.
/// @param size The size to round up.
/// @return The next power of two size.
static size_t size_roundup (size_t size) {

    size_t new_size = size;

    // Avoid rounding up if the size is already a power of two, since it would result in a 2^(n+1) size.
    if (new_size & (new_size - 1)) {
        new_size = new_size | (new_size >> 1);
        new_size = new_size | (new_size >> 2);
        new_size = new_size | (new_size >> 4);
        new_size = new_size | (new_size >> 8);
        new_size = new_size | (new_size >> 16);
        new_size = new_size | (new_size >> 32); // 32-bit shift for 64-bit systems.
        new_size++;
    }

    return new_size;
}

/// @brief Get the minimum of two values.
/// @param a The first value.
/// @param b The second value.
/// @return The minimum of the two values.
static inline size_t min (size_t a, size_t b) {
    return (a < b) ? a : b;
}

/// @brief Align the pointer to the specified alignment to avoid misalignment issues.
/// @param ptr The pointer to align.
/// @param bytes The number of bytes to skip (if any).
/// @param alignment The alignment value, i.e. _Alignof() value.
/// @return The aligned pointer.
static void *align_ptr (void *ptr, size_t bytes, size_t alignment) {

    char *ptr_char = (char *) ptr;

    // Calculate the raw address of the pointer and the misalignment.
    uintptr_t raw_address = (uintptr_t) (ptr_char + bytes);
    uintptr_t misalignment = raw_address & (alignment - 1);

    // Adjust the pointer to the next aligned address.
    if (misalignment != 0) {
        size_t padding = alignment - misalignment;
        raw_address += padding;
    }

    return (void *) raw_address;
}

/// @brief Allocate memory for the client context and associated structures.
/// @note This function allocates fixed-size memory for the buffers defined by the BUFFER_SIZE macro.
/// @param client The client context to allocate memory for.
/// @return Pointer to the allocated client context.
static struct client_context *client_alloc (void) {

    // Allocate memory for the client context in a single memory chunk.
    const size_t mem_size = sizeof(struct client_context) + sizeof(struct client_info) + sizeof(struct io_buffer) * 2;
    void* mem_chunk = NULL;

    if ((mem_chunk = calloc(mem_size, 1U)) == NULL) {
        LOG_ERROR("Error allocating memory for client context");
        return NULL;
    }

    // Partition the memory chunk into the client context and associated structures.
    struct client_context *client = (struct client_context *) mem_chunk;
    mem_chunk = align_ptr(mem_chunk, sizeof(struct client_context), _Alignof(struct client_context));

    client->info = (struct client_info *) mem_chunk;
    mem_chunk = align_ptr(mem_chunk, sizeof(struct client_info), _Alignof(struct client_info));

    client->input = (struct io_buffer *) mem_chunk;
    mem_chunk = align_ptr(mem_chunk, sizeof(struct io_buffer), _Alignof(struct io_buffer));

    client->output = (struct io_buffer *) mem_chunk;

    return client;
}

// --- Function definitions, pollfd wrapper --- //

/// @brief Create a pollfd array of the specified size.
struct pollfds *create_pollfds (size_t max_descs) {

    assert(max_descs > 0);

    // Allocate memory for the pollfds struct.
    struct pollfds *pollfds = malloc(sizeof(*pollfds));

    if (pollfds == NULL) {
        LOG_ERROR("Error creating pollfds struct: memory allocation failed");
        return NULL;
    }

    // Allocate memory for the pollfd array.
    pollfds->fds = calloc(max_descs, sizeof(*pollfds->fds));

    if (pollfds->fds == NULL) {
        LOG_ERROR("Error creating pollfd array: memory allocation failed");
        free(pollfds);
        return NULL;
    }

    // Initialize the pollfd array with invalid file descriptors.
    for (unsigned int idx = 0; idx < max_descs; idx++) {
        pollfds->fds[idx].fd = -1;
    }

    // Initialize the remaining fields of the pollfds struct.
    pollfds->polled = 0;
    pollfds->length = max_descs;
    pollfds->timeout = -1; // Default to blocking mode, no timeout.

    return pollfds;
}

/// @brief Destroy the pollfd array.
void destroy_pollfds (struct pollfds *pollfds) {

    assert(pollfds);

    if (pollfds->fds != NULL) {
        free(pollfds->fds);
    }

    free(pollfds);
}

/// @brief Add (or update) a file descriptor event.
int add_event (struct pollfds *pollfds, int fd, short events) {

    assert(pollfds && pollfds->fds && fd >= 0);

    if (pollfds->polled == pollfds->length) {
        LOG_ERROR("Error adding pollfd event: buffer overflow");
        return -1;
    }

    // Check whether the file descriptor is already being polled.
    unsigned int desc_idx = 0;

    for (; desc_idx < pollfds->polled; desc_idx++) {
        if (pollfds->fds[desc_idx].fd == fd) {

            // Update the events being monitored.
            pollfds->fds[desc_idx].events = events;
            pollfds->fds[desc_idx].revents = 0;
            
            return 0;
        }
    }

    struct pollfd new_fd = {
        .fd = fd,
        .events = events,
        .revents = 0
    };

    // Add the file descriptor and events to the pollfd array.
    pollfds->fds[pollfds->polled++] = new_fd;

    return 0;
}

/// @brief Remove a file descriptor event.
void remove_event (struct pollfds *pollfds, int fd) {

    assert(pollfds && pollfds->fds && fd >= 0);

    if (pollfds->polled == 0) {
        return;
    }

    // Find the file descriptor in the pollfd array.
    unsigned int desc_idx = 0;

    for (; desc_idx < pollfds->polled; desc_idx++) {
        if (pollfds->fds[desc_idx].fd == fd) {
            break;
        }
    }

    // Check if the file descriptor was found.
    if (desc_idx == pollfds->polled) {
        return;
    }

    struct pollfd new_fd = {
        .fd = -1,
        .events = 0,
        .revents = 0
    };

    // Clear the file descriptor and events.
    pollfds->fds[desc_idx] = new_fd;

    // Pack the pollfd array.
    for (unsigned int idx = desc_idx; idx < pollfds->polled; idx++) {
        if (pollfds->fds[idx].fd < 0) {
            continue;
        }
        pollfds->fds[desc_idx++] = pollfds->fds[idx];
        pollfds->fds[idx].fd = -1;
    }

    pollfds->polled--;
}

// --- Function definitions, iobuffer wrappers --- //

/// @brief Allocate memory for the client context and associated structures.
struct io_buffer *iobuff_alloc (size_t size) {

    assert((size & (size - 1)) == 0);
    
    const size_t alloc_size = sizeof(struct io_buffer) + size_roundup(size * sizeof(char));

    struct io_buffer *buffer = NULL;

    if ((buffer = calloc(1U, alloc_size)) == NULL) {
        LOG_ERROR("Error allocating memory for buffer");
        return NULL;
    }

    // char has an alignment of 1, so no need to align the buffer.
    buffer->buffer = (char *) (buffer + 1);
    buffer->size = size;

    return buffer;
}

/// @brief Append the new data to the buffer.
size_t iobuff_append (struct io_buffer *buffer, const char *data, size_t length, bool can_reallocate) {

    assert(buffer && data);

    // If the length is zero or the buffer is full, return early.
    if (length == 0 || iobuff_full(buffer)) {
        return 0;
    }

    size_t free_space = iobuff_space(buffer);

    // Reallocate the buffer if there is insufficient space.
    if (free_space < length) {

        if (can_reallocate) {
            size_t new_size = buffer->size + length;

            // Calculate the next power of two for the new buffer size.
            new_size = size_roundup(new_size);

            char *new_buffer = NULL;

            if ((new_buffer = realloc(buffer->buffer, new_size)) == NULL) {
                LOG_ERROR("Error reallocating buffer");
                return 0;
            }

            buffer->buffer = new_buffer;
            buffer->size = new_size;

            // Update the free space after reallocation.
            free_space = buffer->size - (buffer->head - buffer->tail);
        }
    }

    // Wrap around the buffer pointers for pointer arithmetic.
    const size_t whead = buffer->head & (buffer->size - 1);
    const size_t wtail = buffer->tail & (buffer->size - 1);

    // In case the buffer has wrapped around, we need to copy the data in two chunks.
    if (wtail <= whead) {
        size_t first_chunk = min(length, buffer->size - whead);
        size_t second_chunk = min(length - first_chunk, wtail);
        
        size_t total = first_chunk + second_chunk;

        memcpy(buffer->buffer + whead, data, first_chunk);
        memcpy(buffer->buffer, data + first_chunk, second_chunk);

        buffer->head += total;

        return total;
    }

    size_t chunk = min(length, wtail - whead);

    memcpy(buffer->buffer + whead, data, chunk);

    buffer->head += chunk;

    return chunk;
}

/// @brief Free the memory allocated for the buffer.
void iobuff_free (struct io_buffer *buffer) {

    assert(buffer);

    free(buffer->buffer);
    free(buffer);
}

/// @brief Send data to the client from the (circular) buffer.
ssize_t iobuff_send (struct client_context *client, struct io_buffer *buffer) {

    assert(client && buffer);

    // The number of bytes to send to the client.
    const size_t length = buffer->head - buffer->tail;

    // If the buffer is empty, we can return early.
    if (length == 0) {
        return 0;
    }

    // The number of bytes sent to the client.
    ssize_t sent = 0;

    // Wrap around the buffer pointers for pointer arithmetic.
    const size_t whead = buffer->head & (buffer->size - 1);
    const size_t wtail = buffer->tail & (buffer->size - 1);

    // Since the buffer is circular, we need to wrap around the buffer size.
    char *buffer_wrap = NULL;

    // If the buffer has wrapped around, straighten the buffer.
    if (buffer->tail > buffer->head) {

        if ((buffer_wrap = calloc(1U, buffer->size)) == NULL) {
            LOG_ERROR("Error allocating memory for buffer wrap");
            return -1;
        }

        const size_t tail_size = buffer->size - wtail;

        memcpy(buffer_wrap, buffer->buffer + wtail, tail_size);
        memcpy(buffer_wrap + tail_size, buffer->buffer, whead);

        sent = send(client->info->fd, buffer_wrap, length, 0);
    }
    else {
        sent = send(client->info->fd, buffer->buffer + wtail, length, 0);
    }

    // Check if the data was sent successfully, update the tail pointer.
    if (sent < 0) {
        LOG_ERROR("Error sending data to client");
        return -1;
    }

    buffer->tail += sent;

    // Free the buffer wrap if it was allocated.
    if (buffer_wrap != NULL) {
        free(buffer_wrap);
    }

    return sent;
}

// --- Function definitions, server --- //

int as_bind (struct async_server *server, const char* ipv4) {

    assert(server && ipv4);

    int retvalue = 0;

    if ((server->contexts = htable_create(MAX_CLIENTS, client_hash, client_compare)) == NULL) {
        LOG_ERROR("Error creating hash table");
        retvalue = -1;
        goto error;
    }

    if ((server->polled = create_pollfds(MAX_CLIENTS)) == NULL) {
        LOG_ERROR("Error creating pollfd array");
        retvalue = -1;
        goto error_poll;
    }

    if (server_bind(&server->info, ipv4) < 0) {
        retvalue = -1;
        goto error_server;
    }

    // Set the listener socket to non-blocking mode to avoid blocking on accept.
    int get_flags = 0;

    if ((get_flags = fcntl(server->info.fd, F_GETFL, 0)) < 0) {
        LOG_ERROR("Error getting file descriptor flags");
        goto error_server;
    }

    // Update the file descriptor flags.
    if (fcntl(server->info.fd, F_SETFL, get_flags | O_NONBLOCK) < 0) {
        LOG_ERROR("Error setting file descriptor flags");
        goto error_server;
    }

    // Monitor the server socket for incoming connections.
    if (add_event(server->polled, server->info.fd, POLLIN | POLLPRI) < 0) {
        LOG_ERROR("Error adding event to pollfds");
        goto error_server;
    }

    return 0;

error_server:
    destroy_pollfds(server->polled);
    server->polled = NULL;

error_poll:
    htable_destroy(server->contexts);
    server->contexts = NULL;

error:
    return retvalue;
}

struct client_context *as_accept (struct async_server *server, event_callback_t handler) {

    assert(server && handler);
    
    struct client_context *client;

    if ((client = client_alloc()) == NULL) {
        LOG_ERROR("Error allocating memory for client context");
        goto error;
    }

    client->event_handler = handler;

    if (server_accept(&server->info, client->info) < 0) {
        goto error_free;
    }

    // Get the file descriptor flags to avoid overwriting them.
    int get_flags = 0;

    if ((get_flags = fcntl(client->info->fd, F_GETFL, 0)) < 0) {
        LOG_ERROR("Error getting file descriptor flags");
        goto error_disconnect;
    }

    // Update the file descriptor flags.
    if (fcntl(client->info->fd, F_SETFL, get_flags | O_NONBLOCK) < 0) {
        LOG_ERROR("Error setting file descriptor flags");
        goto error_disconnect;
    }

    if (add_event(server->polled, client->info->fd, POLLIN | POLLOUT | POLLHUP) < 0) {
        LOG_ERROR("Error adding event to pollfds");
        goto error_disconnect;
    }

    if (htable_insert(server->contexts, &client->info->fd, client) < 0) {
        LOG_ERROR("Error inserting client context into hash table");
        goto error_disconnect;
    }

    // Initialize the input and output buffers.
    if ((client->input = iobuff_alloc(BUFFER_SIZE)) == NULL) {
        LOG_ERROR("Error allocating memory for input buffer");
        goto error_in_buffer;
    }

    client->input->size = BUFFER_SIZE;

    if ((client->output = iobuff_alloc(BUFFER_SIZE)) == NULL) {
        LOG_ERROR("Error allocating memory for output buffer");
        goto error_out_buffer;
    }

    client->output->size = BUFFER_SIZE;

    return client;

error_out_buffer: // GOTO: Free the output buffer and continue to free the input buffer.
    iobuff_free(client->output);

error_in_buffer: // GOTO: Free the input buffer and continue to free the client context.
    iobuff_free(client->input);

error_disconnect: // GOTO: Disconnect the client and free resources.
    client_close(client->info);

error_free: // GOTO: Free memory allocated for the client context.
    free(client);

error: // GOTO: Return NULL on error.
    return NULL;
}

/// @brief Process the state of the client connection.
/// @param client The client struct.
/// @return 0 on success, -1 on failure.
void as_disconnect (struct async_server *server, struct client_context *client) {

    assert(server && client);

    // Remove the client from the polled file descriptors.
    remove_event(server->polled, client->info->fd);

    // Remove the client context from the hash table.
    (void) htable_remove(server->contexts, &client->info->fd);

    // Close the client socket.
    client_close(client->info);

    // Destroy the client context.
    free(client);
}

int as_poll (struct async_server *server, void* data) {

    assert(server);

    // Poll the server socket for incoming connections.
    int poll_result;

    if ((poll_result = poll_events(server->polled)) < 0) {
        LOG_ERROR("Error polling file descriptors");
        return -1;
    }

    // Select the server's events, i.e. incoming connections.
    int server_events = server->polled->fds[0].revents;

    if (server_events) {
        server->event_handler(server, server_events, data);
    }

    // Poll the client sockets for incoming data.
    for (unsigned int i = 1; i < server->polled->polled; i++) {

        if (server->polled->fds[i].revents == 0) {
            continue;
        }

        // If there are events on the client socket, process the connection.
        struct client_context *client = htable_get(server->contexts, &server->polled->fds[i].fd);

        // Call the client event handler to process the connection, no need to check for NULL.
        client->event_handler(client, server->polled->fds[i].revents, data);
    }

    return 0;
}

void as_cleanup (struct async_server *server) {

    assert(server);

    // Destroy the pollfds struct and close all file descriptors being polled.
    if (server->polled != NULL) {

        for (unsigned int i = 0; i < server->polled->polled; i++) {
            (void) close(server->polled->fds[i].fd);
        }

        destroy_pollfds(server->polled);
    }

    htable_destroy(server->contexts);
}