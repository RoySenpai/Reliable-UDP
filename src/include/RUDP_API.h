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

#ifndef _RUDP_API_H
#define _RUDP_API_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <stdbool.h>
#include <stdint.h>

	/*
	* @brief This represents a RUDP socket.
	*/
	typedef void* RUDP_socket;

	/*
	 * @brief Create a new RUDP socket.
	 * @param isServer True if the RUDP socket acts like a server, false for client.
	 * @param listen_port The port number to listen on, if the socket is a server. Ignored if the socket is a client.
	 * @param MTU The Maximum Transmission Unit of the network, default is RUDP_MTU_DEFAULT which is 1458 bytes.
	 * @param timeout Maximum waiting time for an ACK / SYN-ACK packet in milliseconds, default is RUDP_SOCKET_TIMEOUT_DEFAULT which is 100 milliseconds.
	 * @param max_retries The maximum number of retries for a packet, before giving up, default is RUDP_MAX_RETRIES_DEFAULT which is 50 retries.
	 * @param debug_mode True to enable debug mode, false otherwise. Default is false.
	 * @note If the socket is a server, it will listen on the specified port.
	 * @return A pointer to the RUDP socket, or NULL if the socket creation fails (also prints an error message).
	 */
	RUDP_socket rudp_socket(bool isServer, uint16_t listen_port, uint16_t MTU, uint16_t timeout, uint16_t max_retries, bool debug_mode);

	/*
	 * @brief Connect to a remote RUDP server.
	 * @param socket The RUDP socket to connect with.
	 * @param dest_ip The IP address of the remote server.
	 * @param dest_port The port number of the remote server.
	 * @return True if the connection is successful, false otherwise (also prints an error message).
	 */
	bool rudp_connect(RUDP_socket socket, const char *dest_ip, uint16_t dest_port);

	/*
	 * @brief Accept an incoming connection from a remote RUDP client.
	 * @param socket The RUDP socket to accept the connection.
	 * @return True if the connection is successful, false otherwise (also prints an error message).
	 */
	bool rudp_accept(RUDP_socket socket);

	/*
	 * @brief Receive data from the connected peer.
	 * @param socket The RUDP socket to receive data from.
	 * @param buffer Buffer to store the received data.
	 * @param buffer_size Size of the buffer.
	 * @return Number of bytes received or -1 if an error occurs (also prints an error message).
	 */
	int rudp_recv(RUDP_socket socket, void *buffer, uint32_t buffer_size);

	/*
	 * @brief Send data to the connected peer.
	 * @param socket The RUDP socket to send data to.
	 * @param buffer Buffer containing the data to be sent.
	 * @param buffer_size Size of the buffer.
	 * @return Number of bytes sent or -1 if an error occurs (also prints an error message).
	 */
	int rudp_send(RUDP_socket socket, void *buffer, uint32_t buffer_size);

	/*
	 * @brief Disconnect from the connected peer.
	 * @param socket The RUDP socket to disconnect.
	 * @return True if the disconnection is successful, false otherwise (also prints an error message).
	 */
	bool rudp_disconnect(RUDP_socket socket);

	/*
	 * @brief Free the RUDP socket and release the resources.
	 * @param socket A pointer to the RUDP socket to free.
	 * @note This function should be called after the socket is no longer needed.
	 * @attention This function resets the pointer back to NULL, so no further operations can be performed on the socket.
	 * @return None.
	 */
	void rudp_socket_free(RUDP_socket *socket);


	/*
	 * @brief Gets the MTU (Maximum Transmission Unit) of the network.
	 * @return The MTU of the network, or 0 if the socket is invalid.
	 */
	uint16_t rudp_get_mtu(RUDP_socket socket);

	/*
	 * @brief Gets the timeout.
	 * @return Maximum waiting time for an ACK / SYN-ACK packet in milliseconds, or 0 if the socket is invalid.
	 */
	uint16_t rudp_get_timeout(RUDP_socket socket);

	/*
	 * @brief Gets the maximum number of retries.
	 * @return The maximum number of retries for a packet, before giving up, or 0 if the socket is invalid.
	 */
	uint16_t rudp_get_maxretries(RUDP_socket socket);

	/*
	 * @brief Gets the MTU of the peer.
	 * @return The MTU of the peer, in case the peer has a smaller MTU, or 0 on error.
	 */
	uint16_t rudp_get_peers_MTU(RUDP_socket socket);

	/*
	 * @brief Checks if the socket is in debug mode.
	 * @return True if the socket is in debug mode, false otherwise.
	 */
	bool rudp_is_debug_mode(RUDP_socket socket);

	/*
	 * @brief Checks if the socket is connected.
	 * @return True if the socket is connected, false otherwise.
	 */
	bool rudp_is_connected(RUDP_socket socket);

	/*
	 * @brief Checks if the socket is a server.
	 * @return True if the socket is a server, false otherwise.
	 */
	bool rudp_is_server(RUDP_socket socket);

	/*
	 * @brief Sets the debug mode.
	 * @param debug_mode True to enable debug mode, false otherwise.
	 * @note Debug mode is slower, but prints more information.
	 */
	void rudp_set_debug_mode(RUDP_socket socket, bool debug_mode);

	/*
	 * @brief Sets the MTU (Maximum Transmission Unit) of the network.
	 * @param MTU The MTU of the network.
	 * @note This value is used to calculate the maximum size of the data in a packet, be careful when changing it.
	 * @attention This value can't be changed if the socket is connected, as the MTU is negotiated and synchronized with the peer.
	 * @attention If the peer has a smaller MTU, the smaller MTU will be used instead for sending data.
	 */
	void rudp_set_MTU(RUDP_socket socket, uint16_t MTU);

	/*
	 * @brief Sets the timeout.
	 * @param timeout Maximum waiting time for an ACK / SYN-ACK packet in milliseconds.
	 * @note This value is used to calculate the maximum waiting time for an ACK / SYN-ACK packet.
	 * @attention This value can't be changed if the socket is connected.
	 */
	void rudp_set_timeout(RUDP_socket socket, uint16_t timeout);

	/*
	 * @brief Sets the maximum number of retries.
	 * @param max_retries The maximum number of retries for a packet, before giving up.
	 * @note This value is used to calculate the maximum number of retries for a packet.
	 * @attention This value can't be changed if the socket is connected.
	 */
	void rudp_set_max_retries(RUDP_socket socket, uint16_t max_retries);

	/*
	 * @brief Forces the socket to use its own MTU, instead of the peer's MTU.
	 * @attention This is experimental, as it can cause failures in some cases.
	 * @attention Use this only if you know what you are doing.
	 */
	void rudp_force_use_own_MTU(RUDP_socket socket);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _RUDP_API_H
