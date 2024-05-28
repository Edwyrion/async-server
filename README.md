# Generic event-based TCP server implementation

This networking library written in C provides a simple asynchronous event-driven interface for the TCP server. The user only has to provide their own implementations of event handlers for both server and clients, which are then used as callbacks inside the main polling function of the library. The purpose of this library is to lower the development time and abstract the elementary network programming. This library is mostly for educational purposes, but it has been tested and can be used.

## Description

This event-driver TCP server interface is an asynchronous, non-blocking polling manager for the server wrapped inside a ```struct server_context```, that keeps track of every single connected client defined by the ```struct client_context``` via hash table keyed by their respective file descriptors. Connected clients are by default polled for both incoming ```POLLIN``` and outcoming ```POLOUT``` events, this polling is managed by the ```struct pollfds```. For convenience there is a ```unsigned int status``` included in client context structure for the purposes of user-defined finite state machine implementation without the need of keeping track of states in a separate data structure.

The main polling for events is managed by the ```as_poll(...)``` function call, which should be called in a loop. The user can optinally pass a pointer to the custom data, that will be propagated to every call-back as a function argument.

For the purposes of inter-communication, an implementation of a ring buffer ```struct io_buffer``` is provided, including basic utility functions, e.g. ```iobuff_append(...)``` which adds new data to the ring buffer with wrapping, or ```iobuff_send(...)``` which tries to empty the whole buffer and send the data to the client. Current implementation supports only sizes that are of powers of two and the default is ```BUFFER_SIZE 1024UL```.

## Usage
1. User must first define a server's event handler function with signature ```void (void *, int, void *)```.
2. Bind the server ```struct server_context``` to the specified address or a port with ```as_bind(...)```.
3. Poll the server for the events with ```as_poll(...)``` and accept new connections with ```as_accept(...)```.
```C
static void server_handler (void *context, int event, void *data) {
    // Unused parameters.
    (void) data;

    // Since the event handler's signature is same for both clients and server we need a typecast.
    struct async_server *server = (struct async_server *) context;

    if (event & POLLIN || event & POLLPRI) {
        // Only accept one client at a time.
        struct client_context *client = NULL;

        if ((client = as_accept(server, client_handler)) == NULL) {
            return;
        }
    }
}

int main (int argc, char **argv) {
    // Unused parameters.
    (void) argc;
    (void) argv;

    // Disable buffering for stdout to print messages immediately.
    (void) setvbuf(stdout, NULL, _IONBF, 0);

    // Try to bind the server to the specified address and port.
    if (as_bind(&server, "127.0.0.1:8080", server_handler) < 0) {
        return EXIT_FAILURE;
    }

    // Main event loop.
    for (;;) {
        as_poll(&server, NULL);
    }

    return EXIT_SUCCESS;
}
```
Since the current implementation of the event-based interface doesn't read the data by itself, user must read the incoming data themselves, but due to the non-blocking nature of the implementation, the read call won't block. For the purposes of the inter-communication the implementation of the ring buffers is provided, which can be used roughly as follows:

1. Manually write data into ```struct io_buffer``` and reset the ```buffer->tail``` and ```buffer->head``` pointers.
2. Append the data by calling ```iobuff_append(...)```.
3. When ready to send the data, call ```iobuff_send(...)``` - this empties the whole ring buffer.

## API documentation

```C
// --- Function Prototypes, asynchronnous server --- //

/// @brief Create a listener socket on the specified port, TCP/IPv4 protocol.
/// @param port The port to listen on.
/// @return 0 on success, -1 on failure.
int as_bind (struct async_server *server, const char* ipv4);

/// @brief Accept a connection from a client for the listener socket.
/// @param server The server struct to accept the connection on.
/// @param handler The event handler for the client connection.
/// @return Pointer to the client context on success, NULL on failure.
struct client_context *as_accept (struct async_server *server, event_callback_t handler);

/// @brief Disconnect the client and free the associated resources.
/// @param server The server struct.
/// @param client The client context to disconnect.
void as_disconnect (struct async_server* server, struct client_context *client);

/// @brief Main loop to poll file descriptors for events and process connections.
/// @return 0 on success, -1 on failure.
int as_poll (struct async_server *server, void* data);

/// @brief Cleanup function to free resources.
void as_cleanup (struct async_server *server);
```

```C
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
```
