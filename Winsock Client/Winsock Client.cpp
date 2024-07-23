
// Creates a Winsock 'client' application.
// This application will try to conect to then obtain data from a 'server' application.

#include "stdafx.h"

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <list>
#include <deque>
#include <istream>
#include <conio.h>

#pragma comment(lib,"ws2_32.lib") 	// Use this library whilst linking - contains the Winsock2 implementation.
int _tmain(int argc, _TCHAR* argv[])
{
	// Initialise Winsock
	WSADATA WsaDat;
	if (WSAStartup(MAKEWORD(2,2), &WsaDat) != 0)
	{
		std::cout << "Winsock error - Winsock initialization failed\r\n";
		WSACleanup();
		return 0;
	}
	
	// Create our socket
	SOCKET Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (Socket == INVALID_SOCKET)
	{
		std::cout << "Winsock error - Socket creation Failed!\r\n";
		WSACleanup();
		return 0;
	}

	// Resolve IP address for hostname.
	struct hostent *host;

	// Change this to point to server, or ip address...

	if ((host = gethostbyname("localhost")) == NULL)   // In this case 'localhost' is the local machine. Change this to a proper IP address if connecting to another machine on the network.
	{
		std::cout << "Failed to resolve hostname.\r\n";
		WSACleanup();
		return 0;
	}

	
	// Sockets has now been initialised, so now can send some data to the server....


	const int buffer_size = 1024;
	char buffer[1024];
	SecureZeroMemory(&buffer, buffer_size);

	struct sockaddr_in server_address;
	int server_address_size = (int)sizeof(server_address);
	short port = 8888;	// Port number - can change this, but needs to be the same on both client and server.
	const char* server_ip_address = "127.0.0.1";	// The local host - change this for proper IP address of server if not on the local machine.
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = inet_addr(server_ip_address);


	bool wantQuit = false;
	int p1orp2;
	char mapdone[10][10];
	while (!wantQuit)
	{
		std::cout << "To join the game, type a name, then press enter : "; // enter name to send to server
		std::cin >> buffer;

		std::cout << "Sending datagram to the server." << std::endl;
		// send name to server
		int serverResult = sendto(Socket, buffer, buffer_size, 0, (SOCKADDR*)&server_address, server_address_size);
		if (serverResult == SOCKET_ERROR)
		{
			std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
			wantQuit = true;
			break;
		}
		// wait for both players
		std::cout << "waiting for other player...";
		int bytes_received = recvfrom(Socket, buffer, buffer_size, 0, (SOCKADDR*)&server_address, &server_address_size);
		if (bytes_received == SOCKET_ERROR)
		{	// If there is an error, deal with it here...
			std::cout << "recvfrom failed with error " << WSAGetLastError();
			std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
		}
		else // server lets us know which player we are so we know what map to base our game off of.
		{
			std::string t = buffer;
			if (t == "one")
			{
				p1orp2 = 1;
			}
			else if (t == "two")
			{
				p1orp2 = 2;
			}
			else
			{
				std::cout << "Game is full. Cannot join game.";
				wantQuit = true;
				break;
			}
		}
		// get other players name
		bytes_received = recvfrom(Socket, buffer, buffer_size, 0, (SOCKADDR*)&server_address, &server_address_size);
		if (bytes_received == SOCKET_ERROR)
		{	// If there is an error, deal with it here...
			std::cout << "recvfrom failed with error " << WSAGetLastError();
			std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
		}
		else
		{
			std::string acknowledge = buffer;
			std::cout << "You are playing with: " << acknowledge.c_str() << std::endl;
			// At this point we have received an acknowledgement from the server, so we can carry on...
			bool playing = true;
			while (playing)
			{
				std::vector<int> snakeBody_x, snakeBody_y;
				std::cout << "You are playing with: " << acknowledge.c_str() << std::endl;
				int fruit_x, fruit_y;
				// Get fruit locaiton
				int temp_fruit_x = recvfrom(Socket, buffer, sizeof(int), 0, (SOCKADDR*)&server_address, &server_address_size);  // fruit x
				if (temp_fruit_x == SOCKET_ERROR)
				{	// If there is an error, deal with it here...
					std::cout << "recvfrom failed with error " << WSAGetLastError();
					std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
					wantQuit = true;
					break;
				}
				else
				{
					fruit_x = static_cast<int>(*buffer);
				}
				int temp_fruit_y = recvfrom(Socket, buffer, sizeof(int), 0, (SOCKADDR*)&server_address, &server_address_size);  // fruit y
				if (temp_fruit_y == SOCKET_ERROR)
				{	// If there is an error, deal with it here...
					std::cout << "recvfrom failed with error " << WSAGetLastError();
					std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
					wantQuit = true;
					break;
				}
				else
				{
					fruit_y = static_cast<int>(*buffer);
				}

				// Get snake length
				int bytes = recvfrom(Socket, buffer, sizeof(int), 0, (SOCKADDR*)&server_address, &server_address_size);  // Length of vector.
				int vector_length;
				if (bytes == SOCKET_ERROR)
				{	// If there is an error, deal with it here...
					std::cout << "recvfrom failed with error " << WSAGetLastError();
					std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
					wantQuit = true;
					break;
				}
				else
				{
					vector_length = static_cast<int>(*buffer);
					// get snake body x coordinates
					bytes_received = recvfrom(Socket, buffer, vector_length * sizeof(int) * 10, 0, (SOCKADDR*)&server_address, &server_address_size); // get snake 
					if (bytes_received == SOCKET_ERROR)
					{	// If there is an error, deal with it here...
						std::cout << "recvfrom failed with error " << WSAGetLastError();
						std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
						wantQuit = true;
						break;
					}
					else
					{
						for (int n = 0; n < vector_length; ++n)
						{
							int x = static_cast<int>(*(buffer + (sizeof(int) * n)));

							snakeBody_x.push_back(x);
						}
					}
					// get snake body y coordinates, vector will always be the same length as the x coordinates
					bytes_received = recvfrom(Socket, buffer, vector_length * sizeof(int), 0, (SOCKADDR*)&server_address, &server_address_size); // get snake 
					if (bytes_received == SOCKET_ERROR)
					{	// If there is an error, deal with it here...
						std::cout << "recvfrom failed with error " << WSAGetLastError();
						std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
						wantQuit = true;
						break;
					}
					else
					{
						for (int n = 0; n < vector_length; ++n)
						{
							int x = static_cast<int>(*(buffer + (sizeof(int) * n)));

							snakeBody_y.push_back(x);
						}
					}
				}

				// neaten up console and display game
				system("cls");
				std::cout << "SKAKE" << std::endl << std::endl;
				std::cout << "You are playing with: " << acknowledge.c_str() << std::endl;
				std::cout << "score : " << snakeBody_x.size() - 1 << "/ 50" << std::endl;
				std::cout << std::endl << std::endl;
				std::cout << "X = Border, you don't want to crash into a border or you'll restart the game.";
				std::cout << std::endl;
				std::cout << "Empty space, your snake can move onto these freely";
				std::cout << std::endl;
				std::cout << "s = Snake head, this is the part of the snake that you move.";
				std::cout << std::endl;
				std::cout << "S = Snake body part, Don't eat your own tail or you'll restart the game.";
				std::cout << std::endl;
				std::cout << "F = Fruit, collect fruit to increase your score";
				std::cout << std::endl;
				std::cout << "if the fruit isn't on your screen then it's on your partners screen, try moving the snake to their screen.";
				std::cout << std::endl << std::endl;
				if (p1orp2 == 1) // if player one, door is on the right side
				{
					char map[10][10] =
					{
						{'X','X','X','X','X','X','X','X','X','X'},
						{'X',' ',' ',' ',' ',' ',' ',' ',' ',' '},
						{'X',' ',' ',' ',' ',' ',' ',' ',' ',' '},
						{'X',' ',' ',' ',' ',' ',' ',' ',' ',' '},
						{'X',' ',' ',' ',' ',' ',' ',' ',' ',' '},
						{'X',' ',' ',' ',' ',' ',' ',' ',' ',' '},
						{'X',' ',' ',' ',' ',' ',' ',' ',' ',' '},
						{'X',' ',' ',' ',' ',' ',' ',' ',' ',' '},
						{'X',' ',' ',' ',' ',' ',' ',' ',' ',' '},
						{'X','X','X','X','X','X','X','X','X','X'}
					};
					//place fuit
					if(fruit_x < 9) // if fruit is on this map side, place fruit
						map[fruit_y + 1][fruit_x + 1] = 'f'; // +1 to account for border
					// get snake position and put it in map
						for (int y = 0; y < snakeBody_x.size(); y++) // draw snake
						{
							if (snakeBody_x.at(y) < 9)
							{
								if(y > 0) // not head
									map[snakeBody_y.at(y) + 1][snakeBody_x.at(y) + 1] = 'S';
								else // is head
									map[snakeBody_y.at(y) + 1][snakeBody_x.at(y) + 1] = 's';

							}
						}

					for (int i = 0; i < 10; i++) // draw map
					{
						for (int j = 0; j < 10; j++)
						{
							std::cout << map[i][j];

						}
						std::cout << std::endl;
					}
				}
				else // player two has the door on the left side
				{
					char map[10][10] =
					{
						{'X','X','X','X','X','X','X','X','X','X'},
						{' ',' ',' ',' ',' ',' ',' ',' ',' ','X'},
						{' ',' ',' ',' ',' ',' ',' ',' ',' ','X'},
						{' ',' ',' ',' ',' ',' ',' ',' ',' ','X'},
						{' ',' ',' ',' ',' ',' ',' ',' ',' ','X'},
						{' ',' ',' ',' ',' ',' ',' ',' ',' ','X'},
						{' ',' ',' ',' ',' ',' ',' ',' ',' ','X'},
						{' ',' ',' ',' ',' ',' ',' ',' ',' ','X'},
						{' ',' ',' ',' ',' ',' ',' ',' ',' ','X'},
						{'X','X','X','X','X','X','X','X','X','X'}
					};
					if(fruit_x > 8) // fuit is on this map side
						map[fruit_y + 1][fruit_x - 9] = 'f'; // + 1 and -9 to account for other map and border
						for (int y = 0; y < snakeBody_x.size(); y++) // draw all snake parts
						{
							if (snakeBody_x.at(y) > 8)
							{
								if(y > 0) // is body
									map[snakeBody_y.at(y) + 1][snakeBody_x.at(y) - 9] = 'S';
								else // is head
									map[snakeBody_y.at(y) + 1][snakeBody_x.at(y) - 9] = 's';
							}
						}
					for (int i = 0; i < 10; i++) // draw map
					{
						for (int j = 0; j < 10; j++)
						{
							std::cout << map[i][j];

						}
						std::cout << std::endl;
					}
				}

				// Server lets us know if it's our turn or not - due to the snake position
				int bytes_received = recvfrom(Socket, buffer, buffer_size, 0, (SOCKADDR*)&server_address, &server_address_size);
				if (bytes_received == SOCKET_ERROR)
				{	// If there is an error, deal with it here...
					std::cout << "recvfrom failed with error " << WSAGetLastError();
					std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
					wantQuit = true;
					break;
				}
				else
				{
					std::string t = buffer;

					if (t == "go") // let player know the game instructions
					{
						std::cout << "It is your time to move.";
						std::cout << std::endl;
						std::cout << "Press w to move up";
						std::cout << std::endl;
						std::cout << "Press s to move down";
						std::cout << std::endl;
						std::cout << "Press a to move left";
						std::cout << std::endl;
						std::cout << "Press d to move right";
						std::cout << std::endl;
						std::cout << "Press r to reset the snake";
						std::cout << std::endl;
						std::cout << "Press q to quit the game";
						std::cout << std::endl;

						// tell the server what the player wants to do
						//std::cin >> buffer
						buffer[0] = _getch();
						std::cout << buffer;

						int serverResult = sendto(Socket, buffer, buffer_size, 0, (SOCKADDR*)&server_address, server_address_size);
						if (serverResult == SOCKET_ERROR)
						{
							std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
							wantQuit = true;
							break;
						}
					}
					else if (t == "wait") // tell player to hold on until the snake is on thier screen
					{
						std::cout << "The snake head is on other players screen, wait for snake to be on your screen to move the snake";
					}
				}
			}

		}

	}
	if (wantQuit)
	{
		// Shutdown our socket.
		shutdown(Socket, SD_SEND);

		// Close our socket entirely.
		closesocket(Socket);

		// Cleanup Winsock.
		WSACleanup();
		system("PAUSE");
		return 0;
	}
}
