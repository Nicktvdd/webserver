#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main()
{
	int clientSocket;
	struct sockaddr_in serverAddress;

	// Create a TCP socket
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == -1)
	{
		std::cerr << "Failed to create socket." << std::endl;
		return 1;
	}

	// Set up the server address
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8080);
	if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0)
	{
		std::cerr << "Invalid address/ Address not supported." << std::endl;
		return 1;
	}

	// Connect to the server
	if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
	{
		std::cerr << "Connection failed." << std::endl;
		return 1;
	}

	std::cout << "Connected to the server." << std::endl;

	std::cout << "Connected to the server." << std::endl;

	// Send a message to the server
	const char *message = "Hello, Server!";
	if (send(clientSocket, message, strlen(message), 0) < 0)
	{
		std::cerr << "Failed to send the message." << std::endl;
		return 1;
	}

	// Receive a response from the server
	char buffer[1024] = {0};
	if (recv(clientSocket, buffer, sizeof(buffer), 0) < 0)
	{
		std::cerr << "Failed to receive a response." << std::endl;
		return 1;
	}

	std::cout << "Server response: " << buffer << std::endl;

	// Close the socket
	close(clientSocket);

	return 0;
}