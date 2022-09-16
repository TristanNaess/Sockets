#include <curses.h>
#include <vector>
#include <string>
#include <semaphore>
#include <thread>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <cstdlib>

#include "socket.hpp"

std::binary_semaphore end_program{0};
std::binary_semaphore new_message{0};
std::binary_semaphore display_update{1};

void cleanup(int flag)
{
    end_program.release();
}

void draw_screen(const std::vector<std::string>& chat, const std::string& buffer, size_t cursor, size_t height, const std::string& prompt)
{
    clear();
    curs_set(0);

    move(height, 0);
    addstr(prompt.c_str());
    move(height, prompt.size());
    addstr(buffer.c_str());
   
    size_t cursor_y = height - 1;
    for (int i = (int)chat.size() - 1; i > -1; i--)
    {
        cursor_y--;
        move (cursor_y, 0);
        addstr(chat[i].c_str());
    }

    move(height, cursor + prompt.size());
    curs_set(1);

    refresh();
}


void socket_thread_handler(const std::string& ip, int port, std::string& message, std::vector<std::string>& chat, size_t chat_size)
{
    
    Socket sock(ip, port);
    SocketPoller poller(1);
    poller.register_socket(sock);


    while (!end_program.try_acquire())
    {
        if (new_message.try_acquire())
        {
            sock.write(message);
        }

        poller.poll(0);

        if (sock.get_events() & POLLIN)
        {

            chat.push_back(sock.read());
            if (chat.size() > chat_size) chat.erase(chat.begin(), chat.end() - chat_size );
            display_update.release();
        }
    }
    end_program.release();
}


int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <ip address> <port no>\n";
        return EXIT_FAILURE;
    }
    // need to check arg validity

    // init window
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    nodelay(stdscr, TRUE);
    int height = LINES - 1;
    std::string buffer;
    size_t cursor = 0;
    move(height, cursor);
    std::string prompt = ": ";
    
    // init socket on thread
    std::string message;
    std::vector<std::string> chat;
    std::thread socket_thread(socket_thread_handler, std::string(argv[1]), std::stoi(argv[2]),std::ref(message), std::ref(chat), height - 2);

    signal(SIGINT, cleanup);
    signal(SIGKILL, cleanup);

    int in;
    while (!end_program.try_acquire())
    {
        if (display_update.try_acquire()) draw_screen(chat, buffer, cursor, height, prompt);

        if ((in = getch()) == ERR) continue;
        
        switch (in)
        {
            case '\n':
                message = buffer;
                new_message.release();
                buffer.clear();
                cursor = 0;
                break;

            case KEY_BACKSPACE:
                if (cursor != 0)
                {
                    buffer.erase(cursor - 1, 1);
                    cursor--;
                }
                break;
                
            case KEY_DC:
                if (cursor != buffer.size())
                {
                    buffer.erase(cursor, 1);
                }
                break;

            case KEY_RIGHT:
                if (cursor < buffer.size())
                {
                    cursor++;
                }
                break;

            case KEY_LEFT:
                if(cursor > 0)
                {
                    cursor--;
                }
                break;

            default:
                if (isprint(in) && cursor != (size_t)(COLS - 1) - prompt.size())
                {
                    buffer.insert(cursor, 1, in);
                    cursor++;
                }
        }
        display_update.release();
    }
    
    end_program.release();
    socket_thread.join();
    endwin();
}
