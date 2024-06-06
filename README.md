# Reliable UDP library
**By Roy Simanovich**

[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0) [![GitHub release](https://img.shields.io/github/v/release/RoySenpai/Reliable-UDP)](https://github.com/RoySenpai/Reliable-UDP/releases) [![GitHub issues](https://img.shields.io/github/issues/RoySenpai/Reliable-UDP)](https://github.com/RoySenpai/Reliable-UDP/issues) [![GitHub pull requests](https://img.shields.io/github/issues-pr/RoySenpai/Reliable-UDP)](https://github.com/RoySenpai/Reliable-UDP/pulls)


## Description

This is a fun project I did to learn more about how UDP works and how to implement a reliable protocol on top of it. The library is written in C++, and uses simple socket programming to send and receive packets via UDP (IPv4) in a reliable manner:

1. A custom header is added to each packet to keep track of the sequence number, type of packet, length of the payload, and a checksum for error detection.
2. Each sent packet is accompanied by a timer that will resend the packet if an acknowledgment is not received within a certain time frame.
3. There isn't any congestion control or advanced flow control implemented, so the library is not suitable for high-speed networks, it is more of a proof of concept and a learning experience.
4. The socket supports only one connection at a time, and no multi-threading support is provided.
5. Flow control is implemented using the Stop-and-Wait protocol, which is a simple protocol that sends one packet at a time and waits for an acknowledgment before sending the next packet.
6. If the MTU of one peer is smaller than the other, the smaller MTU will be used for both peers to avoid fragmentation issues. It can be overridden by the user if needed, using the `forceUseOwnMTU()` method.
7. If there is an active connection, and a packet is received from a different peer, the packet will be ignored, and the receiver will immediately send a FIN packet to the unexpected peer to force it to close the connection.

### Deep dive

The library has one main class that handles the RUDP socket: `RUDP_Socket` for C++ and `RUDP_socket` for C. The class has the following public methods:

- `RUDP_Socket::RUDP_Socket(bool isServer, uint16_t listen_port, uint16_t MTU, uint16_t timeout, uint16_t max_retries, bool debug_mode)`: Constructor that initializes the socket.
- `RUDP_Socket::connect(const char* ip, uint16_t port)`: Connects to a peer with a given IP address and port number (client only).
- `RUDP_Socket::accept(uint16_t port)`: Accepts a connection from a peer on a given port number (server only).
- `RUDP_Socket::send(const void* data, size_t size)`: Sends a packet of data of a given size.
- `RUDP_Socket::recv(void* buffer, size_t size)`: Receives a packet of data of a given size.
- `RUDP_Socket::disconnect()`: Disconnects from the peer, if connected.

Also, the class has some getters and setters for the socket settings:
- `RUDP_Socket::getMTU()`: Returns the MTU of the socket.
- `RUDP_Socket::getTimeout()`: Returns the timeout of the socket.
- `RUDP_Socket::getMaxRetries()`: Returns the maximum number of retries of the socket.

- `RUDP_Socket::isDebugMode()`: Returns whether the socket is in debug mode or not.
- `RUDP_Socket::isConnected()`: Returns whether the socket is connected to a peer or not.
- `RUDP_Socket::isServer()`: Returns whether the socket is a server or a client.

- `RUDP_Socket::setMTU(uint16_t MTU)`: Sets the MTU of the socket, valid only if the socket is not connected.
- `RUDP_Socket::setTimeout(uint16_t timeout)`: Sets the timeout of the socket.
- `RUDP_Socket::setMaxRetries(uint16_t max_retries)`: Sets the maximum number of retries of the socket.
- `RUDP_Socket::setDebugMode(bool debug_mode)`: Sets the debug mode status of the socket.

- `RUDP_Socket::forceUseOwnMTU()`: Forces the socket to use its own MTU instead of the peer's MTU, valid only if the socket is connected. **Experimental feature, use with caution.**


For the C version, the methods are the same, but they are prefixed with `rudp_` instead of `RUDP_Socket::`, and the socket is a pointer to a `RUDP_socket` struct.

Note that `RUDP_API.h` is the header file for the C version of the library, and `RUDP_API.hpp` is the header file for the C++ version of the library.

**Note**: In C, instead of using `free()`, you can use the `rudp_socket_free()` function to free the memory allocated for the socket.

#### The RUDP header
The RUDP header is a simple struct that is added to the beginning of each packet to keep track of the sequence number, type of packet, length of the payload, and a checksum for error detection. The header is defined as follows:

|     Field     |      Type      | Description                                                                |
| :-----------: | :------------: | :------------------------------------------------------------------------- |
|  `seq_num`  |  `uint32_t`  | Sequence number of the packet.                                             |
|  `length`  |  `uint16_t`  | Length of the data in bytes.                                               |
| `checksum` |  `uint16_t`  | Checksum calculated for the entire packet, including the header.           |
|   `flags`   |  `uint8_t`  | Bit-field representing various flags:                                      |
|              |                | -`RUDP_FLAG_SYN`: Connection is being established.                       |
|              |                | -`RUDP_FLAG_ACK`: Acknowledgement of data.                               |
|              |                | -`RUDP_FLAG_PSH`: Data is pushed to the application.                     |
|              |                | -`RUDP_FLAG_LAST`: This is the last packet of the message.               |
|              |                | -`RUDP_FLAG_FIN`: Connection is closing.                                 |
| `_reserved` | `uint8_t[3]` | Three bytes reserved for future use. For now, used for alignment purposes. |

##### The Sequence number
The sequence number is a 16-bit number that is used to keep track of the order of the packets of a single message. It is incremented by one for each packet sent, and it is used by the receiver to put the packets in the correct order into the buffer by adjusting the buffer pointer offset. The sequence number is also used to detect duplicate packets and to handle retransmissions.

##### The Length
The length is a 16-bit number that represents the length of the data in the packet, excluding the header. It is used by the receiver to know how many bytes to read from the packet and put into the buffer. The length is also used to detect incomplete packets and to handle retransmissions.

##### The Checksum
The checksum is a 16-bit number that is calculated for the entire packet, including the header. It is used for error detection and correction. The checksum is calculated by summing all the bytes of the packet, including the header, and then taking the one's complement of the sum. The receiver calculates the checksum for each packet it receives and compares it to the checksum in the packet header. If the checksums do not match, the packet is discarded, and the receiver sends a negative acknowledgment to the sender to request a retransmission of the packet.

##### The Flags
The protocol uses different types of packets to handle various situations. The packet types are defined as follows:
- `SYN`: Connection initiation packet sent by the client to the server. It contains the client's settings (MTU, timeout, max_retries and debug_mode).
- `SYN-ACK`: Connection acknowledgment packet sent by the server to the client. It contains the server's settings (MTU, timeout, max_retries and debug_mode).
- `PSH`: Data packet sent by the sender to the receiver. It contains the data to be sent.
- `ACK`: Acknowledgment packet sent by the receiver to the sender. It acknowledges the receipt of a packet.
- `LAST`: Last packet of the message. It is sent by the sender to indicate that this is the last packet of the message.
- `FIN`: Connection closing packet sent by the sender to the receiver. It indicates that the sender wants to close the connection.
- `FIN-ACK`: Connection closing acknowledgment packet sent by the receiver to the sender. It acknowledges the receipt of the `FIN` packet and closes the connection from the receiver's side.

##### The Reserved field
The reserved field is three bytes reserved for future use. For now, they are used for alignment purposes to make sure that the header is aligned correctly in memory. The reserved field is not used for anything else at the moment, but it may be used for additional flags or information in the future. The reserved field is set to zero when the packet is created and is always ignored by the receiver.


#### The RUDP socket settings
The RUDP socket has a few settings that can be adjusted to change the behavior of the socket. These settings are defined as follows:

|     Setting     |     Type     | Description                                                                                                    |
| :-------------: | :----------: | :------------------------------------------------------------------------------------------------------------- |
|     `MTU`     | `uint16_t` | Maximum Transmission Unit (MTU) of the socket.                                                                 |
|   `timeout`   | `uint16_t` | Timeout in milliseconds for retransmitting packets.                                                            |
| `max_retries` | `uint16_t` | Maximum number of retries before giving up on sending a packet.                                                |
| `debug_mode` |   `bool`   | Whether to print debug messages to the console. In C, exceptions will always print regardless of this setting. |

Those settings are shared between the peers when a connection is established (in the handshake process), and they can be used to adjust the behavior of the socket.


#### The RUDP socket communication process
The RUDP socket uses a simple Stop-and-Wait protocol to send and receive data. The communication process is as follows:
- The client sends a `SYN` packet to the server to initiate the connection. The packet contains the client's settings (MTU, timeout, max_retries and debug_mode).
- The server receives the `SYN` packet, copies the client's data, and sends a `SYN-ACK` packet back to the client. The packet contains the server's settings (MTU, timeout, max_retries and debug_mode).
- The peers now have each other's settings, and the connection is established. The client can now send data to the server, and the server can send data to the client.
- Each data send is done by automatically splitting the data into packets of the MTU size, and sending them one by one using the `PSH` flag. The sender waits for an acknowledgment from the receiver before sending the next packet.
- When the last packet of the current message is sent, the sender turns on the `LAST` flag in the packet to indicate that this is the last packet of the message.
- When the receiver receives the last packet of the message, it sends an acknowledgment back to the sender, and the sender can now send the next message.
- When the sender wants to close the connection, it sends a `FIN` packet to the receiver to indicate that it wants to close the connection.
- The receiver receives the `FIN` packet, sends a `FIN-ACK` packet back to the sender, and closes the connection. It does not accept any more data from the sender and will ignore any packets received after the `FIN` packet.

## Requirements

- A C++ and C compilers that supports C++17 and C11 or later (GCC, Clang, etc.).
- A Unix-like operating system (Linux, MacOS, etc.) or Windows.
- Root/sudo/admin privileges to install the library (optional, but recommended).
- If you're using Windows, its recommended to use MinGW to build the library.

## How to build

1. Clone the repository:

```bash
git clone https://github.com/RoySenpai/Reliable-UDP.git
```

2. Build and install the library (requires root/sudo privileges for linux systems, but you can skip this step if you only want to use the library in your project without installing it system-wide by using `make lib` instead of `make install`):

```bash
cd Reliable-UDP

# Run this command if you want to build the library in release mode
make DEBUG=0 install

# Run this command if you want to build the library in debug mode (symbols and additional debug information)
make DEBUG=1 install
```

3. Build the example programs (only if you want to run the examples):

```bash
make example
```

4. View `src/examples/include/` for additional information and API documentation.

## How to use

1. Include the header file in your program:

```cpp
#include <RUDP_API.hpp>
```

Or in C:

```c
#include <RUDP_API.h>
```

2. Create an instance of the `RUDP_Socket` class:

```cpp
bool isServer = true;
uint16_t port = 8080;
uint16_t MTU = 1024;
uint16_t timeout = 1000;
uint16_t max_retries = 5;
bool debug_mode = false;

RUDP_Socket socket(isServer, port, MTU, timeout, max_retries, debug_mode);
```

Or in C:

```c
bool isServer = true;
uint16_t port = 8080;
uint16_t MTU = 1024;
uint16_t timeout = 1000;
uint16_t max_retries = 5;
bool debug_mode = false;

RUDP_socket socket = rudp_socket(isServer, port, MTU, timeout, max_retries, debug_mode);
```

3. Use the socket to send and receive data:

```cpp
char buffer[] = "Hello, world!";
socket.send(buffer, sizeof(buffer));

char recv_buffer[MTU];
socket.recv(recv_buffer, sizeof(recv_buffer));
```

Or in C:

```c
char buffer[] = "Hello, world!";
rudp_send(socket, buffer, sizeof(buffer));

char recv_buffer[MTU];
rudp_recv(socket, recv_buffer, sizeof(recv_buffer));
```

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Future work

- [X] ~~Add Windows support.~~ (Done in v1.1)
- [ ] Add support for IPv6 and other network protocols.
- [ ] Add support for multiple connections at the same time.
- [ ] Implement congestion control and flow control to make the library suitable for high-speed networks.
- [X] ~~Improve the error detection and correction mechanisms.~~ (Done in v1.3)
- [ ] Add support for more systems.

## Bugs and feature requests

Feel free to open an issue if you find a bug or have a feature request. Pull requests are also welcome.
