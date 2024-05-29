#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <fstream>
#include <vector>
#include <algorithm>

const int MAX_BUFFER_SIZE = 1024;
const int MAX_CLIENTS = 1000000;
const int SERVER_PORT = 8080;
const std::string FILE_NAME = "ReceivedFile.txt";

int createServerSocket()
{
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
	{
		std::cerr << "Failed to create socket." << std::endl;
		exit(1);
	}
	return serverSocket;
}

void bindServerSocket(int serverSocket)
{
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(SERVER_PORT);

	int opt = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::cerr << "Failed to set socket options." << std::endl;
		close(serverSocket);
		exit(1);
	}

	if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
	{
		std::cerr << "Failed to bind socket." << std::endl;
		close(serverSocket);
		exit(1);
	}
}

void setSocketNonBlocking(int socket)
{
	int flags = fcntl(socket, F_GETFL, 0);
	fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

void listenForConnections(int serverSocket)
{
	if (listen(serverSocket, MAX_CLIENTS) == -1)
	{
		std::cerr << "Failed to listen for connections." << std::endl;
		close(serverSocket);
	}
}

int acceptConnection(int serverSocket)
{
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);
	int newSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
	if (newSocket == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "Failed to accept connection." << std::endl;
			close(serverSocket);
		}
	}
	return newSocket;
}

ssize_t receiveData(int socket, char *buffer, size_t bufferSize)
{
	ssize_t bytesRead = recv(socket, buffer, bufferSize, 0);
	if (bytesRead < 0)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			std::cerr << "Failed to receive data. Error: " << strerror(errno) << std::endl;
			close(socket);
		}
	}
	return bytesRead;
}

void sendData(int socket, const char *data, size_t dataSize)
{
	ssize_t totalBytesSent = 0;
	while (totalBytesSent < dataSize)
	{
		ssize_t bytesSent = send(socket, data + totalBytesSent, dataSize - totalBytesSent, 0);
		if (bytesSent < 0)
		{
			if (errno != EWOULDBLOCK && errno != EAGAIN)
			{
				std::cerr << "Failed to send data. Error: " << strerror(errno) << std::endl;
				close(socket);
			}
		}
		else if (bytesSent == 0)
		{
			std::cerr << "Connection closed by client." << std::endl;
			close(socket);
		}
		else
		{
			totalBytesSent += bytesSent;
		}
	}
}

void receiveFile(int clientSocket)
{
    std::ofstream file(FILE_NAME, std::ios::out | std::ios::binary);
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytesRead;
    const char *ack = "ACK";

    printf("Receiving file...\n");
    send(clientSocket, ack, strlen(ack), 0);
    sleep(1);

    do
    {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0 && strcmp(buffer, "FILE_COMPLETE") != 0)
        {
            printf("Bytes read: %ld\n", bytesRead);
            file.write(buffer, bytesRead);

            // Send an acknowledgement to the client
            printf("Sending ACK\n");
            int sendResult;
            do
            {
                sendResult = send(clientSocket, ack, strlen(ack), 0);
                if (sendResult < 0)
                {
                    std::cerr << "Failed to send acknowledgement. Retrying..." << std::endl;
                }
            } while (sendResult < 0);
        }
    } while (bytesRead > 0 && strcmp(buffer, "FILE_COMPLETE") != 0);

    if (bytesRead < 0)
    {
        std::cerr << "Failed to receive data. Error: " << strerror(errno) << std::endl;
    }

    file.close();
    std::cout << "buffer: " << buffer << "\n\n\n" << std::endl;
}

void handleClientMessage(int clientSocket, const std::string &message)
{
	std::string upperMessage;
	std::transform(message.begin(), message.end(), std::back_inserter(upperMessage), ::toupper);
	if (upperMessage == "SEND_FILE")
	{
		receiveFile(clientSocket);
	}
	if (upperMessage == "FILE_COMPLETE")
	{
		send(clientSocket, "FILE_RECEIVED", strlen("FILE_RECEIVED"), 0);
		close(clientSocket);
		std::cout << "File transfer complete." << std::endl;
	}
}

void sendFileData(int clientSocket)
{
	std::ifstream file("File.txt", std::ios::binary);
	if (!file)
	{
		std::cerr << "Failed to open file." << std::endl;
		close(clientSocket);
		return;
	}

	char buffer[MAX_BUFFER_SIZE];
	while (!file.eof())
	{
		file.read(buffer, sizeof(buffer));
		sendData(clientSocket, buffer, file.gcount());
	}

	file.close();
}

int main()
{
	int serverSocket = createServerSocket();
	bindServerSocket(serverSocket);
	setSocketNonBlocking(serverSocket);
	listenForConnections(serverSocket);

	std::cout << "Server started. Listening on port " << SERVER_PORT << "..." << std::endl;

	std::vector<pollfd> fds(MAX_CLIENTS + 1, {-1, POLLIN});
	fds[0].fd = serverSocket;
	fds[0].events = POLLIN;

	int numClients = 0;
	printf("Number of clients: %d\n", numClients);

	while (true)
	{
		int numReady = poll(fds.data(), numClients + 1, -1);
		if (numReady == -1)
		{
			std::cerr << "Failed to poll for events." << std::endl;
			close(serverSocket);
		}

		if (fds[0].revents & POLLIN)
		{
			int newSocket = acceptConnection(serverSocket);
			if (newSocket != -1)
			{
				std::cout << "New connection established." << std::endl;

				setSocketNonBlocking(newSocket);

				if (numClients < MAX_CLIENTS)
				{
					numClients++;
					std::cout << "Number of clients start: " << numClients << std::endl;
					fds[numClients].fd = newSocket;
					fds[numClients].events = POLLIN;
				}
				else
				{
					std::cerr << "Maximum number of clients reached." << std::endl;
					close(newSocket);
				}
			}
		}

		for (int i = 1; i <= numClients; i++)
		{
			if (fds[i].revents & POLLIN)
			{
				char buffer[MAX_BUFFER_SIZE] = {0};
				ssize_t bytesReceived = receiveData(fds[i].fd, buffer, sizeof(buffer));
				if (bytesReceived > 0)
				{
					std::cout << "Client message: " << buffer << std::endl;
					handleClientMessage(fds[i].fd, buffer);
				}
				else if (bytesReceived == 0)
				{
					std::cout << "Client disconnected." << std::endl;
					close(fds[i].fd);
					fds[i].fd = -1;
				}
			}
			else if (fds[i].revents & POLLOUT)
			{
				sendFileData(fds[i].fd);
				close(fds[i].fd);
				fds[i].fd = -1;
			}
		}

		fds.erase(std::remove_if(fds.begin() + 1, fds.end(), [](const pollfd &fd)
								 { return fd.fd == -1; }),
				  fds.end());
		numClients = fds.size() - 1;
	}

	close(serverSocket);
	return 0;
}
