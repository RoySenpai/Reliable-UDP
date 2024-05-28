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

#include "include/RUDP_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
		fprintf(stderr, "Usage: %s -p <port>\n", *argv);
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	else if (strcmp(*(argv + 1), "-p") != 0)
	{
		fprintf(stderr, "Missing -p flag\n");
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	port = atoi(*(argv + 2));

	if (port < 1 || port > 65535)
	{
		fprintf(stderr, "Invalid port number\n");
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

	RUDP_socket server_socket = rudp_socket(true, port, RUDP_MTU_DEFAULT, RUDP_SOCKET_TIMEOUT_DEFAULT, RUDP_MAX_RETRIES_DEFAULT, true);

	if (server_socket == NULL)
	{
		free(buffer);
		free(rtt);
#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
		return 1;
	}

	fprintf(stdout, "Server is listening on port %d...\n", port);

	if (!rudp_accept(server_socket))
	{
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

		fprintf(stdout, "Waiting for client data...\n");

		// Expecting "READY" message from the client, if not received, break the loop.
		if (rudp_recv(server_socket, buffer, 5) == 0)
			break;

		gettimeofday(&start, NULL);
		int bytes_received = rudp_recv(server_socket, buffer, RUDP_FILE_SIZE);
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
	}

	free(buffer);

	double sum = 0.0;

	for (int i = 0; i < times; i++)
		sum += *(rtt + i);

	fprintf(stdout, "\nStatistics:\n");
	fprintf(stdout, "Number of RTT samples: %d\n", times);
	fprintf(stdout, "Average RTT: %.2f ms\n", sum / times);
	fprintf(stdout, "Average throughput: %.2f Mbps\n", ((RUDP_FILE_SIZE * 8.0) / 1024 / 1024) / (sum / times));
	fprintf(stdout, "Total time: %.2f ms\n\n", sum);

	fprintf(stdout, "Individual RTT samples:\n");

	for (int i = 0; i < times; i++)
		fprintf(stdout, "%d. %.2f ms\n", i + 1, *(rtt + i));

	free(server_socket);
	free(rtt);

#if defined(_WIN32) || defined(_WIN64)
		system("pause");
#endif
	
	return 0;
}