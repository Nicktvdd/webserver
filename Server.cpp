#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <fstream>

int main()
{
	int serverSocket, newSocket;
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t clientAddressLength;

	// Create a TCP socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
	{
		std::cerr << "Failed to create socket." << std::endl;
		return 1;
	}

	// Set up the server address
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(8080);

	// Bind the socket to the server address
	if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
	{
		std::cerr << "Failed to bind socket." << std::endl;
		close(serverSocket);
		return 1;
	}

	// Set socket to non-blocking mode
	int flags = fcntl(serverSocket, F_GETFL, 0);
	fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);

	// Listen for incoming connections
	if (listen(serverSocket, 5) == -1)
	{
		std::cerr << "Failed to listen for connections." << std::endl;
		close(serverSocket);
		return 1;
	}

	std::cout << "Server started. Listening on port 8080..." << std::endl;

	const int MAX_CLIENTS = 10;
	struct pollfd fds[MAX_CLIENTS + 1]; // +1 for server socket
	memset(fds, 0, sizeof(fds));

	fds[0].fd = serverSocket;
	fds[0].events = POLLIN | POLLOUT; // Add POLLOUT event

	int numClients = 0;

	while (true)
	{
		// Wait for events on the sockets
		int numReady = poll(fds, numClients + 1, -1);
		if (numReady == -1)
		{
			std::cerr << "Failed to poll for events." << std::endl;
			close(serverSocket);
			return 1;
		}

		// Check if there is a new connection on the server socket
		if (fds[0].revents & POLLIN)
		{
			// Accept a new connection
			clientAddressLength = sizeof(clientAddress);
			newSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
			if (newSocket == -1)
			{
				std::cerr << "Failed to accept connection." << std::endl;
				close(serverSocket);
				return 1;
			}

			std::cout << "New connection established." << std::endl;

			// Set socket to non-blocking mode
			flags = fcntl(newSocket, F_GETFL, 0);
			fcntl(newSocket, F_SETFL, flags | O_NONBLOCK);

			// Add the new client socket to the fds array
			if (numClients < MAX_CLIENTS)
			{
				numClients++;
				fds[numClients].fd = newSocket;
				fds[numClients].events = POLLIN | POLLOUT; // Add POLLOUT event
			}
			else
			{
				std::cerr << "Maximum number of clients reached." << std::endl;
				close(newSocket);
			}
		}

		// Check for events on client sockets
		for (int i = 1; i <= numClients; i++)
		{
			if (fds[i].revents & POLLIN)
			{
				char buffer[1024] = {0};
				ssize_t bytesReceived = recv(fds[i].fd, buffer, sizeof(buffer), 0);
				if (bytesReceived < 0)
				{
					if (errno == EWOULDBLOCK || errno == EAGAIN)
					{
						// Timeout occurred, handle it here
						std::cerr << "Timeout occurred while receiving data." << std::endl;
					}
					else
					{
						std::cerr << "Failed to receive a message. Error: " << strerror(errno) << std::endl;
						close(fds[i].fd);
						fds[i].fd = -1;
					}
				}
				else if (bytesReceived == 0)
				{
					std::cout << "Client disconnected." << std::endl;
					close(fds[i].fd);
					fds[i].fd = -1;
				}
				else
				{
					std::cout << "Client message: " << buffer << std::endl;

					// Send a response to the client
					const char *response = "Hello, Client!";
					ssize_t bytesSent = send(fds[i].fd, response, strlen(response), 0);
					if (bytesSent < 0)
					{
						if (errno == EWOULDBLOCK || errno == EAGAIN)
						{
							// Timeout occurred, handle it here
							std::cerr << "Timeout occurred while sending data." << std::endl;
						}
						else
						{
							std::cerr << "Failed to send the response. Error: " << strerror(errno) << std::endl;
							close(fds[i].fd);
							fds[i].fd = -1;
						}
					}
				}
			}
			else if (fds[i].revents & POLLOUT) // Check if client socket is ready for writing
			{
				// Read file data from the server and send it to the client
				std::ifstream file("file.txt", std::ios::binary);
				if (file)
				{
					std::string fileData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
					ssize_t bytesSent = send(fds[i].fd, fileData.c_str(), fileData.size(), 0);
					if (bytesSent < 0)
					{
						if (errno == EWOULDBLOCK || errno == EAGAIN)
						{
							// Timeout occurred, handle it here
							std::cerr << "Timeout occurred while sending file data." << std::endl;
						}
						else
						{
							std::cerr << "Failed to send file data. Error: " << strerror(errno) << std::endl;
							close(fds[i].fd);
							fds[i].fd = -1;
						}
					}
				}
				else
				{
					std::cerr << "Failed to open file." << std::endl;
					close(fds[i].fd);
					fds[i].fd = -1;
				}
			}
		}

		// Remove closed client sockets from the fds array
		int j = 1;
		for (int i = 1; i <= numClients; i++)
		{
			if (fds[i].fd != -1)
			{
				fds[j] = fds[i];
				j++;
			}
		}
		numClients = j - 1;
	}

	// Close the server socket
	close(serverSocket);

	return 0;
}