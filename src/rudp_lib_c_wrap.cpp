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

	RUDP_socket rudp_socket(bool isServer, uint16_t listen_port, uint16_t MTU, uint16_t timeout, uint16_t max_retries, bool debug_mode)
	{
		try
		{
			return (RUDP_socket)(new RUDP_Socket_p(isServer, listen_port, MTU, timeout, max_retries, debug_mode));
		}

		catch (const std::exception &e)
		{
			std::cerr << "rudp_socket() exception at object creation in RUDP_Socket_p():" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return NULL;
		}
	}

	bool rudp_connect(RUDP_socket socket, const char *dest_ip, uint16_t dest_port)
	{
		bool ret = false;

		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_connect() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return false;
		}

		try
		{
			ret = sock->connect(dest_ip, dest_port);
		}

		catch (const std::exception &e)
		{
			typedef bool (RUDP_Socket_p::*ConnectMethod)(const char *, uint16_t);
			ConnectMethod connectMethod = &RUDP_Socket_p::connect;
			std::cerr << "rudp_connect() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(connectMethod) << " (connect):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return false;
		}

		return ret;
	}

	bool rudp_accept(RUDP_socket socket)
	{
		bool ret = false;

		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_accept() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return false;
		}

		try
		{
			ret = sock->accept();
		}

		catch (const std::exception &e)
		{
			typedef bool (RUDP_Socket_p::*AcceptMethod)();
			AcceptMethod acceptMethod = &RUDP_Socket_p::accept;
			std::cerr << "rudp_accept() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(acceptMethod) << " (accept):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return false;
		}

		return ret;
	}

	int rudp_recv(RUDP_socket socket, void *buffer, uint32_t buffer_size)
	{
		int ret = -1;

		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_recv() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return -1;
		}

		try
		{
			ret = sock->recv(buffer, buffer_size);
		}

		catch (const std::exception &e)
		{
			typedef int (RUDP_Socket_p::*RecvMethod)(void *, uint32_t);
			RecvMethod recvMethod = &RUDP_Socket_p::recv;
			std::cerr << "rudp_recv() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(recvMethod) << " (recv):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return -1;
		}

		return ret;
	}

	int rudp_send(RUDP_socket socket, void *buffer, uint32_t buffer_size)
	{
		int ret = -1;

		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_send() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return -1;
		}

		try
		{
			ret = sock->send(buffer, buffer_size);
		}

		catch (const std::exception &e)
		{
			typedef int (RUDP_Socket_p::*SendMethod)(void *, uint32_t);
			SendMethod sendMethod = &RUDP_Socket_p::send;
			std::cerr << "rudp_send() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(sendMethod) << " (send):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return -1;
		}

		return ret;
	}

	bool rudp_disconnect(RUDP_socket socket)
	{
		bool ret = false;

		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_disconnect() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return false;
		}

		try
		{
			ret = sock->disconnect();
		}

		catch (const std::exception &e)
		{
			typedef bool (RUDP_Socket_p::*DisconnectMethod)();
			DisconnectMethod disconnectMethod = &RUDP_Socket_p::disconnect;
			std::cerr << "rudp_disconnect() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(disconnectMethod) << " (disconnect):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return false;
		}

		return ret;
	}

	void rudp_socket_free(RUDP_socket *socket)
	{
		if (socket == nullptr)
		{
			std::cerr << "rudp_socket_free() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return;
		}

		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)(*socket));

		if (sock == nullptr)
		{
			std::cerr << "rudp_socket_free() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return;
		}

		delete sock;

		// Set the pointer to NULL, to prevent further access to the freed memory.
		*socket = nullptr;
	}

	uint16_t rudp_get_mtu(RUDP_socket socket)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_get_mtu() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return 0;
		}

		return sock->getMTU();
	}

	uint16_t rudp_get_timeout(RUDP_socket socket)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_get_timeout() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return 0;
		}

		return sock->getTimeout();
	}

	uint16_t rudp_get_maxretries(RUDP_socket socket)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_get_maxretries() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return 0;
		}

		return sock->getMaxRetries();
	}

	uint16_t rudp_get_peers_MTU(RUDP_socket socket)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_get_peers_MTU() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return 0;
		}

		try
		{
			return sock->getPeersMTU();
		}

		catch (const std::exception &e)
		{
			typedef uint16_t (RUDP_Socket_p::*GetPeersMTUMethod)() const;
			GetPeersMTUMethod getPeersMTUMethod = &RUDP_Socket_p::getPeersMTU;
			std::cerr << "rudp_get_peers_MTU() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(getPeersMTUMethod) << " (getPeersMTU):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return 0;
		}
	}

	bool rudp_is_debug_mode(RUDP_socket socket)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_is_debug_mode() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return false;
		}

		return sock->isDebugMode();
	}

	bool rudp_is_connected(RUDP_socket socket)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_is_connected() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return false;
		}

		return sock->isConnected();
	}

	bool rudp_is_server(RUDP_socket socket)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_is_server() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return false;
		}

		return sock->isServer();
	}

	void rudp_set_debug_mode(RUDP_socket socket, bool debug_mode)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_set_debug_mode() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return;
		}

		sock->setDebugMode(debug_mode);
	}

	void rudp_set_MTU(RUDP_socket socket, uint16_t MTU)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_set_MTU() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return;
		}

		try
		{
			sock->setMTU(MTU);
		}

		catch (const std::exception &e)
		{
			typedef void (RUDP_Socket_p::*SetMTUMethod)(uint16_t);
			SetMTUMethod setMTUMethod = &RUDP_Socket_p::setMTU;
			std::cerr << "rudp_set_MTU() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(setMTUMethod) << " (setMTU):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return;
		}
	}

	void rudp_set_timeout(RUDP_socket socket, uint16_t timeout)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_set_timeout() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return;
		}

		try
		{
			sock->setTimeout(timeout);
		}

		catch (const std::exception &e)
		{
			typedef void (RUDP_Socket_p::*SetTimeoutMethod)(uint16_t);
			SetTimeoutMethod setTimeoutMethod = &RUDP_Socket_p::setTimeout;
			std::cerr << "rudp_set_timeout() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(setTimeoutMethod) << " (setTimeout):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return;
		}
	}

	void rudp_set_max_retries(RUDP_socket socket, uint16_t max_retries)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_set_max_retries() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return;
		}

		try
		{
			sock->setMaxRetries(max_retries);
		}

		catch (const std::exception &e)
		{
			typedef void (RUDP_Socket_p::*SetMaxRetriesMethod)(uint16_t);
			SetMaxRetriesMethod setMaxRetriesMethod = &RUDP_Socket_p::setMaxRetries;
			std::cerr << "rudp_set_max_retries() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(setMaxRetriesMethod) << " (setMaxRetries):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return;
		}
	}

	void rudp_force_use_own_MTU(RUDP_socket socket)
	{
		RUDP_Socket_p *sock = dynamic_cast<RUDP_Socket_p *>((RUDP_Socket_p *)socket);

		if (sock == nullptr)
		{
			std::cerr << "rudp_force_use_own_MTU() exception at access to socket pointer:" << std::endl;
			std::cerr << "\tInvalid socket pointer: Expected RUDP_Socket_p*, instead got NULL/invalid pointer." << std::endl;
			return;
		}

		try
		{
			sock->forceUseOwnMTU();
		}

		catch (const std::exception &e)
		{
			typedef void (RUDP_Socket_p::*ForceUseOwnMTUMethod)();
			ForceUseOwnMTUMethod forceUseOwnMTUMethod = &RUDP_Socket_p::forceUseOwnMTU;
			std::cerr << "rudp_force_use_own_MTU() exception at " << static_cast<void *>(sock) << " in " << reinterpret_cast<void *&>(forceUseOwnMTUMethod) << " (forceUseOwnMTU):" << std::endl;
			std::cerr << "\t" << e.what() << std::endl;
			return;
		}
	}

#ifdef __cplusplus
}
#endif
