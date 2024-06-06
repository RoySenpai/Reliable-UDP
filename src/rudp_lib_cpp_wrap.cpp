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

#include <exception>
#include "include/RUDP_API.hpp"
#include "include/RUDP_API_wrap.hpp"

RUDP_Socket::RUDP_Socket(bool isServer, uint16_t listen_port, uint16_t MTU, uint16_t timeout, uint16_t max_retries, bool debug_mode): _socket(new RUDP_Socket_p(isServer, listen_port, MTU, timeout, max_retries, debug_mode)) {}

RUDP_Socket::~RUDP_Socket() { delete _socket; }

bool RUDP_Socket::connect(const char *dest_ip, uint16_t dest_port) { return _socket->connect(dest_ip, dest_port); }

bool RUDP_Socket::accept() { return _socket->accept(); }

int RUDP_Socket::recv(void *buffer, uint32_t buffer_size) { return _socket->recv(buffer, buffer_size); }

int RUDP_Socket::send(void *buffer, uint32_t buffer_size) { return _socket->send(buffer, buffer_size); }

bool RUDP_Socket::disconnect() { return _socket->disconnect(); }

uint16_t RUDP_Socket::getMTU() const { return _socket->getMTU(); }

uint16_t RUDP_Socket::getTimeout() const { return _socket->getTimeout(); }

uint16_t RUDP_Socket::getMaxRetries() const { return _socket->getMaxRetries(); }

uint16_t RUDP_Socket::getPeersMTU() const { return _socket->getPeersMTU(); }

bool RUDP_Socket::isDebugMode() const { return _socket->isDebugMode(); }

bool RUDP_Socket::isConnected() const { return _socket->isConnected(); }

bool RUDP_Socket::isServer() const { return _socket->isServer(); }

void RUDP_Socket::setDebugMode(bool debug_mode) { _socket->setDebugMode(debug_mode); }

void RUDP_Socket::setMTU(uint16_t MTU) { _socket->setMTU(MTU); }

void RUDP_Socket::setTimeout(uint16_t timeout) { _socket->setTimeout(timeout); }

void RUDP_Socket::setMaxRetries(uint16_t max_retries) { _socket->setMaxRetries(max_retries); }

void RUDP_Socket::forceUseOwnMTU() { _socket->forceUseOwnMTU(); }