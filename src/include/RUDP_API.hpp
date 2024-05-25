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

#pragma once
#include <cstdint>

/*
 * @brief The MTU (Maximum Transmission Unit) of the network, default is 1458 bytes.
 */
#define RUDP_MTU_DEFAULT 1458

/*
 * @brief Maximum waiting time for an ACK / SYN-ACK packet in milliseconds, default is 100 milliseconds.
 */
#define RUDP_SOCKET_TIMEOUT_DEFAULT 100

/*
 * @brief The maximum number of retries for a packet, before giving up, default is 50 retries.
 */
#define RUDP_MAX_RETRIES_DEFAULT 50

class RUDP_Socket_p;

/*
 * @brief This class represents a Reliable UDP socket.
 * @note This class is not thread-safe, do not use the same object in multiple threads.
 */
class RUDP_Socket
{
	private:
		/*
		* @brief A pointer to the RUDP socket (wrapper).
		*/
		RUDP_Socket_p *_socket = nullptr;

	public:
		/*
		* @brief Creates a new RUDP socket.
		* @param isServer True if the RUDP socket acts like a server, false for client.
		* @param listen_port Port to listen on if the socket is a server. Ignored if the socket is a client.
		* @param MTU The MTU (Maximum Transmission Unit) of the network, default is 1500 bytes.
		* @param timeout Maximum waiting time for an ACK / SYN-ACK packet in milliseconds, default is 100 milliseconds.
		* @param max_retries The maximum number of retries for a packet, before giving up, default is 50 retries.
		* @param debug_mode True to enable debug mode, false otherwise. Default is false.
		* @note If the socket is a server, it will listen on the specified port.
		* @throws std::runtime_error if the socket creation fails, or if bind() fails.
		*/
		RUDP_Socket(bool isServer, uint16_t listen_port, uint16_t MTU = RUDP_MTU_DEFAULT, uint16_t timeout = RUDP_SOCKET_TIMEOUT_DEFAULT, uint16_t max_retries = RUDP_MAX_RETRIES_DEFAULT, bool debug_mode = false);

		/*
		* @brief Destructor.
		* @note Closes the socket if it is still open and disconnects if it is still connected.
		*/
		~RUDP_Socket();

		/*
		* @brief Connects to a server.
		* @param dest_ip Destination IP address.
		* @param dest_port Destination port.
		* @return True if the connection is successful, false otherwise.
		* @throws std::runtime_error if the socket is a server.
		*/
		bool connect(const char *dest_ip, unsigned short int dest_port);

		/*
		* @brief Accepts a connection from a client.
		* @return True if the connection is successful, false otherwise.
		* @throws std::runtime_error if the socket is a client.
		*/
		bool accept();

		/*
		* @brief Receives data from the connected peer.
		* @param buffer Buffer to store the received data.
		* @param buffer_size Size of the buffer.
		* @return Number of bytes received.
		* @throws std::runtime_error if the socket is not connected.
		*/
		int recv(void *buffer, uint32_t buffer_size);

		/*
		* @brief Sends data to the connected peer.
		* @param buffer Buffer containing the data to be sent.
		* @param buffer_size Size of the buffer.
		* @return Number of bytes sent.
		* @throws std::runtime_error if the socket is not connected.
		*/
		int send(void *buffer, uint32_t buffer_size);

		/*
		* @brief Disconnects from the connected peer.
		* @return True if the disconnection is successful, false otherwise.
		*/
		bool disconnect();
};