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
#include <cstring>
#include <string>
#include <stdexcept>

#if defined(_WIN32) || defined(_WIN64) // Windows NT (not Windows 9x)

// Windows specific includes.
#define _OPSYS_WINDOWS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// Compatibility with Unix types, constants and functions.
#define poll WSAPoll

// Unix-like error handling for Winsock2, strerror_r is not available in Windows.
#define strerror_r(errnum, buf, buflen) FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errnum, 0, buf, buflen, NULL)

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__) // Linux, Unix, MacOS

// Unix specific includes.
#define _OPSYS_UNIX

#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>

// Compatibility with Windows types, constants and functions.

typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close

#else // No appropriate platform detected, throw an error
#error "Unsupported platform detected, please refer to the documentation for more information."
#endif

// Error message buffer size.
#define _ERROR_MSG_BUFFER_SIZE 1024

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

/*
 * @brief The minimal MTU (Maximum Transmission Unit) of the network.
 */
#define RUDP_MINIMAL_MTU (sizeof(RUDP_header) + sizeof(RUDP_SYN_packet))

/*
 * @brief The minimal timeout.
 */
#define RUDP_MINIMAL_TIMEOUT 10

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

/*
 * @brief The RUDP header.
 * @param seq_num Sequence number of the packet (used for tracking which packet is which).
 * @param length Length of the data in bytes, without the header.
 * @param checksum Checksum of the packet, including the header.
 * @param flags Flags of the packet.
 * @param _reserved Reserved for future use. Currently is set to 0.
 * @note This is the header of the RUDP packet, it is 12 bytes long.
 * @attention This is for internal use only, manipulating this directly can cause undefined behavior for the library.
 */
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
/*
 * @brief The RUDP SYN packet.
 * @param MTU Maximum Transmission Unit (MTU) of the network.
 * @param timeout Maximum waiting time for an ACK / SYN-ACK packet in milliseconds.
 * @param max_retries The maximum number of retries for a packet, before giving up.
 * @param debug_mode Debug mode.
 * @note This is the SYN packet that is sent when a connection is being established, to inform the other side about the connection parameters and settings.
 * @attention This is for internal use only, manipulating this directly can cause undefined behavior for the library.
 */
typedef struct _RUDP_SYN_packet
{
	uint16_t MTU = RUDP_MTU_DEFAULT;
	uint16_t timeout = RUDP_SOCKET_TIMEOUT_DEFAULT;
	uint16_t max_retries = RUDP_MAX_RETRIES_DEFAULT;
	uint16_t debug_mode = 0;
} RUDP_SYN_packet;
/*
 * @brief A class that represents a Reliable UDP socket.
 * @attention Only use this internally in the library. For external usage, use the wrapper class RUDP_Socket (C++) or RUDP_socket (C).
 */
class RUDP_Socket_p
{
private:
	/*
	 * @brief UDP socket file descriptor
	 */
	SOCKET m_socketHandle = INVALID_SOCKET;

	/*
	 * @brief True if the RUDP socket acts like a server, false for client.
	 */
	bool m_isServer = false;

	/*
	 * @brief True if there is an active connection, false otherwise.
	 */
	bool m_isConnected = false;

	/*
	 * @brief True for debug mode (slower), false for normal mode.
	 */
	bool m_debugMode = false;

	/*
	 * @brief Destination address (IPv4).
	 * @note Client fills it when it connects via rudp_connect(), server fills it when it accepts a connection via rudp_accept().
	 */
	struct sockaddr_in m_destinationAddress4;

	/*
	 * @brief The MTU (Maximum Transmission Unit) of the network.
	 * @attention This value is used to calculate the maximum size of the data in a packet, be careful when changing it.
	 */
	uint16_t m_protocolMTU;

	/*
	 * @brief Maximum waiting time for an ACK / SYN-ACK packet in milliseconds.
	 */
	uint16_t m_protocolTimeout;

	/*
	 * @brief The maximum number of retries for a packet, before giving up.
	 */
	uint16_t m_protocolMaximumRetries;

	/*
	 * @brief The MTU of the peer, in case the peer has a smaller MTU.
	 */
	uint16_t m_peersMTU = 0;

private:
	/*
	 * @brief A checksum function that returns 16 bit checksum for data.
	 * @param data The data to do the checksum for.
	 * @param bytes The length of the data in bytes.
	 * @return The checksum itself as 16 bit unsigned number.
	 * @attention This is an internal method, its not exposed to the user.
	 */
	static uint16_t _calculate_checksum(void *data, uint32_t data_size);

	/*
	 * @brief Checks if the packet source is valid (connected peer).
	 * @param source The source address of the packet.
	 * @param source_size Size of the source address.
	 * @return 0 if the source is valid, non-zero if the source is invalid (also sends a FIN packet if the source is invalid).
	*/
	bool _check_packet_source(struct sockaddr *source, uint32_t source_size);

	/*
	 * @brief Prints a socket error message.
	 * @param message The message to be printed.
	 * @param throw_exception True to throw an exception, false otherwise.
	 * @attention This is an internal method, its not exposed to the user.
	 */
	void _print_socket_error(std::string message, bool throw_exception = false);

	/*
	 * @brief Sends a control packet (SYN, ACK, FIN) to the connected peer.
	 * @param flags Flags to be set in the control packet.
	 * @param seq_num Sequence number of the control packet.
	 * @param destination Destination address. Use nullptr for the connected peer (if the socket is connected).
	 * @param destination_size Size of the destination address. Ignored if destination is nullptr.
	 * @note This function doesn't actually check if the packet is received by the peer.
	 * @attention This is an internal method, its not exposed to the user.
	 */
	void _send_control_packet(uint8_t flags, uint32_t seq_num, struct sockaddr *destination, uint32_t destination_size);

	/*
	 * @brief Checks if the packet is valid.
	 * @param packet The packet to be checked.
	 * @param packet_size Size of the packet in bytes.
	 * @param expected_flags Expected flags in the packet.
	 * @return 1 if the packet is valid, 0 if not, -1 if the packet if a FIN packet.
	 * @attention This is an internal method, its not exposed to the user.
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
	 * @param debug_mode True to enable debug mode, false otherwise. Default is false.
	 * @note If the socket is a server, it will listen on the specified port.
	 * @throws `std::runtime_error` if the socket creation fails, or if bind() fails.
	 */
	RUDP_Socket_p(bool isServer, uint16_t listen_port, uint16_t MTU = RUDP_MTU_DEFAULT, uint16_t timeout = RUDP_SOCKET_TIMEOUT_DEFAULT, uint16_t max_retries = RUDP_MAX_RETRIES_DEFAULT, bool debug_mode = false);

	/*
	 * @brief Destructor.
	 * @note Closes the socket if it is still open and disconnects if it is still connected.
	 */
	~RUDP_Socket_p();

public:
	/*
	 * @brief Connects to a server.
	 * @param dest_ip Destination IP address.
	 * @param dest_port Destination port.
	 * @return True if the connection is successful, false otherwise.
	 * @throws `std::runtime_error` if the socket is a server.
	 */
	bool connect(const char *dest_ip, uint16_t dest_port);

	/*
	 * @brief Accepts a connection from a client.
	 * @return True if the connection is successful, false otherwise.
	 * @throws `std::runtime_error` if the socket is a client.
	 */
	bool accept();

	/*
	 * @brief Receives data from the connected peer.
	 * @param buffer Buffer to store the received data.
	 * @param buffer_size Size of the buffer.
	 * @return Number of bytes received.
	 * @throws `std::runtime_error` if the socket is not connected.
	 */
	int recv(void *buffer, uint32_t buffer_size);

	/*
	 * @brief Sends data to the connected peer.
	 * @param buffer Buffer containing the data to be sent.
	 * @param buffer_size Size of the buffer.
	 * @return Number of bytes sent.
	 * @throws `std::runtime_error` if the socket is not connected.
	 */
	int send(void *buffer, uint32_t buffer_size);

	/*
	 * @brief Disconnects from the connected peer.
	 * @return True if the disconnection is successful, false otherwise.
	 */
	bool disconnect();

/* Getters */
public:
	/*
	 * @brief Gets the MTU (Maximum Transmission Unit) of the network.
	 * @return The MTU of the network.
	 */
	uint16_t getMTU() const { return m_protocolMTU; }

	/*
	 * @brief Gets the timeout.
	 * @return Maximum waiting time for an ACK / SYN-ACK packet in milliseconds.
	 */
	uint16_t getTimeout() const { return m_protocolTimeout; }

	/*
	 * @brief Gets the maximum number of retries.
	 * @return The maximum number of retries for a packet, before giving up.
	 */
	uint16_t getMaxRetries() const { return m_protocolMaximumRetries; }

	/*
	 * @brief Gets the MTU of the peer.
	 * @return The MTU of the peer, in case the peer has a smaller MTU.
	 * @throws `std::runtime_error` if the socket isn't connected.
	 */
	uint16_t getPeersMTU() const { 
		if (!m_isConnected) throw std::runtime_error("Can't get the peer's MTU if the socket is not connected. Please make a connection with the peer first.");
		return m_peersMTU;
	}

	/*
	 * @brief Checks if the socket is in debug mode.
	 * @return True if the socket is in debug mode, false otherwise.
	 */
	bool isDebugMode() const { return m_debugMode; }

	/*
	 * @brief Checks if the socket is connected.
	 * @return True if the socket is connected, false otherwise.
	 */
	bool isConnected() const { return m_isConnected; }

	/*
	 * @brief Checks if the socket is a server.
	 * @return True if the socket is a server, false otherwise.
	 */
	bool isServer() const { return m_isServer; }

/* Setters */
public:
	/*
	 * @brief Sets the debug mode.
	 * @param debug_mode True to enable debug mode, false otherwise.
	 * @note Debug mode is slower, but prints more information.
	*/
	void setDebugMode(bool debug_mode) { m_debugMode = debug_mode; }

	/*
	 * @brief Sets the MTU (Maximum Transmission Unit) of the network.
	 * @param MTU The MTU of the network.
	 * @note This value is used to calculate the maximum size of the data in a packet, be careful when changing it.
	 * @attention This value can't be changed if the socket is connected, as the MTU is negotiated and synchronized with the peer.
	 * @attention If the peer has a smaller MTU, the smaller MTU will be used instead for sending data.
	 * @throws `std::runtime_error` if the socket is connected, or if the MTU is smaller than the minimal MTU.
	*/
	void setMTU(uint16_t MTU) { 
		if (m_isConnected) throw std::runtime_error("Can't change the MTU while connected. Use disconnect() first.");
		if (MTU < RUDP_MINIMAL_MTU) throw std::runtime_error("MTU can't be smaller than the minimal MTU, which is " + std::to_string(RUDP_MINIMAL_MTU) + " bytes.");
		m_protocolMTU = MTU;
	}

	/*
	 * @brief Sets the timeout.
	 * @param timeout Maximum waiting time for an ACK / SYN-ACK packet in milliseconds.
	 * @note This value is used to calculate the maximum waiting time for an ACK / SYN-ACK packet.
	 * @attention This value can't be changed if the socket is connected.
	 * @throws `std::runtime_error` if the socket is connected, or if the timeout is smaller than the minimal timeout.
	*/
	void setTimeout(uint16_t timeout) { 
		if (timeout < RUDP_MINIMAL_TIMEOUT) throw std::runtime_error("Timeout can't be smaller than the minimal timeout, which is " + std::to_string(RUDP_MINIMAL_TIMEOUT) + " milliseconds.");
		m_protocolTimeout = timeout;
	}

	/*
	 * @brief Sets the maximum number of retries.
	 * @param max_retries The maximum number of retries for a packet, before giving up.
	 * @note This value is used to calculate the maximum number of retries for a packet.
	 * @attention This value can't be changed if the socket is connected.
	 * @throws `std::runtime_error` if the socket is connected.
	*/
	void setMaxRetries(uint16_t max_retries) { 
		if (max_retries < 1) throw std::runtime_error("Maximum number of retries can't be smaller than 1, as it will cause the packet to be sent only once and then give up.");
		m_protocolMaximumRetries = max_retries;
	}

	/*
	 * @brief Forces the socket to use its own MTU, instead of the peer's MTU.
	 * @attention This is experimental, as it can cause failures in some cases.
	 * @attention Use this only if you know what you are doing.
	 * @throws `std::runtime_error` if the socket isn't connected.
	*/
	void forceUseOwnMTU() {
		if (!m_isConnected) throw std::runtime_error("Can't force the socket to use its own MTU over the peer's MTU if the socket is not connected. Please make a connection with the peer first.");
		m_peersMTU = m_protocolMTU;
	}
};