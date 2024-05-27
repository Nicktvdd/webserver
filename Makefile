# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -std=c++11

# Linker flags

# Targets
all: server client

server: Server.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

client: Client.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f server client