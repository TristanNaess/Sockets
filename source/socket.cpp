#include "socket.hpp"

// ============= Utils ============= 

bool is_legal_ip(const std::string& ip)
{
    std::string num;
    num.reserve(3);
    size_t count = 0;

    for (char c : ip)
    {
        if (c == '.' && std::stoi(num) < 256)
        {
            count++;
            num = "";
        }
        else if (c >= '0' && c <= '9')
        {
            num += c;
        }
        else
        {
            return false;
        }
    }
    if (count == 3 && std::stoi(num) < 256)
        return true;
    return false;
}

bool is_legal_portno(int port)
{
    return (port > 0 && port < 65536);
}

// ============= BaseSocket ============= 

int BaseSocket::get_fd() const
{
    return m_fd;
}

void BaseSocket::set_poller(SocketPoller* poller, int index)
{
    m_poller = poller;
    m_poller_index = index;
}

void BaseSocket::remove_poller()
{
    m_poller = nullptr;
    m_poller_index = -1;
}

int BaseSocket::get_poller_index() const
{
    return m_poller_index;
}

short BaseSocket::get_events()
{
    if (m_poller != nullptr)
    {
        return m_poller->get_events(m_poller_index);
    }
    return 0;
}

bool BaseSocket::check_event(short event)
{
    if (m_poller != nullptr)
    {
        return m_poller->get_events(m_poller_index) & event;
    }

    return false;
}

std::string BaseSocket::to_string() const
{
    return std::string(inet_ntoa(m_addr.sin_addr)) + ":" + std::to_string(ntohs(m_addr.sin_port));
}

// ============= Socket ============= 

Socket::Socket()
{
    m_fd = -1;
}

Socket::Socket(const std::string& ip, int port_no)
{
#ifdef VERBOSE
    printf("Socket::Socket(cosnt std::string&, int)\n");
#endif

    if ((m_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        throw std::runtime_error("Error constructing socket");
    }

    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port_no);
    m_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(m_fd, (struct sockaddr*) &m_addr, sizeof(m_addr)) < 0)
    {
        throw std::runtime_error("Error connecting socket");
    }
}

Socket::Socket(int fd, struct sockaddr_in& addr)
{
#ifdef VERBOSE
    printf("Socket::Socket(int, struct sockaddr_in&)\n");
#endif
    m_fd = fd;
    m_addr = addr;
}

Socket::Socket(Socket&& other)
{
#ifdef VERBOSE
    printf("Socket::Socket(Socket&&)\n");
#endif
    m_fd = other.m_fd;
    m_addr = other.m_addr;
    m_poller = other.m_poller;
    m_poller_index = other.m_poller_index;
#ifdef VERBOSE
    printf("Setting old fd to -1\n");
#endif
    other.m_fd = -1;
}

Socket& Socket::operator=(Socket&& other)
{
#ifdef VERBOSE
    printf("Socket::operator=(Socket&&)\n");
#endif
    m_fd = other.m_fd;
    m_addr = other.m_addr;
    m_poller = other.m_poller;
    m_poller_index = other.m_poller_index;
#ifdef VERBOSE
    printf("Setting old fd to -1\n");
#endif
    other.m_fd = -1;
    return *this;
}

Socket::~Socket()
{
#ifdef VERBOSE
    printf("Socket::~Socket()\n");
    std::cout << "closing file descriptor: " << m_fd << '\n';
#endif

    if (m_fd != -1) ::close(m_fd);
}

std::string Socket::read()
{
    char buffer[BUFF_LEN + 1];
    size_t recv_len;
    std::string message;

    while (true)
    {
        if ((recv_len = recv(m_fd, buffer, BUFF_LEN, 0)) < 0)
        {
            throw std::runtime_error("Error reading from socket");
        }
        buffer[recv_len] = 0;
        message += buffer;

        if (buffer[recv_len - 1] == '\0')
            return message;
    }
}

void Socket::write(const std::string& text)
{
    if ((size_t) send(m_fd, text.c_str(), text.size() + 1, 0) != text.size() + 1)
    {
        throw std::runtime_error("Error sending over socket");
    }
}

// ============= PublicSocket ============= 

PublicSocket::PublicSocket(int port_no)
{
    if ((m_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        throw std::runtime_error("Error creating public socket");
    }

    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_addr.sin_port = htons(port_no);

    if (bind(m_fd, (struct sockaddr*) &m_addr, sizeof(m_addr)) < 0)
    {
        throw std::runtime_error("Error binding socket");
    }

    if (listen(m_fd, MAX_PENDING) < 0)
    {
        throw std::runtime_error("Error opening socket");
    }
}

PublicSocket::PublicSocket(PublicSocket&& other)
{
    m_fd = other.m_fd;
    m_addr = other.m_addr;
    m_poller = other.m_poller;
    m_poller_index = other.m_poller_index;
    
    other.m_fd = -1;
}

PublicSocket& PublicSocket::operator=(PublicSocket&& other)
{
    m_fd = other.m_fd;
    m_addr = other.m_addr;
    m_poller = other.m_poller;
    m_poller_index = other.m_poller_index;
    
    other.m_fd = -1;
    return *this;
}

PublicSocket::~PublicSocket()
{
    ::close(m_fd);
}

Socket PublicSocket::accept()
{
    int fd;
    struct sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);
    if ((fd = ::accept(m_fd, (struct sockaddr*) &addr, &addr_len)) == -1)
    {
        throw std::runtime_error("Error accepting connection");
    }
    
    return Socket{fd, addr};
}

// ============= SocketPoller ============= 


SocketPoller::SocketPoller(size_t reserved)
{
    if (reserved != 0) m_pfds.reserve(reserved);
}

void SocketPoller::register_socket(BaseSocket& s, short events)
{
    if (s.get_poller_index() != -1)
    {
        return;
    }

    size_t i;
    if (m_count != m_pfds.size())
    {
        for (i = 0; i < m_pfds.size(); i++)
        {
            if (m_pfds[i].fd == -1)
            {
                break;
            }
        }
    }
    else
    {
        i = m_pfds.size();

        m_pfds.push_back({-1, 0, 0});
    }

    m_pfds[i] = {
        s.get_fd(),
        events,
        0
    };

    s.set_poller(this, i);
    m_count++;
}

void SocketPoller::remove_socket(BaseSocket& s)
{
    int i;
    if ((i = s.get_poller_index()) != -1)
    {
        s.remove_poller();
        m_pfds[i].fd = -1;
    }
    m_count--;
}

void SocketPoller::poll(int timeout_ms)
{
    ::poll(m_pfds.data(), m_pfds.size(), timeout_ms);
}

short SocketPoller::get_events(size_t index)
{
    if (index >= m_pfds.size() || m_pfds[index].fd == -1)
    {
        throw std::runtime_error("Index not associated with pfd");
    }

    return m_pfds[index].revents;
}

std::string SocketPoller::get_info()
{
    std::stringstream ss;
    for (size_t i = 0; i < m_pfds.size(); i++)
    {
        ss << '[' << i << "] fd: " << m_pfds[i].fd << " events: " << std::hex << m_pfds[i].revents << std::dec << '\n';
    }
    return ss.str();
}
