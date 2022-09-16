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

        std::cout << "Server listening on port " << port_no << '\n';

        Socket sock{pub_sock.accept()};
        std::cout << "Connection accepted from " << sock.to_string() << "(fd: " << sock.get_fd() << ")\n";
        
        std::string buffer;
        while (true)
        {
            buffer = sock.read();
            buffer = "echo: \"" + buffer + "\"";
            sock.write(buffer);
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
