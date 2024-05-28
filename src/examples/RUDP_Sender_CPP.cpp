/*
 *  Reliable UDP implementation
 *  Copyright (C) 2024  Roy Simanovich
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "include/RUDP_API.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>

#define RUDP_FILE_SIZE 10485760 // 10 MB

char *util_generate_random_data(unsigned int size)
{
	char *buffer = NULL;

	// Argument check.
	if (size == 0)
		return NULL;

	buffer = (char *)calloc(size, sizeof(char));

	// Error checking.
	if (buffer == NULL)
		return NULL; // Randomize the seed of the random number generator.

	srand(time(NULL));

	for (unsigned int i = 0; i < size; i++)
		*(buffer + i) = ((unsigned int)rand() % 256);

	return buffer;
}

int main(int argc, char **argv)
{
	int port = 0;
	bool isOK = true;

	// Argument check.
	if (argc != 5)
	{
		std::cerr << "Usage: " << *argv << " -ip <IP> -p <PORT>" << std::endl;
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	// Parse the arguments.
	if (strcmp(*(argv + 3), "-p") != 0)
	{
		std::cerr << "Missing -p argument." << std::endl;
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	port = atoi(*(argv + 4));

	if (port < 1 || port > 65535)
	{
		std::cerr << "Invalid port number." << std::endl;
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	std::cout << "Argument check passed, starting the program..." << std::endl;

	// Create a new RUDP socket, used the default values for the MTU, timeout, max retries, and debug mode.
	RUDP_Socket rudp_socket(false, 0, RUDP_MTU_DEFAULT, RUDP_SOCKET_TIMEOUT_DEFAULT, RUDP_MAX_RETRIES_DEFAULT, true);

	// Generate the data.
	std::cout << "Generating " << RUDP_FILE_SIZE << " bytes of random data..." << std::endl;
	char *data = util_generate_random_data(RUDP_FILE_SIZE);

	if (data == NULL)
	{
		perror("util_generate_random_data()");

#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	std::cout << "Successfully generated " << RUDP_FILE_SIZE << " bytes of random data." << std::endl;
	std::cout << "Connecting to " << *(argv + 2) << ":" << port << "..." << std::endl;

	// Connect to the server.
	bool ret = false;

	try
	{
		ret = rudp_socket.connect(*(argv + 2), port);
	}

	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		free(data);

#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	if (!ret)
	{
		free(data);

#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	std::cout << "Successfully connected to " << *(argv + 2) << ":" << port << "!" << std::endl;

	while (true)
	{
		std::cout << "Sending " << RUDP_FILE_SIZE << " bytes of data..." << std::endl;

		struct timeval start, end;

		try
		{
			char ready[] = "READY";
			int sent = rudp_socket.send(ready, 5);

			if (sent <= 0)
			{
				std::cerr << "Failed to send the READY message." << std::endl;
				isOK = false;
				break;
			}
			
			gettimeofday(&start, NULL);
			sent = rudp_socket.send(data, RUDP_FILE_SIZE);
			gettimeofday(&end, NULL);

			if (sent <= 0)
			{
				std::cerr << "Failed to send the data." << std::endl;
				isOK = false;
				break;
			}

			// Calculate the time it took to send the data.
			double time_taken = (end.tv_sec - start.tv_sec) * 1000.0 + ((double)end.tv_usec - start.tv_usec) / 1000.0;

			std::cout << std::fixed;
			std::cout.precision(2);
			std::cout << "Successfully sent " << RUDP_FILE_SIZE << " bytes of data!" << std::endl;
			std::cout << "Time taken: " << time_taken << " ms" << std::endl;

			char choice = '\0';

			// Ask the user if they want to send more data.
			while (choice != 'y' && choice != 'n')
			{
				std::cout << "Do you want to send more data? (y/n) ";
				std::cout.flush();

				std::cin >> choice;

				if (choice != 'y' && choice != 'n')
					std::cout << "Invalid choice. Please try again." << std::endl;
			}
			
			if (choice == 'n')
				break;

			std::cout << "Continuing..." << std::endl;
		}

		catch (const std::exception &e)
		{
			std::cerr << e.what() << std::endl;
			free(data);

#if defined(_WIN32) || defined(_WIN64)
			system("pause");
#endif
			return 1;
		}
	}

	if (isOK)
	{
		std::cout << "Successfully sent all the data." << std::endl;
		std::cout << "Closing the connection..." << std::endl;
		rudp_socket.disconnect();
	}

	else
		std::cerr << "An error occurred" << std::endl;

	free(data);

#if defined(_WIN32) || defined(_WIN64)
	system("pause");
#endif

	return 0;
}