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

#include <cstdlib>
#include <iostream>
#include "include/RUDP_API.h"
#include "include/RUDP_API_wrap.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

	RUDP_socket rudp_socket(bool isServer, uint16_t listen_port, uint16_t MTU, uint16_t timeout, uint16_t max_retries)
	{
		return (RUDP_socket)(new RUDP_Socket_p(isServer, listen_port, MTU, timeout, max_retries));
	}

	bool rudp_connect(RUDP_socket socket, const char *dest_ip, uint16_t dest_port)
	{
		bool ret = false;
		RUDP_Socket_p *sock = (RUDP_Socket_p *)socket;
		try
		{
			ret = sock->connect(dest_ip, dest_port);
		}

		catch (const std::exception &e)
		{
			std::cerr << e.what() << std::endl;
			return false;
		}

		return ret;
	}

	bool rudp_accept(RUDP_socket socket)
	{
		bool ret = false;
		RUDP_Socket_p *sock = (RUDP_Socket_p *)socket;
		try
		{
			ret = sock->accept();
		}

		catch (const std::exception &e)
		{
			std::cerr << e.what() << std::endl;
			return false;
		}

		return ret;
	}

	int rudp_recv(RUDP_socket socket, void *buffer, uint32_t buffer_size)
	{
		int ret = -1;
		RUDP_Socket_p *sock = (RUDP_Socket_p *)socket;
		try
		{
			ret = sock->recv(buffer, buffer_size);
		}

		catch (const std::exception &e)
		{
			std::cerr << e.what() << std::endl;
			return -1;
		}

		return ret;
	}

	int rudp_send(RUDP_socket socket, void *buffer, uint32_t buffer_size)
	{
		int ret = -1;
		RUDP_Socket_p *sock = (RUDP_Socket_p *)socket;
		try
		{
			ret = sock->send(buffer, buffer_size);
		}

		catch (const std::exception &e)
		{
			std::cerr << e.what() << std::endl;
			return -1;
		}

		return ret;
	}

	bool rudp_disconnect(RUDP_socket socket)
	{
		bool ret = false;
		RUDP_Socket_p *sock = (RUDP_Socket_p *)socket;
		try
		{
			ret = sock->disconnect();
		}

		catch (const std::exception &e)
		{
			std::cerr << e.what() << std::endl;
			return false;
		}

		return ret;
	}

#ifdef __cplusplus
}
#endif
