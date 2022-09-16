# ---------------------------------------
# Program Specific Defines
# ---------------------------------------
CC := g++
# This is necessary for skipping the file when compiling unit tests
LIBRARIES := ncurses
DEFINITIONS := 
FLAGS = -g -std=c++20 -Wall -Werror

# ---------------------------------------
# Probably don't need to change
# ---------------------------------------

# Directory layout
SDIR := source
IDIRS := include #include/vendor

# create flags
LIBS = $(patsubst %, -l%, $(LIBRARIES))
DEFS = $(patsubst %, -D%, $(DEFINITIONS))
IPATHS = $(patsubst %, -I%, $(IDIRS))
CFLAGS = $(FLAGS) $(IPATHS) $(LIBS) $(DEFS)


basic: $(SDIR)/socket.cpp $(SDIR)/basic_echo/client.cpp $(SDIR)/basic_echo/server.cpp 
	$(CC) -o server $(SDIR)/socket.cpp $(SDIR)/basic_echo/server.cpp $(CFLAGS)
	$(CC) -o client $(SDIR)/socket.cpp $(SDIR)/basic_echo/client.cpp $(CFLAGS)

multi_echo: $(SDIR)/socket.cpp $(SDIR)/multi_client_echo/client.cpp $(SDIR)/multi_client_echo/server.cpp 
	$(CC) -o server $(SDIR)/socket.cpp $(SDIR)/multi_client_echo/server.cpp $(CFLAGS)
	$(CC) -o client $(SDIR)/socket.cpp $(SDIR)/multi_client_echo/client.cpp $(CFLAGS)

chat: $(SDIR)/socket.cpp $(SDIR)/chat/client.cpp $(SDIR)/chat/server.cpp 
	$(CC) -o server $(SDIR)/socket.cpp $(SDIR)/chat/server.cpp $(CFLAGS)
	$(CC) -o client $(SDIR)/socket.cpp $(SDIR)/chat/client.cpp $(CFLAGS)

.PHONY: clean

clean:
	rm client server

