#include <iostream>
#include <string>

#include "socket.hpp"

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port no>\n";
        return EXIT_FAILURE;
    }

    int port_no;
    try
    {
        port_no = std::stoi(argv[1]);
        if (!is_legal_portno(port_no))
        {
            std::cerr << "Error: argument is not a valid port number\n";
            return EXIT_FAILURE;
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Error: argument could not be interpreted as a number";
        return EXIT_FAILURE;
    }

    try
    {
        PublicSocket pub_sock{port_no};
        SocketPoller poller(5);
        poller.register_socket(pub_sock);

        std::cout << "Server listening on port " << port_no << '\n';
 
        std::vector<Socket> sockets;
        sockets.reserve(4);

        std::string buffer;
        while (true)
        {
            poller.poll();

            std::cout << poller.get_info() << '\n';

            if (pub_sock.get_events() & POLLIN)
            {
                sockets.push_back(std::move(pub_sock.accept()));
                poller.register_socket(sockets.back());
                std::cout << "Connection accepted from " << sockets.back().to_string() << "(fd: " << sockets.back().get_fd() << ")\n";
            }
            
            for (size_t i = 0; i < sockets.size(); i++)
            {
                Socket& s = sockets[i];
                short events = s.get_events();

                if (events & POLLHUP)
                {
                    std::cout << "Socket " << s.to_string() << " disconnected\n";
                    poller.remove_socket(s);
                    sockets.erase(sockets.begin() + i);
                    i--;
                    if (sockets.size() == 0) std::cout << "All connections closed\n";
                    continue;
                }
                if (events & POLLIN)
                {
                    buffer = s.read();

                    if (buffer.size())
                    {
                        std::cout << "message from " << s.to_string() << " : " << buffer << '\n';
                        for (char& c : buffer)
                        {
                            if ((c & 0xdf) >= 'A' && (c & 0xdf) <= 'Z') c ^= 0x20; // swap case of all letters
                        }
                    }
                    s.write(buffer); // Need to write to trigger POLLHUP
                }
            }
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Some other error occurred\n";
        return EXIT_FAILURE;
    }
}
