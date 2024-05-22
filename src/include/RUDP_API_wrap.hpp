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
#include <netinet/in.h>

/*
 * @brief The MTU (Maximum Transmission Unit) of the network, default is 1500 bytes.
 */
#define RUDP_MTU_DEFAULT 1500

/*
 * @brief Maximum waiting time for an ACK / SYN-ACK packet in milliseconds, default is 100 milliseconds.
*/
#define RUDP_SOCKET_TIMEOUT_DEFAULT 100

/*
 * @brief The maximum number of retries for a packet, before giving up, default is 50 retries.
 */
#define RUDP_MAX_RETRIES_DEFAULT 50


/* Flags for Reliable UDP Protocol */

/*
 * @brief The SYN flag - connection is being established.
 */
#define RUDP_FLAG_SYN 0x01

/*
 * @brief The ACK flag - acknowledgement of data.
 */
#define RUDP_FLAG_ACK 0x02

/*
 * @brief The PSH flag - data is pushed to the application.
 */
#define RUDP_FLAG_PSH 0x04

/*
 * @brief The LAST flag - this is the last packet of the message.
 */
#define RUDP_FLAG_LAST 0x08

/*
 * @brief The FIN flag - connection is closing.
 */
#define RUDP_FLAG_FIN 0x10

struct RUDP_header
{
	/*
	 * @brief Sequence number field.
	 * @note Sequence number of the packet.
	*/
	uint32_t seq_num = 0;

	/*
	 * @brief Length field.
	 * @note Length of the data in bytes.
	*/
	uint16_t length = 0;

	/*
	 * @brief Checksum field.
	 * @note Checksum is calculated for the entire packet, including the header.
	*/
	uint16_t checksum = 0;

	/*
	 * @brief Flags field.
	 * @note Possible flags:
	 * @note RUDP_FLAG_SYN - connection is being established.
	 * @note RUDP_FLAG_ACK - acknowledgement of data.
	 * @note RUDP_FLAG_PSH - data is pushed to the application.
	 * @note RUDP_FLAG_LAST - this is the last packet of the message.
	 * @note RUDP_FLAG_FIN - connection is closing.
	*/
	uint8_t flags = 0;

	/*
	 * @brief Reserved field.
	 * @note 3 byte reserved for future use.
	 * @note Must be set to 0.
	*/
	uint8_t _reserved[3] = {0};
};

class RUDP_Socket_p
{
	private:
		/*
		* @brief UDP socket file descriptor
		*/
		int _socket_fd = -1;

		/*
		* @brief True if the RUDP socket acts like a server, false for client.
		*/
		bool _is_server = false;

		/*
		* @brief True if there is an active connection, false otherwise.
		*/
		bool _is_connected = false;

		/*
		* @brief Destination address.
		* @note Client fills it when it connects via rudp_connect(), server fills it when it accepts a connection via rudp_accept().
		*/
		struct sockaddr_in _dest_addr;

		/*
		 * @brief The MTU (Maximum Transmission Unit) of the network.
		 * @attention This value is used to calculate the maximum size of the data in a packet, be careful when changing it.
		 */
		uint16_t _RUDP_MTU;

		/*
		* @brief Maximum waiting time for an ACK / SYN-ACK packet in milliseconds.
		*/
		uint16_t _RUDP_SOCKET_TIMEOUT;

		/*
		* @brief The maximum number of retries for a packet, before giving up.
		*/
		uint16_t _RUDP_MAX_RETRIES;

		/*
		* @brief A checksum function that returns 16 bit checksum for data.
		* @param data The data to do the checksum for.
		* @param bytes The length of the data in bytes.
		* @return The checksum itself as 16 bit unsigned number.
		*/
		uint16_t _calculate_checksum(void *data, uint32_t data_size);

		/*
		* @brief Sends a control packet (SYN, ACK, FIN) to the connected peer.
		* @param flags Flags to be set in the control packet.
		* @param seq_num Sequence number of the control packet.
		* @note This function doesn't actually check if the packet is received by the peer.
		*/
		void _send_control_packet(uint8_t flags, uint32_t seq_num);

		/*
		* @brief Checks if the packet is valid.
		* @param packet The packet to be checked.
		* @param packet_size Size of the packet in bytes.
		* @param expected_flags Expected flags in the packet.
		* @return 1 if the packet is valid, 0 if not, -1 if the packet if a FIN packet.
		*/
		int _check_packet_validity(void *packet, uint32_t packet_size, uint8_t expected_flags);

	public:
		/*
		* @brief Creates a new RUDP socket.
		* @param isServer True if the RUDP socket acts like a server, false for client.
		* @param listen_port Port to listen on if the socket is a server. Ignored if the socket is a client.
		* @param MTU The MTU (Maximum Transmission Unit) of the network, default is 1500 bytes.
		* @param timeout Maximum waiting time for an ACK / SYN-ACK packet in milliseconds, default is 100 milliseconds.
		* @param max_retries The maximum number of retries for a packet, before giving up, default is 50 retries.
		* @note If the socket is a server, it will listen on the specified port.
		*/
		RUDP_Socket_p(bool isServer, uint16_t listen_port, uint16_t MTU = RUDP_MTU_DEFAULT, uint16_t timeout = RUDP_SOCKET_TIMEOUT_DEFAULT, uint16_t max_retries = RUDP_MAX_RETRIES_DEFAULT);

		/*
		* @brief Destructor.
		* @note Closes the socket if it is still open and disconnects if it is still connected.
		*/
		~RUDP_Socket_p();

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