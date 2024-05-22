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

	// Argument check.
	if (argc != 5)
	{
		fprintf(stderr, "Usage: %s -ip <IP> -p <PORT>\n", *argv);
		return 1;
	}

	// Parse the arguments.
	if (strcmp(*(argv + 3), "-p") != 0)
	{
		fprintf(stderr, "Missing -p flag.\n");
		return 1;
	}

	port = atoi(*(argv + 4));

	if (port < 1 || port > 65535)
	{
		fprintf(stderr, "Invalid port number.\n");
		return 1;
	}

	fprintf(stdout, "Argument check passed, starting the program...\n");

	// Create a new RUDP socket.
	RUDP_socket client_socket = rudp_socket(false, 0, RUDP_MTU_DEFAULT, RUDP_SOCKET_TIMEOUT_DEFAULT, RUDP_MAX_RETRIES_DEFAULT);

	if (client_socket == NULL)
	{
		fprintf(stderr, "rudp_socket() failed.\n");
		return 1;
	}

	// Generate the data.
	fprintf(stdout, "Generating %d bytes of random data...\n", RUDP_FILE_SIZE);
	char *data = util_generate_random_data(RUDP_FILE_SIZE);

	if (data == NULL)
	{
		perror("util_generate_random_data()");
		return 1;
	}

	fprintf(stdout, "Successfully generated %d bytes of random data.\n", RUDP_FILE_SIZE);
	fprintf(stdout, "Connecting to %s:%d...\n", *(argv + 2), port);

	// Connect to the server.
	bool ret = rudp_connect(client_socket, *(argv + 2), port);

	if (!ret)
	{
		free(data);
		return 1;
	}

	fprintf(stdout, "Successfully connected to %s:%d!\n", *(argv + 2), port);

	while (true)
	{
		rudp_send(client_socket, "READY", 5);
		fprintf(stdout, "Sending %d bytes of data...\n", RUDP_FILE_SIZE);

		struct timeval start, end;

		gettimeofday(&start, NULL);
		rudp_send(client_socket, data, RUDP_FILE_SIZE);
		gettimeofday(&end, NULL);

		// Calculate the time it took to send the data.
		double time_taken = (end.tv_sec - start.tv_sec) * 1000.0 + ((double)end.tv_usec - start.tv_usec) / 1000.0;

		fprintf(stdout, "Successfully sent %d bytes of data!\n", RUDP_FILE_SIZE);
		fprintf(stdout, "Time taken: %.2f ms\n", time_taken);

		char choice = '\0';

		// Ask the user if they want to send more data.
		while (choice != 'y' && choice != 'n')
		{
			fprintf(stdout, "Do you want to send more data? (y/n) ");
			fflush(stdout);

			scanf(" %c", &choice);

			if (choice != 'y' && choice != 'n')
				fprintf(stdout, "Invalid choice. Please try again.\n");
		}

		if (choice == 'n')
			break;

		fprintf(stdout, "Continuing...\n");
	}

	fprintf(stdout, "Closing the connection...\n");
	rudp_disconnect(client_socket);
	free(client_socket);
	free(data);

	return 0;
}