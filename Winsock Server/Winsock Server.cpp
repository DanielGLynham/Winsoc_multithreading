// Winsock Server Application

#include "stdafx.h"

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <list>
#include <deque>
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <string>
#include <queue>
#include <random>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")	// Use this library whilst linking - contains the Winsock2 implementation.

CRITICAL_SECTION CriticalSection;

std::queue<std::string> data_queue;
bool gameStarted = false;

struct sockaddr_in client_address1;	// Placeholder for client address information 
struct sockaddr_in client_address2;	// Placeholder for client address information 


unsigned __stdcall ReceiveFunction(void *pArguments)
{
	std::cout << "Starting Winsock on thread." << std::endl;

	WSADATA WsaDat;

	// Initialise Windows Sockets
	if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != 0)
	{
		std::cout << "WSA Initialization failed!\r\n";
		WSACleanup();
		return 0;
	}

	// Create a unbound socket.
	SOCKET Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (Socket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed!";
		WSACleanup();
		return 0;
	}

	// Now, try and bind the socket to any incoming IP address on Port 8888.
	SOCKADDR_IN serverInf;

	serverInf.sin_family = AF_INET;				// Address protocol family - internet protocol (IP addressing).
	serverInf.sin_addr.s_addr = htonl(INADDR_ANY);	// Will accept any IP address from anywhere.
	serverInf.sin_port = htons(8888);			// Port number - can change this, but needs to be the same on both client and server.

	if (bind(Socket, (SOCKADDR*)(&serverInf), sizeof(serverInf)) == SOCKET_ERROR)
	{
		std::cout << "Unable to bind socket!\r\n";
		WSACleanup();
		return 0;
	}

	// Now Sockets have been initialised, so now wait for some data from a client...

	bool finished = false;
	int users = 0;

	std::string p1Name, p2Name;
	std::vector<int> snakeBody_x, snakeBody_y;
	int fruit_x = 0, fruit_y = 0;
	int snakeLength = 1;
	int direction = 0;
	while (!finished)
	{
		int sendOk;
		if (!gameStarted)
		{
			const int buffer_size = 1024;
			char buffer[buffer_size];	// Space for the data.

			struct sockaddr_in client_address;	// Placeholder for client address information - 'recvfrom' will fill this in and return the size of this data in client_address_size.
			int client_address_size = sizeof(client_address);

			std::cout << "Waiting for data from client..." << std::endl;

			int bytes_received = recvfrom(Socket, buffer, buffer_size, 0, (SOCKADDR*)&client_address, &client_address_size);
			bool gotBoth = false;
			if (bytes_received == SOCKET_ERROR)
			{	// If there is an error, deal with it here...
				std::cout << "'recvfrom' failed with error " << WSAGetLastError();
				std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
				finished = true;
				break;
			}
			else
			{
				users++; // counting users because we only want two
				// No error, so put the data on the queue for the main thread.
				std::string t = buffer;	// t contains the string sent here from the current client.

				if (p1Name == "") // player one joins
				{
					p1Name = t;
					client_address1 = client_address;
				}
				else if (p2Name == "") // player one has already joined so must be player 2
				{
					p2Name = t;
					client_address2 = client_address;
					gotBoth = true;
				}
				else
				{
					// we got two people so don't do nothing :)
				}
				EnterCriticalSection(&CriticalSection); // only time we need to do this as otherwise we are only hearing from one player
				data_queue.push(t);
				LeaveCriticalSection(&CriticalSection);
				std::cout << users << std::endl;

				if (gotBoth) // have two players so send them eachothers names and if they are player one or player two
				{
					std::string data_to_send = "one"; // say they are player 1
					strcpy(buffer, data_to_send.c_str());
					sendOk = sendto(Socket, buffer, buffer_size, 0, (SOCKADDR*)&client_address1, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					data_to_send = p2Name;
					strcpy(buffer, data_to_send.c_str()); // say other players name
					sendOk = sendto(Socket, buffer, buffer_size, 0, (SOCKADDR*)&client_address1, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					data_to_send = "two"; // say they are player 2
					strcpy(buffer, data_to_send.c_str());
					sendOk = sendto(Socket, buffer, buffer_size, 0, (SOCKADDR*)&client_address2, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					data_to_send = p1Name; // say other players name
					strcpy(buffer, data_to_send.c_str());
					sendOk = sendto(Socket, buffer, buffer_size, 0, (SOCKADDR*)&client_address2, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					gameStarted = true;

				}
			}
		}
		else // playing
		{
			if (snakeBody_x.size() == 0) // game start up
			{
				srand((int)time(0));
				snakeBody_x.push_back(rand() % 18); // takes border into account
				snakeBody_y.push_back(rand() % 8);
				fruit_x = rand() % 18;
				fruit_y = rand() % 8;
				while (fruit_x == snakeBody_x.at(0) && fruit_y == snakeBody_y.at(0)) // don't place fruit on snake
				{
					fruit_x = rand() % 18;
					fruit_y = rand() % 8;
				}
				direction = 0;

			}
			else // snake going loop
			{
				bool playing = true;
				bool needsRestart = false;
				while (playing)
				{

					struct sockaddr_in client_addressTurn, client_addressNotTurn;
					if (snakeBody_x.at(0) < 9) // if in player one's screen, set player one to move snake
					{
						client_addressTurn = client_address1;
						client_addressNotTurn = client_address2;
					}
					else // if in player two's screen, set player one to move snake
					{
						client_addressTurn = client_address2;
						client_addressNotTurn = client_address1;
					}
					int client_address_size = sizeof(client_addressTurn);

					// send fruit position to both players
					sendOk = sendto(Socket, (char*)&fruit_x, sizeof(int), 0, (SOCKADDR*)&client_addressTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					sendOk = sendto(Socket, (char*)&fruit_y, sizeof(int), 0, (SOCKADDR*)&client_addressTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					sendOk = sendto(Socket, (char*)&fruit_x, sizeof(int), 0, (SOCKADDR*)&client_addressNotTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					sendOk = sendto(Socket, (char*)&fruit_y, sizeof(int), 0, (SOCKADDR*)&client_addressNotTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					// send snake over to both players, need length of vector first, then the vector
					int temp = snakeBody_x.size();
					sendOk = sendto(Socket, (char*)&temp, sizeof(int), 0, (SOCKADDR*)&client_addressTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					sendOk = sendto(Socket, (char*)&temp, sizeof(int), 0, (SOCKADDR*)&client_addressNotTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					sendOk = sendto(Socket, (char*)&snakeBody_x[0], snakeBody_x.size() * sizeof(int), 0, (SOCKADDR*)& client_addressTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					sendOk = sendto(Socket, (char*)&snakeBody_y[0], snakeBody_y.size() * sizeof(int), 0, (SOCKADDR*)& client_addressTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					sendOk = sendto(Socket, (char*)&snakeBody_x[0], snakeBody_x.size() * sizeof(int), 0, (SOCKADDR*)& client_addressNotTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					sendOk = sendto(Socket, (char*)&snakeBody_y[0], snakeBody_y.size() * sizeof(int), 0, (SOCKADDR*)& client_addressNotTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}




					const int buffer_size = 1024;
					char buffer[buffer_size];	// Space for the data.

					// tell player whos turn it is that they can enter thier move
					std::string data_to_send = "go";
					strcpy(buffer, data_to_send.c_str());
					sendOk = sendto(Socket, buffer, buffer_size, 0, (SOCKADDR*)&client_addressTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}
					// tell other player to hold on and not do anything
					data_to_send = "wait";
					strcpy(buffer, data_to_send.c_str());
					sendOk = sendto(Socket, buffer, buffer_size, 0, (SOCKADDR*)&client_addressNotTurn, client_address_size);
					if (sendOk == SOCKET_ERROR)
					{
						finished = true;
						break;
					}

					// hear back from the player who's turn it is with the move they made
					int bytes_received = recvfrom(Socket, buffer, buffer_size, 0, (SOCKADDR*)&client_addressTurn, &client_address_size);
					if (bytes_received == SOCKET_ERROR)
					{	// If there is an error, deal with it here...
						std::cout << "'recvfrom' failed with error " << WSAGetLastError();
						std::cout << std::endl << "Most likely, the other player has left or the server has shut down." << std::endl;
						finished = true;
						break;
					}
					else
					{
						int snakeHead_x = snakeBody_x.at(0), snakeHead_y = snakeBody_y.at(0); // temp snake head
						// No error, so put the data on the queue for the main thread.
						char t = buffer[0];	// t contains the string sent here from the current client.
						std::cout << t; // take input
						// 0  == up. 1 == left. 2 == right. 3 == down
						if (t == 'w' || t == 'W') // move snake up
						{
							direction = 0;

						}
						else if (t == 'a' || t == 'A') // move snake left
						{
							direction = 1;

						}
						else if (t == 'd' || t =='D') // move snake right
						{
							direction = 2;

						}
						else if (t == 's' || t == 'S') // move down
						{
							direction = 3;

						}
						else if (t == 'q' || t == 'Q') // wants to quit game
						{
							// tell player two and close loops
							playing = false;
							finished = true;
						}
						else if (t == 'r' || t == 'R') // restart
						{
							needsRestart = true;
						}

						//Sleep(600);
						switch (direction)
						{
						case 0:
							if (snakeBody_y.at(0) > 0)
							{
								snakeHead_y = snakeBody_y.at(0) - 1;
								snakeHead_x = snakeBody_x.at(0);
							}
							else // hit the boundary
							{
								needsRestart = true;
							}
							break;
						case 1:
							if (snakeBody_x.at(0) > 0)
							{
								snakeHead_x = snakeBody_x.at(0) - 1;
								snakeHead_y = snakeBody_y.at(0);
							}
							else // hit boundary
							{
								needsRestart = true;
							}
							break;
						case 2:
							if (snakeBody_x.at(0) < 18)
							{
								snakeHead_x = snakeBody_x.at(0) + 1;
								snakeHead_y = snakeBody_y.at(0);
								finished = true;
							}
							else // hit boundary
							{
								needsRestart = true;
							}
							break;

						case 3:
							if (snakeBody_y.at(0) < 7)
							{
								snakeHead_y = snakeBody_y.at(0) + 1;
								snakeHead_x = snakeBody_x.at(0);
							}
							else // hit boundary
							{
								needsRestart = true;
							}
							break;

						}

						if (snakeHead_x == fruit_x && snakeHead_y == fruit_y) // snake eats a fruit
						{
							// extra tail piece placed on last tail position
							int x = snakeBody_x.at(snakeBody_x.size() - 1);
							int y = snakeBody_y.at(snakeBody_y.size() - 1);
							snakeBody_x.push_back(x);
							snakeBody_y.push_back(y);
							// new fruit
							srand((int)time(0));
							fruit_x = rand() % 18;
							fruit_y = rand() % 8;
							bool fruitOnSnake = false;
							for (int i = 0; i < snakeBody_x.size(); i++) // don't want fuit on a snake part
							{
								if (fruit_x == snakeBody_x.at(i) && fruit_y == snakeBody_y.at(i))
								{
									fruitOnSnake = true;
								}
							}
							while (fruitOnSnake) // keep trying to get fruit off snake parts
							{
								fruit_x = rand() % 18;
								fruit_y = rand() % 8;
								for (int i = 0; i < snakeBody_x.size(); i++)
								{
									if (fruit_x == snakeBody_x.at(i) && fruit_y == snakeBody_y.at(i))
									{
										fruitOnSnake = true;
									}
								}
							}
						}
						if (snakeBody_x.at(0) != snakeHead_x || snakeBody_y.at(0) != snakeHead_y) // make sure the snake did move - caused by player not entering a proper key
						{
							for (int i = snakeBody_x.size() - 1; i > 0; i--) // move snake parts forward with head
							{
								snakeBody_x.at(i) = snakeBody_x.at(i - 1); // start at end and move parts to one in front
								snakeBody_y.at(i) = snakeBody_y.at(i - 1);
							}	
							snakeBody_x.at(0) = snakeHead_x; // move the head now body has caught up
							snakeBody_y.at(0) = snakeHead_y;
							for (int i = 1; i < snakeBody_x.size(); i++) // check if snake has eaten a part of itself
							{
								if (snakeBody_x.at(0) == snakeBody_x.at(i) && snakeBody_y.at(0) == snakeBody_y.at(i))
								{
									needsRestart = true;
								}
							}
						}
						if (needsRestart)
						{
							if(snakeBody_x.size() < 50) // only need size 50 to win
								std::cout << "You LOST, restarting game."; // game over
							else
								std::cout << "You WON, restarting game."; // game over
								
							needsRestart = false;
							snakeBody_x.clear();
							snakeBody_y.clear();

							srand((int)time(0));
							snakeBody_x.push_back(rand() % 18); // takes border into account
							snakeBody_y.push_back(rand() % 8);
							fruit_x = rand() % 18;
							fruit_y = rand() % 8;
							while (fruit_x == snakeBody_x.at(0) && fruit_y == snakeBody_y.at(0)) // don't place fruit on snake
							{
								fruit_x = rand() % 18;
								fruit_y = rand() % 8;
							}
						}
						// Useful debugging information 
						std::cout << std::endl;
						std::cout << "Snake size : " << snakeBody_x.size();
						std::cout << std::endl;
						std::cout << " snake head x "<<snakeBody_x.at(0);
						std::cout << " snake head y "<<snakeBody_y.at(0);
						std::cout << " fruit x "<< fruit_x;
						std::cout << " fruit y "<< fruit_y;
						std::cout << std::endl;
					}
				}
			}
		}
	}

	// Shutdown the socket.
	shutdown(Socket, SD_SEND);

	// Close our socket entirely.
	closesocket(Socket);

	// Cleanup Winsock - release any resources used by Winsock.
	WSACleanup();

	_endthreadex(0);
	return 0;
}


int main()
{
	HANDLE hThread;
	unsigned threadID;

	InitializeCriticalSectionAndSpinCount(&CriticalSection, 1000);

	// Create a critical section  object for future use.
	hThread = (HANDLE)_beginthreadex(NULL, 0, &ReceiveFunction, NULL, 0, &threadID);

	// Enter the application's main loop...

	bool done = false;

	while (!done)
	{
		std::string d = "";
		
		EnterCriticalSection(&CriticalSection);	// Access the queue - safely via a critical section.

		if (data_queue.size() > 0)
		{
			d = data_queue.front();	// Retrieve the next data item from the queue.
			data_queue.pop();		// Eliminate it from the queue.
			
			std::cout << "Data Received:" << d;
			std::cout << std::endl;
		}

		LeaveCriticalSection(&CriticalSection);	// Leave the critical section.

		if (d == "quit") done = true;	// Terminate the app.
	}

	// Wait for the receiving thread to complete.
	WaitForSingleObject(hThread, INFINITE);

	// Clean up...
	CloseHandle(hThread);

	return 0;
}
