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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/time.h>

#define RUDP_FILE_SIZE 10485760 // 10 MB

int main(int argc, char **argv)
{
	int port = 0, times = 0, arrsize = 1;
	char *buffer = NULL;
	double *rtt = NULL;

	// Argument validation
	if (argc != 3)
	{
		std::cerr << "Usage: " << *argv << " -p <port>" << std::endl;
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	else if (strcmp(*(argv + 1), "-p") != 0)
	{
		std::cerr << "Missing -p flag" << std::endl;
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	port = atoi(*(argv + 2));

	if (port < 1 || port > 65535)
	{
		std::cerr << "Invalid port number" << std::endl;
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	buffer = (char *)malloc(RUDP_FILE_SIZE);
	rtt = (double *)malloc(arrsize * sizeof(double));

	if (buffer == NULL || rtt == NULL)
	{
		perror("malloc");
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	RUDP_Socket server_sockfd(true, port, RUDP_MTU_DEFAULT, RUDP_SOCKET_TIMEOUT_DEFAULT, RUDP_MAX_RETRIES_DEFAULT, true);
	std::cout << "Server is listening on port " << port << "..." << std::endl;

	try
	{
		server_sockfd.accept();
	}

	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		free(buffer);
		free(rtt);
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	while (true)
	{
		struct timeval start, end;
		int bytes_read = 0;

		std::cout << "Waiting for client data..." << std::endl;

		try
		{
			if (!server_sockfd.recv(buffer, 5))
				break;
			
			gettimeofday(&start, NULL);
			int bytes_received = server_sockfd.recv(buffer, RUDP_FILE_SIZE);
			gettimeofday(&end, NULL);

			if (!bytes_received)
				break;

			else if (bytes_received < 0)
			{
				free(buffer);
				free(rtt);
#if defined(_WIN32) || defined(_WIN64)
				system("pause");
#endif
				return 1;
			}

			bytes_read += bytes_received;

			*(rtt + times++) = (double)(end.tv_sec - start.tv_sec) * 1000.0 + (double)(end.tv_usec - start.tv_usec) / 1000.0;

			if (times == arrsize)
			{
				arrsize *= 2;
				rtt = (double *)realloc(rtt, arrsize * sizeof(double));

				if (rtt == NULL)
				{
					perror("realloc");
					free(buffer);
#if defined(_WIN32) || defined(_WIN64)
					system("pause");
#endif
					return 1;
				}
			}

			std::cout << "Received " << bytes_read << " bytes from client." << std::endl;
		}

		catch (const std::exception &e)
		{
			std::cerr << e.what() << std::endl;
			free(buffer);
			free(rtt);
#if defined(_WIN32) || defined(_WIN64)
			system("pause");
#endif
			return 1;
		}
	}

	free(buffer);

	double sum = 0.0;

	for (int i = 0; i < times; i++)
		sum += *(rtt + i);

	std::cout << std::fixed;
	std::cout.precision(2);
	std::cout << "Statistics:" << std::endl;
	std::cout << "Number of RTT samples: " << times << std::endl;
	std::cout << "Average RTT: " << sum / times << " ms" << std::endl;
	std::cout << "Average throughput: " << ((RUDP_FILE_SIZE * 8.0) / 1024 / 1024) / (sum / times) << " Mbps" << std::endl;
	std::cout << "Total time: " << sum << " ms" << std::endl << std::endl;

	std::cout << "Individual RTT samples:" << std::endl;

	for (int i = 0; i < times; i++)
		std::cout << i + 1 << ". " << *(rtt + i) << " ms" << std::endl;

	free(rtt);
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif

	return 0;
}