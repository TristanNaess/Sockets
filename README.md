# As Of Yet Unnamed Socket Library
C++ Wrapper library around POSIX sockets to allow for RAII and std::string interface.<br>
The library currently only supports TCP and uses a null character to delimit the end of message.<br>
Other implementations and formats may be added later, but as part of a larger project, I don't have a need for other syntaxes right now.<br>

## Structure
The header includes two specialized sockets, `Socket` and `PublicSocket` derived from a `BaseSocket` class, and a associated `SocketPoller` class for socket event polling.<br>
`BaseSocket` apart from containing the underlying structres needed for POSIX sockets has public functions:<br>
- `int get_fd() const` - returns the socket file descriptor
- `short get_events()` - returns the bitfield of socket events set by `SocketPoller::poll()` associated with this socket if registered or 0 if not. Events are specified in [https://man7.org/linux/man-pages/man2/poll.2.html](poll(2))
- `bool check_event(short event)` - returns whether the passed event occurred
- `std::string to_string() const` - returns the string "ip:port" for connected device
### The following three functions are mainly intended for use by SocketPoller
- `void set_poller(SocketPoller* poller, int index)` - sets the pointer to the associated SocketPoller and the index of the associated file descriptor in the Poller. Effectively allows usage of `get_events()`. This could be set private and SocketPoller made friend, but I wanted to avoid friend usage
- `void remove_poller()` - nulls pointer to SocketPoller; Effectively disables `get_events()`
- `int get_poller_index() const` - returns the index of the Socket fd in the SocketPoller
`Socket` extends `BaseSocket` to implement a read-write socket connection with additional functions:<br>
- `Socket()` - construct unconnected socket, file desriptor = -1
- `Socket(const std::string& ip, int port_no)` - Client constructor from server IP and port number
- `Socket(int fd, struct sockaddr_in& addr)` - Serverside constructor used by `PublicSocket::accept()`
- `Socket(Socket&& other)` and `Socket& operator=(Socket&& other)` only move constructors allowed, file descriptor of 'other' will be set to -1 to render inoperable
- Copy constructor and copy assignment operator deleted
- `std::string read()` - reads a null-terminated string from the socket and returns it
- `void write(const std::string& text)` - writes 'text' to the socket followed by a null character
`PublicSocket` extends `BaseSocket` to implement a serverside public facing socket
- `PublicSocket(int port_no)` - Constructs a public facing socket on the specified port
- `PublicSocket(PublicSocket&& other)` and `PublicSocket& operator=(PublicSocket&& other)` onlu move constructors allowed, file descriptor of 'other' will be set to -1
- Copy constructor and copy assignment operator deleted
- `Socket accept()` - recieves a connection request on the socket and returns the constructed `Socket`
`SocketPoller` provides a wrapper around the poll() call and associated structures with functions:
- `SocketPoller(size_t reserved = 0)` - reserves the internal structures for the passed number of sockets.
- `void register_socket(BaseSocket& s, short events = POLLIN | POLLHUP)` - Adds passed socket to polling structure and watches for specified events
- `void remove_socket(BaseSocket& s)` - Removes socket from polling structure
- `void poll(int timeout_ms = -1)` - Checks for events on the registered sockets. Default blocks until an event occurs, or will timeout in the passed number of milliseconds. The event results can be retrieved with:
- `short get_events(size_t index)` - Returns the bitfield of events for the socket associated with 'index' as set by `SocektPoller::poll()`

## TODO
Functions intended to only be used by other classes, e.g. Socket(int, struct sockaddr\_in&) and set/remove\_poller() should be made private and the classes set as friend.<br>
Other types of socket protocol may be implemented, but are not necessary for my project so I haven't implemented them.
