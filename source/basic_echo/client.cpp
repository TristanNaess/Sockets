#include <iostream>
#include <string>

#include "socket.hpp"

int main(int argc, char** argv)
{
    if (argc < 2 || argc > 3)
    {
        std::cerr << "Usage: " << argv[0] << " <IP address> <portno> or <IP:portno>\n";
        return EXIT_FAILURE;
    }

    std::string ip;
    int port;

    if (argc == 3)
    {
        ip = argv[1];
        try
        {
            port = std::stoi(argv[2]);
        }
        catch (const std::invalid_argument& e)
        {
            std::cerr << e.what() << '\n';
        }

        if (!(is_legal_ip(ip) && is_legal_portno(port)))
        {
            std::cerr << "Invalid IP or port number\n";
            return EXIT_FAILURE;
        }
    }
    else
    {
        ip = argv[1];

        try
        {
            port = std::stoi(ip.substr(ip.find(':') + 1));
        }
        catch (const std::invalid_argument& e)
        {
            std::cerr << e.what() << '\n';
        }

        ip = ip.substr(0, ip.find(':'));
    }


    try
    {
        Socket sock(ip, port);

        std::string buffer;
        std::cout << "type Q/q <ENTER> to quit\n\n";
        bool loop = true;
        while (loop)
        {
            std::cout << "> ";
            std::getline(std::cin, buffer);
            if (buffer == "Q" || buffer == "q")
            {
                loop = false;
                continue;
            }

            sock.write(buffer);
            buffer = sock.read();

            std::cout << "Server: " << buffer << '\n';
        }
    }

    catch(const std::runtime_error& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
