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

#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "include/RUDP_API_wrap.hpp"

uint16_t RUDP_Socket_p::_calculate_checksum(void *data, uint32_t data_size) {
	uint16_t *data_ptr = (uint16_t *)data;
	uint32_t checksum = 0;

	for (uint32_t i = 0; i < data_size / 2; i++)
		checksum += data_ptr[i];

	if (data_size % 2)
		checksum += ((uint8_t *)data)[data_size - 1];

	while (checksum >> 16)
		checksum = (checksum & 0xFFFF) + (checksum >> 16);

	return ~checksum;
}

void RUDP_Socket_p::_send_control_packet(uint8_t flags, uint32_t seq_num) {
	RUDP_header packet;

	packet.flags = flags;
	packet.length = 0;
	packet.seq_num = htonl(seq_num);
	packet.checksum = htons(_calculate_checksum(&packet, sizeof(packet)));

	if (sendto(_socket_fd, &packet, sizeof(packet), 0, (struct sockaddr *)&_dest_addr, sizeof(_dest_addr)) < 0)
		throw std::runtime_error("Failed to send a control packet: " + std::string(strerror(errno)));
}

int RUDP_Socket_p::_check_packet_validity(void *packet, uint32_t packet_size, uint8_t expected_flags) {
	if (packet_size < sizeof(RUDP_header))
	{
		std::cerr << "Packet is too small: " << packet_size << " bytes, while the minimum size is " << sizeof(RUDP_header) << " bytes." << std::endl;
		return 0;
	}

	RUDP_header *header = (RUDP_header *)packet;
	uint16_t checksum = ntohs(header->checksum);
	uint16_t length = ntohs(header->length);
	header->checksum = 0;
	header->checksum = _calculate_checksum(header, packet_size);

	if (length != (packet_size - sizeof(RUDP_header)))
	{
		std::cerr << "Length mismatch: expected " << length << ", got " << (packet_size - sizeof(RUDP_header)) << std::endl;
		return 0;
	}

	if (checksum != header->checksum)
	{
		std::cerr << std::hex << "Checksum mismatch: expected 0x" << (int)checksum << ", got 0x" << (int)header->checksum << std::dec << std::endl;
		return 0;
	}

	if (header->flags == RUDP_FLAG_FIN)
	{
		if (!_is_connected)
		{
			std::cerr << "Received a disconnection request, but there is no active connection." << std::endl;
			return 0;
		}

		std::cout << "Received a disconnection request, closing the connection with " << inet_ntoa(_dest_addr.sin_addr) << ":" << ntohs(_dest_addr.sin_port) << "." << std::endl;
		_send_control_packet(RUDP_FLAG_FIN | RUDP_FLAG_ACK, 0);
		_is_connected = false;
		return -1;
	}

	if (header->flags != expected_flags && (((header->flags & RUDP_FLAG_LAST) == 0) && ((header->flags & RUDP_FLAG_PSH) == 0)))
	{
		std::cerr << "Flags mismatch: expected 0x" << std::hex << (int)expected_flags << ", got 0x" << (int)header->flags << std::dec << std::endl;
		return 0;
	}

	return 1;
}

RUDP_Socket_p::RUDP_Socket_p(bool isServer, uint16_t listen_port, uint16_t MTU, uint16_t timeout, uint16_t max_retries) {
	_is_server = isServer;
	_RUDP_MTU = MTU;
	_RUDP_SOCKET_TIMEOUT = timeout;
	_RUDP_MAX_RETRIES = max_retries;

	// Create a UDP socket
	_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (_socket_fd < 0)
		throw std::runtime_error("Failed to create a socket: " + std::string(strerror(errno)));

	if (_is_server)
	{
		// Bind the socket
		struct sockaddr_in server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(listen_port);

		if (bind(_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		{
			close(_socket_fd);
			throw std::runtime_error("Failed to bind the socket: " + std::string(strerror(errno)));
		}

		// Ensure that the port can be reused
		int enable = 1;

		if (setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		{
			close(_socket_fd);
			throw std::runtime_error("Failed to set the socket option SO_REUSEADDR: " + std::string(strerror(errno)));
		}
	}
}

RUDP_Socket_p::~RUDP_Socket_p() {
	if (_is_connected)
		disconnect();
	
	if (_socket_fd >= 0)
		close(_socket_fd);
}

bool RUDP_Socket_p::connect(const char *dest_ip, uint16_t dest_port) {
	if (_is_server)
	{
		throw std::runtime_error("Server sockets cannot connect to other servers. Use accept() instead.");
		return false;
	}

	else if (_is_connected)
	{
		throw std::runtime_error("There is already an active connection. Use disconnect() to close it.");
		return false;
	}

	_dest_addr.sin_family = AF_INET;
	_dest_addr.sin_port = htons(dest_port);

	if (inet_pton(AF_INET, dest_ip, &_dest_addr.sin_addr) <= 0)
	{
		std::cerr << "Invalid address: " << dest_ip << std::endl;
		return false;
	}

	// Try to connect to the server, up to _RUDP_MAX_RETRIES times
	for (size_t num_of_tries = 0; num_of_tries < _RUDP_MAX_RETRIES; num_of_tries++)
	{
		_send_control_packet(RUDP_FLAG_SYN, 0);

		uint8_t buffer[_RUDP_MTU] = {0};

		pollfd poll_fd[1] = {
			{.fd = _socket_fd, .events = POLLIN, .revents = 0 }
		};

		int ret = poll(poll_fd, 1, _RUDP_SOCKET_TIMEOUT);

		if (ret < 0)
			throw std::runtime_error("Failed to poll the socket: " + std::string(strerror(errno)));

		else if (ret == 0)
		{
			std::cerr << "Warning: Timeout occurred while waiting for a response packet." << std::endl;
			std::cerr << "Retrying connection (" << num_of_tries + 1 << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
			continue;
		}

		int bytes_recv = recvfrom(_socket_fd, buffer, sizeof(buffer), 0, NULL, NULL);

		if (bytes_recv < 0)
		{
			std::cerr << "Warning: Failed to receive a response packet: " << strerror(errno) << std::endl;
			std::cerr << "Retrying connection (" << num_of_tries + 1 << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
			continue;
		}

		int packet_validity = _check_packet_validity(buffer, bytes_recv, RUDP_FLAG_SYN | RUDP_FLAG_ACK);

		switch (packet_validity)
		{
			case 0:
				std::cerr << "Retrying connection (" << num_of_tries + 1 << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
				break;

			case -1:
				return false;

			default:
				_is_connected = true;
				std::cout << "Connection established with " << dest_ip << ":" << dest_port << std::endl;
				return true;
		}
	}

	std::cerr << "Failed to connect to " << dest_ip << ":" << dest_port << std::endl;
	std::cerr << "Please check the server's IP address and port number." << std::endl;

	return false;
}

bool RUDP_Socket_p::accept()
{
	if (!_is_server)
	{
		throw std::runtime_error("Client sockets cannot accept connections. Use connect() instead.");
		return false;
	}

	else if (_is_connected)
	{
		throw std::runtime_error("There is already an active connection. Use disconnect() to close it.");
		return false;
	}

	uint8_t buffer[_RUDP_MTU] = {0};

	// This is the client's address information, filled by recvfrom(). Will be stored in the field _dest_addr for future use.
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	while (true)
	{
		// Receive a connection request packet. This is OK to wait indefinitely here.
		int bytes_recv = recvfrom(_socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);

		// Check if the received packet is valid
		if (bytes_recv < 0)
		{
			std::cerr << "Failed to receive a connection request: " << strerror(errno) << std::endl;
			return false;
		}

		int packet_validity = _check_packet_validity(buffer, bytes_recv, RUDP_FLAG_SYN);

		if (packet_validity == 0)
			continue;

		else if (packet_validity == -1)
			return false;

		_is_connected = true;
		
		// Store the client's address information for future use
		memcpy(&_dest_addr, &client_addr, client_addr_len);
		_send_control_packet(RUDP_FLAG_SYN | RUDP_FLAG_ACK, 0);
		break;
	}

	// Print a message to the console
	std::cout << "Connection established with " << inet_ntoa(_dest_addr.sin_addr) << ":" << ntohs(_dest_addr.sin_port) << std::endl;
	
	return true;
}

int RUDP_Socket_p::recv(void *buffer, uint32_t buffer_size)
{
	if (!_is_connected)
	{
		throw std::runtime_error("There is no active connection to receive data from.");
		return -1;
	}

	uint8_t packet[_RUDP_MTU] = {0};

	uint8_t *buffer_ptr = (uint8_t *)buffer;
	uint32_t total_packets = 0;
	uint32_t total_actual_packets = 0;
	uint32_t prev_seq_num = UINT32_MAX;
	int total_bytes = 0;
	int total_actual_bytes = 0;

	// Handle the first packet separately
	int bytes_recv = recvfrom(_socket_fd, packet, sizeof(packet), 0, NULL, NULL);

	if (bytes_recv < 0)
	{
		throw std::runtime_error("Failed to receive the first packet: " + std::string(strerror(errno)));
		return -1;
	}

	int packet_validity = _check_packet_validity(packet, bytes_recv, RUDP_FLAG_PSH);

	if (packet_validity == 0)
		return -1;

	else if (packet_validity == -1)
		return 0;

	total_bytes += bytes_recv - sizeof(RUDP_header);
	total_packets++;
	total_actual_bytes += bytes_recv;
	total_actual_packets++;
	prev_seq_num = ntohl(((RUDP_header *)packet)->seq_num);

	_send_control_packet(RUDP_FLAG_ACK, prev_seq_num);

	// If the first packet is the last packet, return immediately
	if (((RUDP_header *)packet)->flags & RUDP_FLAG_LAST)
	{
		memcpy(buffer, packet + sizeof(RUDP_header), total_bytes);
		std::cout << "Received " << total_bytes << " bytes in " << total_packets << " packets (Actual overhead: " << total_actual_bytes << " bytes in " << total_actual_packets << " packets)." << std::endl;
		return total_bytes;
	}

	// Wait for the rest of the packets
	while (true)
	{
		for (size_t num_of_tries = 0; num_of_tries <= _RUDP_MAX_RETRIES; num_of_tries++)
		{
			if (num_of_tries == _RUDP_MAX_RETRIES)
			{
				std::cerr << "Failed to receive the packet (" << num_of_tries << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
				return -1;
			}

			memset(packet, 0, sizeof(packet));

			// Wait for a packet, up to _RUDP_MAX_RETRIES times
			pollfd poll_fd[1] = {
				{.fd = _socket_fd, .events = POLLIN, .revents = 0 }
			};

			int ret = poll(poll_fd, 1, _RUDP_SOCKET_TIMEOUT);

			if (ret < 0)
				throw std::runtime_error("Failed to poll the socket: " + std::string(strerror(errno)));

			else if (ret == 0)
			{
				std::cerr << "Warning: Timeout occurred while waiting for a response packet." << std::endl;
				std::cerr << "Retrying to receive the packet (" << num_of_tries + 1 << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
				continue;
			}

			bytes_recv = recvfrom(_socket_fd, packet, sizeof(packet), 0, NULL, NULL);

			if (bytes_recv < 0)
			{
				throw std::runtime_error("Failed to receive a packet: " + std::string(strerror(errno)));
				return -1;
			}

			packet_validity = _check_packet_validity(packet, bytes_recv, RUDP_FLAG_PSH);

			total_actual_bytes += bytes_recv;
			total_actual_packets++;

			if (packet_validity == 0)
				continue;

			else if (packet_validity == -1)
				return 0;

			break;
		}

		// Extract the header information
		RUDP_header *header = (RUDP_header *)packet;
		uint32_t packet_seq_num = ntohl(header->seq_num);
		uint16_t packet_size = ntohs(header->length);
		uint32_t offset = ntohl(header->seq_num) * (_RUDP_MTU - sizeof(RUDP_header));

		// Check if its duplicate packet.
		if (packet_seq_num == prev_seq_num)
		{
			std::cerr << "Received a duplicate packet, ignoring it." << std::endl;
			continue;
		}

		// Check if the packet is out of order
		if (packet_seq_num != prev_seq_num + 1)
		{
			std::cerr << "Received an out-of-order packet, ignoring it." << std::endl;
			continue;
		}

		total_bytes += packet_size;
		total_packets++;
		prev_seq_num = packet_seq_num;

		// Copy the data to the buffer
		memcpy((buffer_ptr + offset), (packet + sizeof(RUDP_header)), packet_size);

		_send_control_packet(RUDP_FLAG_ACK, header->seq_num);

		if ((header->flags & RUDP_FLAG_LAST) != 0 || total_bytes >= ((int32_t)buffer_size))
			break;
	}

	std::cout << "Received " << total_bytes << " bytes in " << total_packets << " packets (Actual overhead: " << total_actual_bytes << " bytes in " << total_actual_packets << " packets)." << std::endl;

	return total_bytes;
}

int RUDP_Socket_p::send(void *buffer, uint32_t buffer_size)
{
	uint8_t *buffer_ptr = (uint8_t *)buffer;
	uint32_t total_packets = 0;
	uint32_t total_actual_packets = 0;
	uint32_t expected_packets = (buffer_size / (_RUDP_MTU - sizeof(RUDP_header))) + 1;
	uint32_t prev_seq_num = UINT32_MAX;
	int total_bytes = 0;
	int total_actual_bytes = 0;

	for (uint32_t i = 0; i < expected_packets; i++)
	{
		uint32_t packet_size = std::min(buffer_size - (uint32_t)total_bytes, (uint32_t)(_RUDP_MTU - sizeof(RUDP_header)));
		//std::cout << "Sending packet " << i + 1 << "/" << expected_packets << " (" << packet_size << " bytes)" << std::endl;
		uint8_t packet[_RUDP_MTU] = {0};

		memcpy(packet + sizeof(RUDP_header), buffer_ptr + (uint32_t)total_bytes, packet_size);

		RUDP_header *header = (RUDP_header *)packet;
		header->flags = (i == expected_packets - 1) ? (RUDP_FLAG_PSH | RUDP_FLAG_LAST) : RUDP_FLAG_PSH;
		header->length = htons(packet_size);
		header->seq_num = htonl(total_packets);
		header->checksum = htons(_calculate_checksum(packet, sizeof(RUDP_header) + packet_size));

		// Wait for an ACK packet, up to _RUDP_MAX_RETRIES times
		for (size_t num_of_tries = 0; num_of_tries <= _RUDP_MAX_RETRIES; num_of_tries++)
		{
			if (num_of_tries == _RUDP_MAX_RETRIES)
			{
				std::cerr << "Failed to send the packet (" << num_of_tries << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
				return -1;
			}

			int bytes_sent = sendto(_socket_fd, packet, sizeof(RUDP_header) + packet_size, 0, (struct sockaddr *)&_dest_addr, sizeof(_dest_addr));
		
			if (bytes_sent < 0)
			{
				throw std::runtime_error("Failed to send a packet: " + std::string(strerror(errno)));
				return -1;
			}

			else if (bytes_sent != (((int)sizeof(RUDP_header)) + ((int)packet_size)))
			{
				std::cerr << "Failed to send the entire packet." << std::endl;
				std::cerr << "Bytes sent: " << bytes_sent << ", expected: " << sizeof(RUDP_header) + packet_size << std::endl;
				return -1;
			}

			total_actual_bytes += bytes_sent;
			total_actual_packets++;

			uint8_t ack_buffer[_RUDP_MTU] = {0};

			pollfd poll_fd[1] = {
				{.fd = _socket_fd, .events = POLLIN, .revents = 0 }
			};

			int ret = poll(poll_fd, 1, _RUDP_SOCKET_TIMEOUT);

			if (ret < 0)
				throw std::runtime_error("Failed to poll the socket: " + std::string(strerror(errno)));

			else if (ret == 0)
			{
				std::cerr << "Warning: Timeout occurred while waiting for a response packet." << std::endl;
				std::cerr << "Retrying to send the packet (" << num_of_tries + 1 << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
				continue;
			}

			int bytes_recv = recvfrom(_socket_fd, ack_buffer, sizeof(ack_buffer), 0, NULL, NULL);

			if (bytes_recv < 0)
			{
				throw std::runtime_error("Failed to receive an ACK packet: " + std::string(strerror(errno)));
				return -1;
			}

			int packet_validity = _check_packet_validity(ack_buffer, bytes_recv, RUDP_FLAG_ACK);

			if (packet_validity == 0)
			{
				std::cerr << "Retrying to send the packet (" << num_of_tries + 1 << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
				continue;
			}

			else if (packet_validity == -1)
				return 0;

			RUDP_header *ack_packet = (RUDP_header *)ack_buffer;
			uint32_t ack_seq_num = ntohl(ack_packet->seq_num);

			// Check for duplicate ACK packets
			if (ack_seq_num == prev_seq_num)
			{
				std::cout << "Warning: Received a duplicate ACK packet, continuing to the next packet." << std::endl;
				break;
			}

			prev_seq_num = ack_seq_num;

			total_bytes += packet_size;
			total_packets++;

			break;
		}
	}

	std::cout << "Sent " << total_bytes << " bytes in " << total_packets << " packets (Actual overhead: " << total_actual_bytes << " bytes in " << total_actual_packets << " packets)." << std::endl;
	return total_bytes;
}

bool RUDP_Socket_p::disconnect()
{
	if (!_is_connected)
	{
		throw std::runtime_error("There is no active connection to close.");
		return false;
	}

	RUDP_header fin_packet;
	fin_packet.flags = RUDP_FLAG_FIN;
	fin_packet.checksum = htons(_calculate_checksum(&fin_packet, sizeof(fin_packet)));

	// Try to disconnect from the server, up to _RUDP_MAX_RETRIES times
	for (size_t num_of_tries = 0; num_of_tries < _RUDP_MAX_RETRIES; num_of_tries++)
	{
		if (sendto(_socket_fd, &fin_packet, sizeof(fin_packet), 0, (struct sockaddr *)&_dest_addr, sizeof(_dest_addr)) < 0)
		{
			throw std::runtime_error("Failed to send a disconnection request: " + std::string(strerror(errno)));
			return false;
		}

		uint8_t buffer[_RUDP_MTU] = {0};

		int bytes_recv = recvfrom(_socket_fd, buffer, sizeof(buffer), 0, NULL, NULL);

		if (bytes_recv < 0)
		{
			throw std::runtime_error("Failed to receive a response packet: " + std::string(strerror(errno)));
			return false;
		}

		else if (bytes_recv != sizeof(RUDP_header))
		{
			std::cerr << "Warning: Received an incomplete response packet, ignoring it." << std::endl;
			std::cerr << "Retrying disconnection (" << num_of_tries + 1 << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
			continue;
		}

		RUDP_header response_packet;
		memcpy(&response_packet, buffer, sizeof(response_packet));

		uint16_t checksum = response_packet.checksum;
		response_packet.checksum = 0;

		if (checksum != htons(_calculate_checksum(&response_packet, sizeof(response_packet))) || response_packet.flags != (RUDP_FLAG_FIN | RUDP_FLAG_ACK) || response_packet.length != 0)
		{
			std::cerr << "Warning: Received a corrupted response packet, ignoring it." << std::endl;
			std::cerr << "Retrying disconnection (" << num_of_tries + 1 << "/" << _RUDP_MAX_RETRIES << ")" << std::endl;
			continue;
		}

		_is_connected = false;
		std::cout << "Connection closed with " << inet_ntoa(_dest_addr.sin_addr) << ":" << ntohs(_dest_addr.sin_port) << std::endl;
		memset(&_dest_addr, 0, sizeof(_dest_addr)); // Clear the destination address
		return true;
	}

	std::cerr << "Failed to disconnect from " << inet_ntoa(_dest_addr.sin_addr) << ":" << ntohs(_dest_addr.sin_port) << std::endl;
	std::cerr << "Assuming that the connection is closed." << std::endl;

	_is_connected = false;
	memset(&_dest_addr, 0, sizeof(_dest_addr)); // Clear the destination address

	return true;
}