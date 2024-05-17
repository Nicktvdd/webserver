#include <iostream>
#include <fstream>
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

	// Open the file to upload
	std::ifstream file("/path/to/file.txt", std::ios::binary);
	if (!file)
	{
		std::cerr << "Failed to open the file." << std::endl;
		return 1;
	}

	// Send the file contents to the server
	char buffer[1024];
	while (file.read(buffer, sizeof(buffer)))
	{
		if (send(clientSocket, buffer, file.gcount(), 0) < 0)
		{
			std::cerr << "Failed to send the file." << std::endl;
			return 1;
		}
	}

	// Close the file
	file.close();

	// Notify the server that the file upload is complete
	if (send(clientSocket, "", 0, 0) < 0)
	{
		std::cerr << "Failed to send the file completion signal." << std::endl;
		return 1;
	}

	// Receive a response from the server
	char response[1024] = {0};
	if (recv(clientSocket, response, sizeof(response), 0) < 0)
	{
		std::cerr << "Failed to receive a response." << std::endl;
		return 1;
	}

	std::cout << "Server response: " << response << std::endl;

	// Close the socket
	close(clientSocket);

	return 0;
}