# Reliable UDP library

## Description
This is a fun project I did to learn more about how UDP works and how to implement a reliable protocol on top of it. The library is written in C++, and uses simple socket programming to send and receive packets via UDP (IPv4) in a reliable manner:
1. A custom header is added to each packet to keep track of the sequence number, type of packet, length of the payload, and a checksum for error detection.
2. Each sent packet is accompanied by a timer that will resend the packet if an acknowledgment is not received within a certain time frame.
3. There isn't any congestion control or advanced flow control implemented, so the library is not suitable for high-speed networks, it is more of a proof of concept and a learning experience.
4. The socket supports only one connection at a time, and no multi-threading support is provided.
5. Flow control is implemented using the Stop-and-Wait protocol, which is a simple protocol that sends one packet at a time and waits for an acknowledgment before sending the next packet.

### Deep dive
The library has one main class that handles the RUDP socket: `RUDP_Socket` for C++ and `RUDP_socket` for C. The class has the following public methods:
- `RUDP_Socket::RUDP_Socket(bool isServer, uint16_t listen_port, uint16_t MTU, uint16_t timeout, uint16_t max_retries, bool debug_mode)`: Constructor that initializes the socket.
- `RUDP_Socket::connect(const char* ip, uint16_t port)`: Connects to a peer with a given IP address and port number (client only).
- `RUDP_Socket::accept(uint16_t port)`: Accepts a connection from a peer on a given port number (server only).
- `RUDP_Socket::send(const void* data, size_t size)`: Sends a packet of data of a given size.
- `RUDP_Socket::recv(void* buffer, size_t size)`: Receives a packet of data of a given size.
- `RUDP_Socket::disconnect()`: Disconnects from the peer, if connected.

For the C version, the methods are the same, but they are prefixed with `rudp_` instead of `RUDP_Socket::`, and the socket is a pointer to a `RUDP_socket` struct.

Note that `RUDP_API.h` is the header file for the C version of the library, and `RUDP_API.hpp` is the header file for the C++ version of the library.


#### The RUDP header
The RUDP header is a simple struct that is added to the beginning of each packet to keep track of the sequence number, type of packet, length of the payload, and a checksum for error detection. The header is defined as follows:
|   Field    |    Type     |                                    Description                                        |
|:----------:|:-----------:|:--------------------------------------------------------------------------------------|
|  `seq_num` |  `uint32_t` | Sequence number of the packet.                                                        |
|  `length`  |  `uint16_t` | Length of the data in bytes.                                                          |
| `checksum` |  `uint16_t` | Checksum calculated for the entire packet, including the header.                      |
|   `flags`  |  `uint8_t`  | Bit-field representing various flags:                                                 |
|            |             | - `RUDP_FLAG_SYN`: Connection is being established.                                   |
|            |             | - `RUDP_FLAG_ACK`: Acknowledgement of data.                                           |
|            |             | - `RUDP_FLAG_PSH`: Data is pushed to the application.                                 |
|            |             | - `RUDP_FLAG_LAST`: This is the last packet of the message.                           |
|            |             | - `RUDP_FLAG_FIN`: Connection is closing.                                             |
| `_reserved`| `uint8_t[3]`| Three bytes reserved for future use. For now, used for alignment purposes.            |


#### The RUDP socket settings
The RUDP socket has a few settings that can be adjusted to change the behavior of the socket. These settings are defined as follows:
|   Setting  |    Type     |                                    Description                                        |
|:----------:|:-----------:|:--------------------------------------------------------------------------------------|
|   `MTU`    |  `uint16_t` | Maximum Transmission Unit (MTU) of the socket.                                                               |
| `timeout`  |  `uint16_t` | Timeout in milliseconds for retransmitting packets.                                                           |
| `max_retries`| `uint16_t`| Maximum number of retries before giving up on sending a packet.                                              |
| `debug_mode`| `bool`     | Whether to print debug messages to the console. In C, exceptions will always print regardless of this setting.|


## Requirements
- A C++ compiler that supports C++17 or later (GCC, Clang, etc.).
- A Unix-like operating system (Linux, MacOS, etc.).
- Root/sudo privileges to install the library (optional, but recommended).

## How to build
1. Clone the repository:
```bash
git clone https://github.com/RoySenpai/Reliable-UDP.git
```

2. Build and install the library (requires root/sudo privileges):
```bash
cd Reliable-UDP
make install
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
- Add Windows support.
- Add support for IPv6 and other network protocols.
- Add support for multiple connections at the same time.
- Implement congestion control and flow control to make the library suitable for high-speed networks.
- Improve the error detection and correction mechanisms.