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
	RUDP_Socket_p *_socket = nullptr;

public:
	/*
	 * @brief Creates a new RUDP socket.
	 * @param isServer True if the RUDP socket acts like a server, false for client.
	 * @param listen_port Port to listen on if the socket is a server. Ignored if the socket is a client.
	 * @param MTU The Maximum Transmission Unit of the network, default is RUDP_MTU_DEFAULT which is 1458 bytes.
	 * @param timeout Maximum waiting time for an ACK / SYN-ACK packet in milliseconds, default is RUDP_SOCKET_TIMEOUT_DEFAULT which is 100 milliseconds.
	 * @param max_retries The maximum number of retries for a packet, before giving up, default is RUDP_MAX_RETRIES_DEFAULT which is 50 retries.
	 * @param debug_mode True to enable debug mode, false otherwise. Default is false.
	 * @note If the socket is a server, it will listen on the specified port.
	 * @throws `std::runtime_error` if the socket creation fails, or if bind() fails.
	 */
	RUDP_Socket(bool isServer, uint16_t listen_port, uint16_t MTU = RUDP_MTU_DEFAULT, uint16_t timeout = RUDP_SOCKET_TIMEOUT_DEFAULT, uint16_t max_retries = RUDP_MAX_RETRIES_DEFAULT, bool debug_mode = false);

	/*
	 * @brief Destructor.
	 * @note Closes the socket if it is still open and disconnects if it is still connected.
	 */
	~RUDP_Socket();

public:
	/*
	 * @brief Connects to a server.
	 * @param dest_ip Destination IP address.
	 * @param dest_port Destination port.
	 * @return True if the connection is successful, false otherwise.
	 * @throws `std::runtime_error` if the socket is a server, if there is an active connection, or if there is a socket error.
	 */
	bool connect(const char *dest_ip, uint16_t dest_port);

	/*
	 * @brief Accepts a connection from a client.
	 * @return True if the connection is successful, false otherwise.
	 * @throws `std::runtime_error` if the socket is a client, if there is an active connection, or if there is a socket error.
	 */
	bool accept();

	/*
	 * @brief Receives data from the connected peer.
	 * @param buffer Buffer to store the received data.
	 * @param buffer_size Size of the buffer.
	 * @return Number of bytes received or -1 if an error occurs.
	 * @throws `std::runtime_error` if the socket is not connected, or if there is a socket error.
	 */
	int recv(void *buffer, uint32_t buffer_size);

	/*
	 * @brief Sends data to the connected peer.
	 * @param buffer Buffer containing the data to be sent.
	 * @param buffer_size Size of the buffer.
	 * @return Number of bytes sent, or -1 if an error occurs.
	 * @throws `std::runtime_error` if the socket is not connected, or if there is a socket error.
	 */
	int send(void *buffer, uint32_t buffer_size);

	/*
	 * @brief Disconnects from the connected peer.
	 * @return True if the disconnection is successful, false otherwise.
	 * @throws `std::runtime_error` if there is a socket error.
	 */
	bool disconnect();

public:
	/*
	 * @brief Gets the MTU (Maximum Transmission Unit) of the network.
	 * @return The MTU of the network.
	 */
	uint16_t getMTU() const;

	/*
	 * @brief Gets the timeout.
	 * @return Maximum waiting time for an ACK / SYN-ACK packet in milliseconds.
	 */
	uint16_t getTimeout() const;

	/*
	 * @brief Gets the maximum number of retries.
	 * @return The maximum number of retries for a packet, before giving up.
	 */
	uint16_t getMaxRetries() const;

	/*
	 * @brief Gets the MTU of the peer.
	 * @return The MTU of the peer, in case the peer has a smaller MTU.
	 * @throws `std::runtime_error` if the socket isn't connected.
	 */
	uint16_t getPeersMTU() const;
	/*
	 * @brief Checks if the socket is in debug mode.
	 * @return True if the socket is in debug mode, false otherwise.
	 */
	bool isDebugMode() const;

	/*
	 * @brief Checks if the socket is connected.
	 * @return True if the socket is connected, false otherwise.
	 */
	bool isConnected() const;

	/*
	 * @brief Checks if the socket is a server.
	 * @return True if the socket is a server, false otherwise.
	 */
	bool isServer() const;

public:
	/*
	 * @brief Sets the debug mode.
	 * @param debug_mode True to enable debug mode, false otherwise.
	 * @note Debug mode is slower, but prints more information.
	 */
	void setDebugMode(bool debug_mode);

	/*
	 * @brief Sets the MTU (Maximum Transmission Unit) of the network.
	 * @param MTU The MTU of the network.
	 * @note This value is used to calculate the maximum size of the data in a packet, be careful when changing it.
	 * @attention This value can't be changed if the socket is connected, as the MTU is negotiated and synchronized with the peer.
	 * @attention If the peer has a smaller MTU, the smaller MTU will be used instead for sending data.
	 * @throws `std::runtime_error` if the socket is connected, or if the MTU is smaller than the minimal MTU.
	 */
	void setMTU(uint16_t MTU);
	/*
	 * @brief Sets the timeout.
	 * @param timeout Maximum waiting time for an ACK / SYN-ACK packet in milliseconds.
	 * @note This value is used to calculate the maximum waiting time for an ACK / SYN-ACK packet.
	 * @attention This value can't be changed if the socket is connected.
	 * @throws `std::runtime_error` if the socket is connected, or if the timeout is smaller than the minimal timeout.
	 */
	void setTimeout(uint16_t timeout);

	/*
	 * @brief Sets the maximum number of retries.
	 * @param max_retries The maximum number of retries for a packet, before giving up.
	 * @note This value is used to calculate the maximum number of retries for a packet.
	 * @attention This value can't be changed if the socket is connected.
	 * @throws `std::runtime_error` if the socket is connected.
	 */
	void setMaxRetries(uint16_t max_retries);

	/*
	 * @brief Forces the socket to use its own MTU, instead of the peer's MTU.
	 * @attention This is experimental, as it can cause failures in some cases.
	 * @attention Use this only if you know what you are doing.
	 * @throws `std::runtime_error` if the socket isn't connected.
	 */
	void forceUseOwnMTU();
};