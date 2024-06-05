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
#include <iomanip>
#include <cstdlib>
#include <errno.h>
#include "include/RUDP_API_wrap.hpp"

uint16_t RUDP_Socket_p::_calculate_checksum(void *data, uint32_t data_size) {
	uint16_t *data_ptr = (uint16_t *)data;
	uint32_t checksum = 0;

	for (uint32_t i = 0; i < data_size / 2; i++) checksum += data_ptr[i];
	if (data_size % 2) checksum += ((uint8_t *)data)[data_size - 1];
	while (checksum >> 16) checksum = (checksum & 0xFFFF) + (checksum >> 16);

	return ~checksum;
}

bool RUDP_Socket_p::_check_packet_source(struct sockaddr *source, uint32_t source_size) {
	int result = memcmp(source, (struct sockaddr *)&m_destinationAddress4, source_size);
	if (result != 0)
	{
		if (m_debugMode) std::cerr << "Warning: Received a packet from an unknown source address (" << inet_ntoa(((struct sockaddr_in *)source)->sin_addr) << ":" << ntohs(((struct sockaddr_in *)source)->sin_port) << "), sending a rejection packet (FIN)." << std::endl;
		_send_control_packet(RUDP_FLAG_FIN, 0, source, source_size);
	}
	return result;
}

void RUDP_Socket_p::_send_control_packet(uint8_t flags, uint32_t seq_num, struct sockaddr *destination, uint32_t destination_size) {
	uint8_t packet[m_protocolMTU] = {0};
	RUDP_header header = {
		.seq_num = htonl(seq_num),
		.flags = flags
	};

	if ((flags & RUDP_FLAG_SYN) != RUDP_FLAG_SYN) header.checksum = htons(RUDP_Socket_p::_calculate_checksum(&header, sizeof(header)));
	else {
		header.length = htons(sizeof(RUDP_SYN_packet));
		RUDP_SYN_packet syn_packet = {
			.MTU = htons(m_protocolMTU),
			.timeout = htons(m_protocolTimeout),
			.max_retries = htons(m_protocolMaximumRetries),
			.debug_mode = htons(m_debugMode)
		};
		memcpy(packet + sizeof(header), &syn_packet, sizeof(RUDP_SYN_packet));
		memcpy(packet, &header, sizeof(header));
		header.checksum = htons(RUDP_Socket_p::_calculate_checksum(&packet, sizeof(header) + sizeof(RUDP_SYN_packet)));
	}

	memcpy(packet, &header, sizeof(header));

	if (destination == nullptr)
	{
		destination = (struct sockaddr *)&m_destinationAddress4;
		destination_size = sizeof(m_destinationAddress4);
	}

	if (sendto(m_socketHandle, (char *)(&packet), (ntohs(header.length) + sizeof(header)), 0, destination, destination_size) == SOCKET_ERROR) _print_socket_error("Failed to send a control packet", false);
}

int RUDP_Socket_p::_check_packet_validity(void *packet, uint32_t packet_size, uint8_t expected_flags) {
	static const std::string flag_names[] = {
		"Syncronization (SYN)",
		"Acknowledgement (ACK)",
		"Push (PSH)",
		"Last (LAST)",
		"Closure (FIN)"
	};

	if (packet_size < sizeof(RUDP_header))
	{
		if (m_debugMode)
		{
			std::cerr << "Packet validity error:" << std::endl;
			std::cerr << "\tPacket size: " << packet_size << " bytes" << std::endl;
			std::cerr << "\tMinimum packet size: " << sizeof(RUDP_header) << " bytes" << std::endl;
		}

		return 0;
	}

	RUDP_header *header = (RUDP_header *)packet;
	uint16_t checksum = ntohs(header->checksum);
	uint16_t length = ntohs(header->length);
	header->checksum = 0;
	header->checksum = RUDP_Socket_p::_calculate_checksum(header, packet_size);

	if (length != (packet_size - sizeof(RUDP_header)))
	{
		if (m_debugMode)
		{
			std::cerr << "Packet validity error:" << std::endl;
			std::cerr << "\tPacket length: " << length << " bytes" << std::endl;
			std::cerr << "\tActual packet length: " << (packet_size - sizeof(RUDP_header)) << " bytes" << std::endl;
		}
		
		return 0;
	}

	if (checksum != header->checksum)
	{
		if (m_debugMode)
		{
			std::cerr << "Packet validity error:" << std::endl;
			std::cerr << std::setw(2) << std::setfill('0') << std::hex << std::showbase << "\tExpected checksum: " << checksum << std::noshowbase << std::dec << std::endl;
			std::cerr << std::setw(2) << std::setfill('0') << std::hex << std::showbase << "\tReceived checksum: " << header->checksum << std::noshowbase << std::dec << std::endl;
		}
		
		return 0;
	}

	if (header->flags == RUDP_FLAG_FIN && (expected_flags != RUDP_FLAG_FIN && expected_flags != (RUDP_FLAG_FIN | RUDP_FLAG_ACK)))
	{
		if (!m_isConnected)
		{
			if (expected_flags & RUDP_FLAG_SYN)
			{
				if (m_debugMode) std::cerr << "Error: Connection was forcibly closed by the peer." << std::endl;
				return -1;
			}

			if (m_debugMode)
			{
				std::cerr << "Packet validity error:" << std::endl;
				std::cerr << "\tReceived a disconnection request, but there is no active connection." << std::endl;
			}

			return 0;
		}

		if (m_debugMode) std::cout << "Received a disconnection request, closing the connection with " << inet_ntoa(m_destinationAddress4.sin_addr) << ":" << ntohs(m_destinationAddress4.sin_port) << "." << std::endl;
		_send_control_packet(RUDP_FLAG_FIN | RUDP_FLAG_ACK, 0, nullptr, 0);
		m_isConnected = false;
		return -1;
	}

	if (expected_flags && (header->flags != expected_flags) && (((header->flags & RUDP_FLAG_LAST) == 0) && ((header->flags & RUDP_FLAG_PSH) == 0)))
	{
		if (m_debugMode)
		{
			std::string expected_flags_str, received_flags_str;

			for (size_t i = 0; i < 5; i++)
			{
				if (expected_flags & (1 << i)) expected_flags_str += flag_names[i] + ", ";
				if (header->flags & (1 << i)) received_flags_str += flag_names[i] + ", ";
			}

			expected_flags_str.pop_back();
			expected_flags_str.pop_back();
			received_flags_str.pop_back();
			received_flags_str.pop_back();

			std::cerr << "Packet validity error:" << std::endl;
			std::cerr << "\tExpected flags: " << expected_flags_str << std::endl;
			std::cerr << "\tReceived flags: " << received_flags_str << std::endl;
		}
		
		return 0;
	}

	return 1;
}

void RUDP_Socket_p::_print_socket_error(std::string message, bool throw_exception) {
	char err_buf[_ERROR_MSG_BUFFER_SIZE] = {0};

#if defined(_OPSYS_WINDOWS)
	int last_error = WSAGetLastError();
#elif defined(_OPSYS_UNIX)
	int last_error = errno;
#endif

	strerror_r(last_error, err_buf, sizeof(err_buf));

	if (throw_exception) throw std::runtime_error(message + ": " + err_buf);
	else std::cerr << message << ": " << err_buf << std::endl;
}

RUDP_Socket_p::RUDP_Socket_p(bool isServer, uint16_t listen_port, uint16_t MTU, uint16_t timeout, uint16_t max_retries, bool debug_mode) {
	m_isServer = isServer;
	m_protocolMTU = MTU;
	m_protocolTimeout = timeout;
	m_protocolMaximumRetries = max_retries;
	m_debugMode = debug_mode;

	if (m_protocolMTU < (RUDP_MINIMAL_MTU)) throw std::runtime_error("Invalid MTU: " + std::to_string(m_protocolMTU) + " bytes, the minimum MTU is " + std::to_string(RUDP_MINIMAL_MTU) + " bytes. Please reajust the MTU value.");
	if (m_protocolTimeout < RUDP_MINIMAL_TIMEOUT) throw std::runtime_error("Invalid timeout: " + std::to_string(m_protocolTimeout) + " milliseconds, the minimum timeout is " + std::to_string(RUDP_MINIMAL_TIMEOUT) + " milliseconds.");
	if (m_protocolMaximumRetries == 0) throw std::runtime_error("Invalid maximum number of retries: " + std::to_string(m_protocolMaximumRetries) + ", the minimum number of retries is 1.");

#ifdef _OPSYS_WINDOWS
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) _print_socket_error("Failed to initialize Winsock2", true);
#endif

	if ((m_socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) _print_socket_error("Failed to create a socket", true);

	if (m_isServer)
	{
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(listen_port);

		int enable = 1;

		if (bind(m_socketHandle, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR || 
			setsockopt(m_socketHandle, SOL_SOCKET, SO_REUSEADDR, (const char *)&enable, sizeof(int)) == SOCKET_ERROR)
			_print_socket_error("Socket faliure", true);

	}
}

RUDP_Socket_p::~RUDP_Socket_p() {
	if (m_isConnected) disconnect();
	
	if (m_socketHandle != INVALID_SOCKET)
	{
		closesocket(m_socketHandle);
#if defined(_OPSYS_WINDOWS)
		WSACleanup();
#endif
	}
}

bool RUDP_Socket_p::connect(const char *dest_ip, uint16_t dest_port) {
	if (m_isServer) throw std::runtime_error("Server sockets cannot connect to other servers. Use accept() instead.");
	if (m_isConnected) throw std::runtime_error("There is already an active connection. Use disconnect() to close it.");

	memset(&m_destinationAddress4, 0, sizeof(m_destinationAddress4));
	m_destinationAddress4.sin_family = AF_INET;
	m_destinationAddress4.sin_port = htons(dest_port);

	if (inet_pton(AF_INET, dest_ip, &m_destinationAddress4.sin_addr) <= 0) _print_socket_error("Failed to convert the IP address", true);

	uint8_t buffer[m_protocolMTU] = {0};

	struct sockaddr_in server_addr;
	size_t server_addr_len = sizeof(server_addr);

	for (size_t num_of_tries = 0; num_of_tries < m_protocolMaximumRetries; num_of_tries++)
	{
		memset(buffer, 0, sizeof(buffer));
		_send_control_packet(RUDP_FLAG_SYN, 0, nullptr, 0);

		pollfd poll_fd[1] = {
			{.fd = m_socketHandle, .events = POLLIN, .revents = 0 }
		};

		int ret = poll(poll_fd, 1, m_protocolTimeout);

		if (ret == SOCKET_ERROR) _print_socket_error("Failed to poll the socket", true);

		else if (ret == 0)
		{
			if (m_debugMode) std::cerr << "Warning: Timeout occurred while waiting for a response packet. Retrying connection (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
			continue;
		}

		int bytes_recv = recvfrom(m_socketHandle, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, (socklen_t *)&server_addr_len);

		if (bytes_recv == SOCKET_ERROR) _print_socket_error("Failed to receive a response packet", true);
		else if (_check_packet_source((struct sockaddr *)&server_addr, server_addr_len))
		{
			num_of_tries--;
			continue;
		}

		int packet_validity = _check_packet_validity(buffer, bytes_recv, RUDP_FLAG_SYN | RUDP_FLAG_ACK);

		switch (packet_validity)
		{
			case 0:
				if (m_debugMode) std::cerr << "Retrying connection (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
				break;

			case -1:
				throw std::runtime_error("Failed to connect to " + std::string(dest_ip) + ":" + std::to_string(dest_port) + ": connection rejected by the peer.");

			default:
				m_isConnected = true;
				std::cout << "Connection established with " << dest_ip << ":" << dest_port << std::endl;

				RUDP_SYN_packet *syn_packet = (RUDP_SYN_packet *)(buffer + sizeof(RUDP_header));
				m_peersMTU = ntohs(syn_packet->MTU);

				if (m_debugMode)
				{
					std::cout << "Peer connection information:" << std::endl;
					std::cout << "\tMTU: " << m_peersMTU << " bytes" << std::endl;
					std::cout << "\tTimeout: " << ntohs(syn_packet->timeout) << " milliseconds" << std::endl;
					std::cout << "\tMaximum number of retries: " << ntohs(syn_packet->max_retries) << std::endl;
					std::cout << "\tDebug mode: " << ntohs(syn_packet->debug_mode) << std::endl;

					if (m_peersMTU < m_protocolMTU)
					{
						std::cerr << "Warning: MTU mismatch: configured " << m_protocolMTU << " bytes, peer's MTU is " << m_peersMTU << " bytes; Automatic readjustment of the MTU value for this connection." << std::endl;
						std::cerr << "You can use forceUseOwnMTU() to force the use of the configured MTU value instead, but this may cause issues with the connection." << std::endl;
					}
				}

				return true;
		}
	}

	std::cerr << "Failed to connect to " << dest_ip << ":" << dest_port << std::endl;
	std::cerr << "Please check the server's IP address and port number." << std::endl;

	return false;
}

bool RUDP_Socket_p::accept()
{
	if (!m_isServer) throw std::runtime_error("Client sockets cannot accept connections. Use connect() instead.");
	if (m_isConnected) throw std::runtime_error("There is already an active connection. Use disconnect() to close it.");

	uint8_t buffer[m_protocolMTU] = {0};

	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	memset(&client_addr, 0, sizeof(client_addr));

	while (true)
	{
		int bytes_recv = recvfrom(m_socketHandle, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);

		if (bytes_recv == SOCKET_ERROR) _print_socket_error("Failed to receive a connection request packet", true);

		int packet_validity = _check_packet_validity(buffer, bytes_recv, RUDP_FLAG_SYN);

		if (!packet_validity) continue;
		else if (packet_validity == -1) return false;

		m_isConnected = true;

		RUDP_SYN_packet *syn_packet = (RUDP_SYN_packet *)(buffer + sizeof(RUDP_header));
		m_peersMTU = ntohs(syn_packet->MTU);

		if (m_debugMode)
		{
			std::cout << "Peer connection information:" << std::endl;
			std::cout << "\tMTU: " << m_peersMTU << " bytes" << std::endl;
			std::cout << "\tTimeout: " << ntohs(syn_packet->timeout) << " milliseconds" << std::endl;
			std::cout << "\tMaximum number of retries: " << ntohs(syn_packet->max_retries) << std::endl;
			std::cout << "\tDebug mode: " << ntohs(syn_packet->debug_mode) << std::endl;

			if (m_peersMTU < m_protocolMTU)
			{
				std::cerr << "Warning: MTU mismatch: configured " << m_protocolMTU << " bytes, peer's MTU is " << m_peersMTU << " bytes; Automatic readjustment of the MTU value for this connection." << std::endl;
				std::cerr << "You can use forceUseOwnMTU() to force the use of the configured MTU value instead, but this may cause issues with the connection." << std::endl;
			}
		}
		
		memcpy(&m_destinationAddress4, &client_addr, client_addr_len);
		_send_control_packet(RUDP_FLAG_SYN | RUDP_FLAG_ACK, 0, nullptr, 0);
		break;
	}

	std::cout << "Connection established with " << inet_ntoa(m_destinationAddress4.sin_addr) << ":" << ntohs(m_destinationAddress4.sin_port) << std::endl;
	
	return true;
}

int RUDP_Socket_p::recv(void *buffer, uint32_t buffer_size)
{
	if (!m_isConnected)
		throw std::runtime_error("There is no active connection to receive data from.");

	if (buffer == nullptr)
		throw std::runtime_error("Buffer is null.");

	uint8_t packet[m_protocolMTU] = {0}, *buffer_ptr = (uint8_t *)buffer;
	uint32_t total_packets = 0, total_actual_packets = 0, prev_seq_num = UINT32_MAX;
	int total_bytes = 0, total_actual_bytes = 0, dup_packets = 0, bytes_recv = 0;
	int packet_validity = 0;

	struct sockaddr_in source_addr;
	socklen_t source_addr_len = sizeof(source_addr);

	for (size_t num_of_tries = 0; num_of_tries <= m_protocolMaximumRetries; num_of_tries++)
	{
		if (num_of_tries == m_protocolMaximumRetries) throw std::runtime_error("Failed to receive the first packet: maximum number of retries reached (" + std::to_string(m_protocolMaximumRetries) + ")");

		bytes_recv = recvfrom(m_socketHandle, (char *)packet, sizeof(packet), 0, (struct sockaddr *)&source_addr, &source_addr_len);

		if (bytes_recv == SOCKET_ERROR) _print_socket_error("Failed to receive the first packet", true);
		else if (_check_packet_source((struct sockaddr *)&source_addr, source_addr_len))
		{
			num_of_tries--;
			continue;
		}

		packet_validity = _check_packet_validity(packet, bytes_recv, RUDP_FLAG_PSH);

		if (packet_validity == 0)
		{
			if (m_debugMode) std::cerr << "Retrying to receive the first packet (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
			continue;
		}

		else if (packet_validity == -1) return 0;
		break;
	}

	total_bytes += (bytes_recv - sizeof(RUDP_header));

	memcpy(buffer_ptr, packet + sizeof(RUDP_header), ((total_bytes > (int)buffer_size) ? buffer_size : total_bytes));
	total_packets++;
	total_actual_bytes += bytes_recv;
	total_actual_packets++;
	prev_seq_num = ntohl(((RUDP_header *)packet)->seq_num);

	_send_control_packet(RUDP_FLAG_ACK, prev_seq_num, nullptr, 0);

	if (total_bytes > (int)buffer_size)
	{
		if (m_debugMode) 
		{
			std::cerr << "Warning: Buffer overflow detected, stopping the reception." << std::endl;
			std::cerr << "Received " << total_bytes << " bytes over " << total_packets << " packets, but could only store " << buffer_size << " bytes." << std::endl;
		}
		return total_bytes;
	}

	if (((RUDP_header *)packet)->flags & RUDP_FLAG_LAST)
	{
		if (m_debugMode) std::cout << "Received " << total_bytes << " bytes over " << total_packets << " packets." << std::endl;
		return total_bytes;
	}

	while (true)
	{
		for (size_t num_of_tries = 0; num_of_tries <= m_protocolMaximumRetries; num_of_tries++)
		{
			if (num_of_tries == m_protocolMaximumRetries) throw std::runtime_error("Failed to receive the packet: maximum number of retries reached (" + std::to_string(m_protocolMaximumRetries) + ")");

			memset(packet, 0, sizeof(packet));

			pollfd poll_fd[1] = {
				{.fd = m_socketHandle, .events = POLLIN, .revents = 0 }
			};
			int ret = poll(poll_fd, 1, m_protocolTimeout);
			if (ret == SOCKET_ERROR) _print_socket_error("Failed to poll the socket", true);
			else if (ret == 0)
			{
				if (m_debugMode) std::cerr << "Warning: Timeout occurred while waiting for a data packet with sequence number " << prev_seq_num + 1 << ". Retrying (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
				continue;
			}

			bytes_recv = recvfrom(m_socketHandle, (char *)packet, sizeof(packet), 0, (struct sockaddr *)&source_addr, &source_addr_len);

			if (bytes_recv == SOCKET_ERROR) _print_socket_error("Failed to receive a packet", true);
			else if (_check_packet_source((struct sockaddr *)&source_addr, source_addr_len))
			{
				num_of_tries--;
				continue;
			}

			packet_validity = _check_packet_validity(packet, bytes_recv, RUDP_FLAG_PSH);

			total_actual_bytes += bytes_recv;
			total_actual_packets++;

			if (packet_validity == 0)
			{
				if (m_debugMode) std::cerr << "Retrying to receive the packet with sequence number " << prev_seq_num + 1 << " (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
				continue;
			}
			else if (packet_validity == -1) return 0;

			break;
		}

		RUDP_header *header = (RUDP_header *)packet;
		uint32_t packet_seq_num = ntohl(header->seq_num);
		uint16_t packet_size = ntohs(header->length);
		uint32_t offset = ntohl(header->seq_num) * (m_protocolMTU - sizeof(RUDP_header));

		if (packet_seq_num == prev_seq_num)
		{
			if (m_debugMode) std::cerr << "Warning: Received a duplicate packet with sequence number " << packet_seq_num << ", send duplicate ACK packet." << std::endl;
			dup_packets++;
			_send_control_packet(RUDP_FLAG_ACK, prev_seq_num, nullptr, 0);
			continue;
		}

		if (packet_seq_num != (prev_seq_num + 1))
		{
			if (m_debugMode) std::cerr << "Warning: Received an out-of-order packet with sequence number " << packet_seq_num << ", expected " << (prev_seq_num + 1) << ", retrying to receive the packet." << std::endl;
			_send_control_packet(RUDP_FLAG_ACK, prev_seq_num, nullptr, 0);
			continue;
		}

		total_bytes += packet_size;
		total_packets++;
		prev_seq_num = packet_seq_num;

		if ((offset + packet_size) > buffer_size) packet_size = buffer_size - offset;
		memcpy((buffer_ptr + offset), (packet + sizeof(RUDP_header)), packet_size);
		_send_control_packet(RUDP_FLAG_ACK, packet_seq_num, nullptr, 0);

		if ((header->flags & RUDP_FLAG_LAST) != 0)
		{
			if (m_debugMode) std::cout << "Received the last packet, stopping the reception." << std::endl;
			break;
		}
		
		if (total_bytes > ((int32_t)buffer_size))
		{
			if (m_debugMode) std::cout << "Warning: Buffer overflow detected, stopping the reception." << std::endl;
			break;
		}
	}

	if (m_debugMode)
	{
		std::cout << "Received " << total_bytes << " bytes over " << total_packets << " packets." << std::endl;
		std::cout << "Actual overhead: " << total_actual_bytes << " bytes over " << total_actual_packets << " packets, of which " << dup_packets << " are duplicate packets." << std::endl;
	}

	return total_bytes;
}

int RUDP_Socket_p::send(void *buffer, uint32_t buffer_size)
{
	if (!m_isConnected) throw std::runtime_error("There is no active connection to send data to.");
	if (buffer == nullptr) throw std::runtime_error("Buffer is null.");
	
	uint8_t packet[m_protocolMTU] = {0}, *buffer_ptr = (uint8_t *)buffer;
	uint32_t total_packets = 0, total_actual_packets = 0, expected_packets = ((buffer_size / (std::min(m_protocolMTU, m_peersMTU) - sizeof(RUDP_header))) + 1), prev_seq_num = UINT32_MAX;
	int total_bytes = 0, total_actual_bytes = 0, retry_packets = 0;

	struct sockaddr_in source_addr;
	socklen_t source_addr_len = sizeof(source_addr);

	if (m_debugMode) std::cout << "Sending " << buffer_size << " bytes over " << expected_packets << " packets." << std::endl;

	for (uint32_t i = 0; i < expected_packets; i++)
	{
		uint32_t packet_size = std::min(buffer_size - (uint32_t)total_bytes, (uint32_t)(std::min(m_protocolMTU, m_peersMTU) - sizeof(RUDP_header)));
		memset(packet, 0, sizeof(packet));
		memcpy(packet + sizeof(RUDP_header), buffer_ptr + (uint32_t)total_bytes, packet_size);

		RUDP_header *header = (RUDP_header *)packet;
		header->flags = (i == expected_packets - 1) ? (RUDP_FLAG_PSH | RUDP_FLAG_LAST) : RUDP_FLAG_PSH;
		header->length = htons(packet_size);
		header->seq_num = htonl(total_packets);
		header->checksum = htons(RUDP_Socket_p::_calculate_checksum(packet, sizeof(RUDP_header) + packet_size));

		for (size_t num_of_tries = 0; num_of_tries <= m_protocolMaximumRetries; num_of_tries++)
		{
			if (num_of_tries == m_protocolMaximumRetries) throw std::runtime_error("Failed to send the packet: maximum number of retries reached (" + std::to_string(m_protocolMaximumRetries) + ").");
			else if (num_of_tries > 0) retry_packets++;

			int bytes_sent = sendto(m_socketHandle, (char*)packet, sizeof(RUDP_header) + packet_size, 0, (struct sockaddr *)&m_destinationAddress4, sizeof(m_destinationAddress4));

			if (bytes_sent == SOCKET_ERROR) _print_socket_error("Failed to send a packet", true);

			total_actual_bytes += bytes_sent;
			total_actual_packets++;

			uint8_t ack_buffer[m_protocolMTU] = {0};

			pollfd poll_fd[1] = {
				{.fd = m_socketHandle, .events = POLLIN, .revents = 0 }
			};
			int ret = poll(poll_fd, 1, m_protocolTimeout);
			if (ret == SOCKET_ERROR) _print_socket_error("Failed to poll the socket", true);
			else if (ret == 0)
			{
				if (m_debugMode) std::cerr << "Warning: Timeout occurred while waiting for a response packet with sequence number " << total_packets << ", retrying to send the packet (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
				continue;
			}

			int bytes_recv = recvfrom(m_socketHandle, (char*)ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&source_addr, &source_addr_len);
			if (bytes_recv == SOCKET_ERROR) _print_socket_error("Failed to receive an ACK packet", true);
			else if (_check_packet_source((struct sockaddr *)&source_addr, source_addr_len))
			{
				num_of_tries--;
				continue;
			}

			int packet_validity = _check_packet_validity(ack_buffer, bytes_recv, RUDP_FLAG_ACK);
			if (packet_validity == 0)
			{
				if (m_debugMode) std::cerr << "Retrying to send packet " << total_packets << " (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
				continue;
			}
			else if (packet_validity == -1) return 0;

			RUDP_header *ack_packet = (RUDP_header *)ack_buffer;
			uint32_t ack_seq_num = ntohl(ack_packet->seq_num);

			if (ack_seq_num == prev_seq_num && (i != (expected_packets - 1)))
			{
				if (m_debugMode) std::cout << "Warning: Received a duplicate ACK packet with sequence number " << ack_seq_num << ", continuing to the next packet." << std::endl;
				break;
			}

			if (ack_seq_num < total_packets)
			{
				if (m_debugMode) std::cerr << "Warning: Received an out-of-order ACK packet with sequence number " << ack_seq_num << " while expecting " << total_packets << ", retrying to send the packet (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
				continue;
			}

			prev_seq_num = ack_seq_num;

			total_bytes += packet_size;
			total_packets++;

			break;
		}
	}

	if (m_debugMode)
	{
		std::cout << "Sent " << total_bytes << " bytes over " << total_packets << " packets." << std::endl;
		std::cout << "Actual overhead: " << total_actual_bytes << " bytes over " << total_actual_packets << " packets, of which " << retry_packets << " are retransmissions." << std::endl;
	}
	
	return total_bytes;
}

bool RUDP_Socket_p::disconnect()
{
	if (!m_isConnected) throw std::runtime_error("There is no active connection to close.");

	uint8_t buffer[m_protocolMTU] = {0};
	struct sockaddr_in source_addr;
	socklen_t source_addr_len = sizeof(source_addr);

	for (size_t num_of_tries = 0; num_of_tries < m_protocolMaximumRetries; num_of_tries++)
	{
		_send_control_packet(RUDP_FLAG_FIN, 0, nullptr, 0);
		memset(buffer, 0, sizeof(buffer));

		pollfd poll_fd[1] = {
			{.fd = m_socketHandle, .events = POLLIN, .revents = 0 }
		};

		int ret = poll(poll_fd, 1, m_protocolTimeout);

		if (ret == SOCKET_ERROR) _print_socket_error("Failed to poll the socket", true);

		else if (ret == 0)
		{
			if (m_debugMode) std::cerr << "Warning: Timeout occurred while waiting for a response packet. Retrying disconnection (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
			continue;
		}

		int bytes_recv = recvfrom(m_socketHandle, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&source_addr, &source_addr_len);

		if (bytes_recv == SOCKET_ERROR) _print_socket_error("Failed to receive a response packet", true);
		else if (_check_packet_source((struct sockaddr *)&source_addr, source_addr_len))
		{
			num_of_tries--;
			continue;
		}

		int packet_validity = _check_packet_validity(buffer, bytes_recv, RUDP_FLAG_FIN | RUDP_FLAG_ACK);

		if (packet_validity == 0)
		{
			if (m_debugMode) std::cerr << "Warning: Received an invalid response packet, ignoring it. Retrying disconnection (" << num_of_tries + 1 << "/" << m_protocolMaximumRetries << ")" << std::endl;
			continue;
		}

		m_isConnected = false;
		std::cout << "Connection closed with " << inet_ntoa(m_destinationAddress4.sin_addr) << ":" << ntohs(m_destinationAddress4.sin_port) << std::endl;
		memset(&m_destinationAddress4, 0, sizeof(m_destinationAddress4));
		return true;
	}

	std::cerr << "Failed to disconnect from " << inet_ntoa(m_destinationAddress4.sin_addr) << ":" << ntohs(m_destinationAddress4.sin_port) << std::endl;
	std::cerr << "Assuming that the connection is closed." << std::endl;

	m_isConnected = false;
	memset(&m_destinationAddress4, 0, sizeof(m_destinationAddress4));

	return true;
}