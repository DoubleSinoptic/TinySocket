# TinySocket
A cross platform, minimal low-level socket library for C++

OS           | Support
------------ | --------
Windows  	 | Yes
Unix Based   | Yes

Make socket 
```cpp
ts::socket tcp_socket(ts::address_famaly::internet_network, ts::socket_type::stream, ts::protocol_type::tcp);
//or
ts::socket udp_socket(ts::address_famaly::internet_network, ts::socket_type::dgram, ts::protocol_type::udp);
```
Make socket address (endpoint of CSharp)
```cpp
ts::ip_end_point address(ts::ip_address(127, 0, 0, 1), 0);

ts::ip_end_point address(ts::ip_address_any, 0);
```
Sending data to address
```cpp
ts::socket udp_socket(ts::address_famaly::internet_network, ts::socket_type::dgram, ts::protocol_type::udp);
ts::ip_end_point address(ts::ip_address(127, 0, 0, 1), 0);

char message[] = "hello wrold!";
udp_socket.send_to(message, sizeof(message), address);
//recive

char buffer[1024];
udp_socket.bind(ts::ip_end_point(ts::ip_address_any, 19192));

ts::ip_end_point address_of(ts::ip_address_none, 0)
udp_socket.recive_from(buffer, 1024, address_of);
```