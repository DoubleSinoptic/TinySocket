#include "tinysocket.h"

#if defined(_WIN32)
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include <WinSock2.h>
	#if defined(_MSC_VER)
		#pragma comment(lib,"ws2_32.lib") 
	#endif	
	typedef int socklen_t;
	
	#define _sioct ioctlsocket
	#define _sclose closesocket
	#define _ssuberrorconst 10038

	char exception_buffer[2048];
	class ws2data_quard
	{
		
	public:
		ws2data_quard()
		{
			std::lock_guard<std::mutex> _(_lockRoot);
			is_init = false;
			if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
			{
				std::sprintf(exception_buffer, "wsa startup failed: error code: %d", WSAGetLastError());
				throw std::runtime_error(exception_buffer);
			}
			is_init = true;
		}
		~ws2data_quard()
		{
			std::lock_guard<std::mutex> _(_lockRoot);
			if (is_init)
				::WSACleanup();
		}

		void cheak() 
		{
			std::lock_guard<std::mutex> _(_lockRoot);
			if(!is_init)
				throw std::runtime_error("error: wsa not startup");
		}

	private:
		std::mutex _lockRoot;
		bool is_init;
		WSADATA wsa;
	} _wsintance;

	#define CHEK_SOCKET _wsintance.cheak()

#else
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <string.h>
	#include <errno.h>

	
	#define _sioct ioctl
	#define _sclose close
	#define _ssuberrorconst 88 

	#define CHEK_SOCKET

	#define INVALID_SOCKET -1	
#endif


ts::socket_native_error_code get_socket_error_code()
{
#if defined(_WIN32)
	return WSAGetLastError();
#else
	return errno;
#endif
}

#include <stdexcept>

const char* socket_errors[] = {
		"socket is closed",
		"destination address required",
		"message size",
		"protocol type",
		"protocol option",
		"protocol not supported",
		"socket not supported",
		"operation not supported",
		"protocol family not supported",
		"address family not supported",
		"address already in use",
		"address not available",
		"network down",
		"network unreachable",
		"network reset" ,
		"connection aborted" ,
		"connection reset",
		"no buffer space available" ,
		"is connected",
		"not connected",
		"shutdown" ,
		"timed out" ,
		"connection refused",
		"host down",
		"host unreachable"
	};


const char* errormsg_from_native_code(ts::socket_native_error_code code)
{
	ts::socket_native_error_code native_code = code - _ssuberrorconst;
	if ((native_code > 24) || (native_code < 0))
		return "socket undefined error (see the error code)";
	else
		return socket_errors[native_code];
}

ts::socket_exception::socket_exception(const char * message, socket_native_error_code code)
	: _code(code)
{
	snprintf(_exceptionMessgeBuffer, sizeof(_exceptionMessgeBuffer),
		"%s: %s", message, errormsg_from_native_code(code));
	
}

const char * ts::socket_exception::what() const throw()
{
	return _exceptionMessgeBuffer;
}

ts::socket_native_error_code ts::socket_exception::formated_error_code() const throw()
{
	return _code - _ssuberrorconst;
}

ts::socket_native_error_code ts::socket_exception::system_error_code() const throw()
{
	return _code;
}


int native_enum_socket_type(ts::socket_type ttype)
{
	switch (ttype)
	{
	case ts::socket_type::raw:
		return SOCK_RAW;
		break;
	case ts::socket_type::stream:
		return SOCK_STREAM;
		break;
	case ts::socket_type::dgram:
		return SOCK_DGRAM;
		break;
	default:
		return -1;
	}
}
ts::socket_type un_native_enum_socket_type(int ttype)
{
	switch (ttype)
	{
	case SOCK_STREAM:
		return ts::socket_type::stream;
		break;
	case SOCK_RAW:
		return ts::socket_type::raw;
		break;
	case SOCK_DGRAM:
		return ts::socket_type::dgram;
		break;
	default:
		return (ts::socket_type) - 1;
	}
}








int native_enum_address_famaly(ts::address_famaly ttype)
{
	switch (ttype)
	{
	case ts::address_famaly::internet_network:
		return AF_INET;
		break;
	case ts::address_famaly::internet_network_ipv6:
		return AF_INET6;
		break;
	default:
		return -1;
	}
}
ts::address_famaly un_native_enum_address_famaly(int ttype)
{
	switch (ttype)
	{
	case AF_INET:
		return ts::address_famaly::internet_network;
		break;
	case AF_INET6:
		return ts::address_famaly::internet_network_ipv6;
		break;
	default:
		return (ts::address_famaly) - 1;
	}
}



int native_enum_protocol_type(ts::protocol_type ttype)
{
	switch (ttype)
	{
	case ts::protocol_type::raw:
		return IPPROTO_RAW;
		break;
	case ts::protocol_type::ip:
		return IPPROTO_IP;
		break;
	case ts::protocol_type::tcp:
		return IPPROTO_TCP;
		break;
	case ts::protocol_type::udp:
		return IPPROTO_UDP;
		break;
	default:
		return -1;
	}
}
ts::protocol_type un_native_enum_protocol_type(int ttype)
{
	switch (ttype)
	{
	case IPPROTO_RAW:
		return ts::protocol_type::raw;
		break;
	case IPPROTO_IP:
		return ts::protocol_type::ip;
		break;
	case IPPROTO_TCP:
		return ts::protocol_type::tcp;
		break;
	case IPPROTO_UDP:
		return ts::protocol_type::udp;
		break;
	default:
		return (ts::protocol_type)-1;
	}
}






ts::port::port(int _Port)
{
	_port = htons(_Port);
}

std::ostream & ts::operator<<(std::ostream & _Stream, const ip_address & _Address)
{
	return (_Stream << (inet_ntoa(*(in_addr*)&_Address)));
}

std::ostream & ts::operator<<(std::ostream & _Stream, const port & _Port)
{
	return (_Stream << htons(_Port._port));
}

ts::ip_address::ip_address(const char * _Address)
{

	this->_address = inet_addr(_Address);
}

ts::ip_address::ip_address(uint32_t _NativeHotPost)
{
	_address = _NativeHotPost;
}

ts::ip_address::ip_address(ip4_part _A, ip4_part _B, ip4_part _C, ip4_part _D)
	: _address(0)
{
	 
	_address |= _D;
	_address = _address << 8;

	_address |= _C;
	_address = _address << 8;

	_address |= _B;
	_address = _address << 8;

	_address |= _A;
	//_address = htonl(_address);
}


void ts::ip_socket_address::serialaze(void * _Dest) const
{
	sockaddr_in* _r = (sockaddr_in*)_Dest;
	memset(_r, 0, sizeof(sockaddr_in));
	_r->sin_family = native_enum_address_famaly(get_famaly());
	_r->sin_port = get_port().native_port();
	_r->sin_addr.s_addr = get_address().native_address();
}

void ts::ip_socket_address::deserialaze(const void * _Src)
{
	sockaddr_in* _r = (sockaddr_in*)_Src;
	set_famaly(un_native_enum_address_famaly(_r->sin_family));
	set_port(htons(_r->sin_port));
	set_address(ip_address(_r->sin_addr.s_addr));

}

std::size_t totalBytesSended = 0;
std::size_t totalBytesReceived = 0;
std::size_t ts::socket::get_total_bytes_sended()
{
	return totalBytesSended;
}

std::size_t ts::socket::get_total_bytes_received()
{
	return totalBytesReceived;
}

ts::socket::socket(ts::address_famaly _Famaly, ts::socket_type _SocketTpye, ts::protocol_type _ProtocolType)  throw(socket_exception)
	: _endpoint(ts::ip_socket_address(ts::ip_address_none, 0))
{
	CHEK_SOCKET;

	_fd = ::socket(
		native_enum_address_famaly(_Famaly), 
		native_enum_socket_type(_SocketTpye),
		native_enum_protocol_type(_ProtocolType));

	if (_fd == INVALID_SOCKET) 
		throw socket_exception("error: of create socket", get_socket_error_code());
}

ts::socket::socket(socket_native_fd _NativeFd, const ip_socket_address& _RemoteAddres) throw(socket_exception)
	: _endpoint(ts::ip_socket_address(ts::ip_address_none, 0))
{
	_fd = _NativeFd;
	_endpoint = _RemoteAddres;
}

ts::socket::~socket()
{
	close();
}

void ts::socket::listen(int _maxconnections) throw(socket_exception)
{
	if (::listen(_fd, _maxconnections))
	{
		throw socket_exception("error: of listen socket", get_socket_error_code());
	}

}

void ts::socket::bind(const socket_address & _EndPoint) throw(socket_exception)
{
	
	sockaddr isp;

	_EndPoint.serialaze(&isp);

	if (::bind(_fd, &isp, sizeof(isp)))
	{
		//std::cout << WSAGetLastError() << std::endl;
		throw socket_exception("error: of bind socket", get_socket_error_code());
	}
}

ts::socket_native_fd ts::socket::get_native_fd()
{
	return _fd;
}

//void ts::socket::set_option(int _Option)
//{
//
//	
//}
//
//int ts::socket::get_option()
//{
//	return 0;
//}

void ts::socket::set_receive_time_out(ms_time_out mssec) throw(socket_exception)
{
	struct timeval tv;

	tv.tv_sec = mssec;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
	
	if (setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval))) 
		throw socket_exception("error: socket: of set option to", get_socket_error_code());
}

void ts::socket::set_send_time_out(ms_time_out mssec) throw(socket_exception)
{
	struct timeval tv;

	tv.tv_sec = mssec;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors

	if(setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval)))
		throw socket_exception("error: socket: of set option to", get_socket_error_code());
}

void ts::socket::connect(const socket_address & _To) throw(socket_exception)
{
	sockaddr addr_to; 
	_To.serialaze(&addr_to);
	
	if (::connect(_fd, &addr_to, sizeof(sockaddr))) 
	{
		throw socket_exception("error: socket: of connect to", get_socket_error_code());
	}
	_endpoint.deserialaze(&_To);
}

size_t ts::socket::send(const void * _Data, size_t _DataLen, socket_flags flags) throw(socket_exception)
{
	totalBytesSended += _DataLen;
	int rret = ::send(_fd, (const char*)_Data, _DataLen, (int)flags);
	if (rret < 0)
		throw socket_exception("error: socket: of send data", get_socket_error_code());
	return rret;
}

size_t ts::socket::receive(void * _Data, size_t _DataLen, socket_flags flags) throw(socket_exception)
{
	totalBytesReceived += _DataLen;
	int rret = ::recv(_fd, (char*)_Data, _DataLen, (int)flags);
	if (rret < 0)
		throw socket_exception("error: socket: of receive data", get_socket_error_code());
	return rret;
}

#include <iostream>
size_t ts::socket::send_to(const void * _Data, size_t _DataLen, const socket_address & _To, socket_flags flags) throw(socket_exception)
{
	totalBytesSended += _DataLen;
	sockaddr data;
	_To.serialaze(&data);
	int rret = ::sendto(_fd, (const char*)_Data, _DataLen, (int)flags, &data, sizeof(sockaddr));
	if (rret < 0) {
		//std::cout << WSAGetLastError() << std::endl;
		throw socket_exception("error: socket: of send data to endpoint", get_socket_error_code());
	
	}	
	return rret;
}

size_t ts::socket::receive_from(void * _Data, size_t _DataLen, socket_address & _From, socket_flags flags) throw(socket_exception)
{
	totalBytesReceived += _DataLen;
	socklen_t fromlen = sizeof(sockaddr);
	sockaddr data;
	_From.serialaze(&data);
	
	int rret = ::recvfrom(_fd, (char*)_Data, _DataLen, (int)flags, &data, &fromlen);
	if (rret < 0) {
		//std::cout << WSAGetLastError() << std::endl;
		throw socket_exception("error: socket: of receive from data from endpoint", get_socket_error_code());
	
	}
	_From.deserialaze(&data);
	return rret;
}

ts::socket ts::socket::accept() throw(socket_exception)
{
	sockaddr recv;
	socklen_t len = sizeof(sockaddr);
	memset(&recv, 0, sizeof(recv));
	int e = ::accept(_fd, &recv, &len);
	if (e == INVALID_SOCKET)
		throw socket_exception("error: of accept socket", get_socket_error_code());
	_endpoint.deserialaze(&recv);
	return socket(e, _endpoint);

}

ts::socket * ts::socket::accept_new() throw(socket_exception)
{
	sockaddr recv;
	socklen_t len = 0;
	memset(&recv, 0, sizeof(recv));
	ts::socket_native_fd e = ::accept(_fd, &recv, &len);
	if (e == INVALID_SOCKET)
		throw socket_exception("error: of accept socket", get_socket_error_code());
	_endpoint.deserialaze(&recv);
	return new socket(e, _endpoint);
}

void ts::socket::close()
{
	if (_fd == 0)
		return;
	::_sclose(_fd);
	_fd = 0;
}

void ts::socket::shutdown(socket_shutdown _O) throw(socket_exception)
{
	if(::shutdown(_fd, static_cast<int>(_O)))
		throw socket_exception("error: of shutdown socket", get_socket_error_code());
}

ts::ip_socket_address ts::socket::remote_endpoint()
{
	return _endpoint;
}

void  ts::socket::set_noblocking(bool _Enabled)  throw(socket_exception)
{
	unsigned long opt = _Enabled;
	if(::_sioct(_fd, FIONBIO,&opt))
		throw socket_exception("error: of set noblocking socket (ioctl)", get_socket_error_code());	
}

size_t ts::socket::bytes_available() throw(socket_exception)
{
	unsigned long bytes_available = 0;
	if(::_sioct(_fd, FIONREAD,&bytes_available))
		throw socket_exception("error: of get bytes_available socket (ioctl)", get_socket_error_code());	
	return bytes_available;
}
