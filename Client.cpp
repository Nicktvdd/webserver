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
	std::ifstream file("File.txt", std::ios::binary);
	if (!file)
	{
		std::cerr << "Failed to open the file." << std::endl;
		return 1;
	}

	// Send the "SEND_FILE" message to the server
	const char *message = "SEND_FILE";
	ssize_t bytesSent = send(clientSocket, message, strlen(message), 0);
	if (bytesSent < 0)
	{
		std::cerr << "Failed to send the message. Error: " << strerror(errno) << std::endl;
		return 1;
	}

	// Send the file contents to the server
	char buffer[1024];
	while (file.read(buffer, sizeof(buffer)))
	{
		ssize_t bytesSent = send(clientSocket, buffer, file.gcount(), 0);
		if (bytesSent < 0)
		{
			std::cerr << "Failed to send the file data. Error: " << strerror(errno) << std::endl;
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

	// Send the file completion signal to the server
	const char *completionSignal = "FILE_COMPLETE";
	if (send(clientSocket, completionSignal, strlen(completionSignal), 0) < 0)
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