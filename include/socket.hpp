#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

extern "C"
{
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
}

#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>


#ifdef VERBOSE
#include <cstdio>
#endif

#define BUFF_LEN 1024
#define MAX_PENDING 5

bool is_legal_ip(const std::string& ip);
bool is_legal_portno(int port);

class SocketPoller;

class BaseSocket
{
    public:
        int get_fd() const;
        void set_poller(SocketPoller* poller, int index); // poller passes self
        void remove_poller();
        int get_poller_index() const;

        short get_events();
        bool check_event(short event);

        std::string to_string() const;

    protected:
        int m_fd;
        struct sockaddr_in m_addr;

        SocketPoller* m_poller = nullptr; // reference only, raw pointer safe
        int m_poller_index = -1;
};

class Socket : public BaseSocket
{
    public:
        Socket();
        Socket(const std::string& ip, int port_no); // client constructor
        Socket(int fd, struct sockaddr_in& addr);   // server constructor called by PublicSocket::accept()
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;
        Socket(Socket&& other);
        Socket& operator=(Socket&& other);
        

        ~Socket();                                  // Needed for RAII

        std::string read();
        void write(const std::string& text);

};

class PublicSocket : public BaseSocket
{
    public:
        PublicSocket(int port_no);
        PublicSocket(const PublicSocket&) = delete;
        PublicSocket& operator=(const PublicSocket&) = delete;
        PublicSocket(PublicSocket&& other);
        PublicSocket& operator=(PublicSocket&& other);

        ~PublicSocket();

        Socket accept();
};

class SocketPoller
{
    public:
        SocketPoller(size_t reserved = 0);

        void register_socket(BaseSocket& s, short events = POLLIN | POLLHUP);
        void remove_socket(BaseSocket& s);

        void poll(int timeout_ms = -1);

        short get_events(size_t index);

        std::string get_info();

    private:
        std::vector<struct pollfd> m_pfds;
        size_t m_count = 0;

};

#endif //__SOCKET_HPP__
