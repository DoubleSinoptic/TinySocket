#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "tinysocket.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mutex>

#if defined(_WIN32)
	#include <WinSock2.h>
	#include <Ws2tcpip.h>
	#include <Windows.h>
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
			_isInit = false;
			if (::WSAStartup( MAKEWORD(2, 2) , &_data) != 0)
			{
				std::sprintf(exception_buffer, "error: WSA startup failed: error code: %d", ::WSAGetLastError());
				throw std::runtime_error(exception_buffer);
			}
			_isInit = true;
		}
		~ws2data_quard()
		{
			std::lock_guard<std::mutex> _(_lockRoot);
			if (_isInit)
				::WSACleanup();
		}

		void chek() 
		{
			std::lock_guard<std::mutex> _(_lockRoot);
			if(!_isInit)
				throw std::runtime_error("error: WSA not startup");
		}

	private:
		std::mutex _lockRoot;
		bool _isInit;
		WSADATA _data;
	} _wsintance;

	#define CHEK_SOCKET _wsintance.chek()
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
	#include <netdb.h>

	#define _sioct ioctl
	#define _sclose close
	#define _ssuberrorconst 88 

	#define CHEK_SOCKET

	#define INVALID_SOCKET -1	
#endif

#include "tinysocket_parser.inl"

ts::socket_native_error_code get_socket_error_code()
{
#if defined(_WIN32)
	return ::WSAGetLastError();
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

ts::socket_exception::socket_exception(const char * message)
	: _code(-1)
{
	snprintf(_exceptionMessgeBuffer, sizeof(_exceptionMessgeBuffer),
		"%s", message);

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
	case ts::address_famaly::internet_network_v6:
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
		return ts::address_famaly::internet_network_v6;
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

ts::ip_address::ip_address(const ip_part * _Address)
{
	_address = htonl(*((uint32_t*)_Address));
}

ts::ip_address::ip_address(uint32_t _NativeHotPost)
{
	_address = _NativeHotPost;
}

ts::ip_address::ip_address(ip_part _Part0, ip_part _Part1, ip_part _Part2, ip_part _Part3)
	: _address(0)
{
	uint32_t rezaddr;
	ip_part* p = (ip_part*)&rezaddr;
	rezaddr[0] = _Part0;
	rezaddr[1] = _Part1;
	rezaddr[2] = _Part2;
	rezaddr[3] = _Part3;
	_address = htonl(rezaddr);
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
	: _remoteEndpoint(ts::ip_end_point(ts::ip_address_none, 0)), _isConnected(false), _isListening(false)
{
	CHEK_SOCKET;

	_fd = ::socket(
		native_enum_address_famaly(_Famaly), 
		native_enum_socket_type(_SocketTpye),
		native_enum_protocol_type(_ProtocolType));

	if (_fd == -1) 
		throw socket_exception("error: of create socket", get_socket_error_code());
}

ts::socket::socket(ts::protocol_type _ProtocolType) throw(socket_exception)
	: _remoteEndpoint(ts::ip_end_point(ts::ip_address_none, 0)), _isConnected(false), _isListening(false)
{
	CHEK_SOCKET;

	address_famaly fam = address_famaly::internet_network;
	socket_type type;
	switch (_ProtocolType)
	{
	case ts::protocol_type::tcp:
		type = ts::socket_type::stream;
		break;
	case ts::protocol_type::udp:
		type = ts::socket_type::dgram;
		break;
	case ts::protocol_type::ip:
		type = ts::socket_type::dgram;
		break;
	case ts::protocol_type::raw:
		type = ts::socket_type::raw;
		break;
	default:
		throw socket_exception("error: invalid protocol type", -1);
	}

	_fd = ::socket(
		native_enum_address_famaly(fam),
		native_enum_socket_type(type),
		native_enum_protocol_type(_ProtocolType));

	if (_fd == -1)
		throw socket_exception("error: of create socket", get_socket_error_code());
}

ts::socket::socket(socket_native_fd _NativeFd, const ip_end_point& _RemoteAddres) throw(socket_exception)
	: _remoteEndpoint(ts::ip_end_point(ts::ip_address_none, 0)), _isConnected(false), _isListening(false)
{
	_fd = _NativeFd;
	_remoteEndpoint = _RemoteAddres;
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
	_isListening = true;
}

void ts::socket::bind(const ip_end_point & _EndPoint) throw(socket_exception)
{
	if (::bind(_fd, (sockaddr*)_EndPoint.native_address(), _EndPoint.native_size()))
	{
		throw socket_exception("error: of bind socket", get_socket_error_code());
	}
}

ts::socket_native_fd ts::socket::get_native_fd()
{
	return _fd;
}

void ts::socket::set_receive_time_out(ms_time_out mssec) throw(socket_exception)
{
	struct timeval tv;

	tv.tv_sec = mssec;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
	
	if (::setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval))) 
		throw socket_exception("error: socket: of set option to", get_socket_error_code());
}

void ts::socket::set_send_time_out(ms_time_out mssec) throw(socket_exception)
{
	struct timeval tv;

	tv.tv_sec = mssec;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors

	if(::setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval)))
		throw socket_exception("error: socket: of set option to", get_socket_error_code());
}

void ts::socket::tcp_no_delay(bool enabled) throw(ts::socket_exception)
{
	int flag = enabled ? 1 : 0;
	if (::setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)))
		throw socket_exception("error: socket: of set option to", get_socket_error_code());
}

void ts::socket::connect(const ip_end_point & _To) throw(socket_exception)
{
	if (::connect(_fd, (sockaddr*)_To.native_address(), _To.native_size()))
		throw socket_exception("error: socket: of connect to", get_socket_error_code());	
	_isConnected = true;
}

int ts::socket::send_some(const void * _Data, size_t _DataLen, socket_flags flags) throw()
{
	totalBytesSended += _DataLen;
	return ::send(_fd, (const char*)_Data, _DataLen, (int)flags);
}

int ts::socket::receive_some(void * _Data, size_t _DataLen, socket_flags flags) throw()
{
	totalBytesReceived += _DataLen;
	return ::recv(_fd, (char*)_Data, _DataLen, (int)flags);
}

int ts::socket::send_to_some(const void * _Data, size_t _DataLen, const ip_end_point & _To, socket_flags flags) throw()
{
	totalBytesSended += _DataLen;
	return ::sendto(_fd, (const char*)_Data, _DataLen, (int)flags, (sockaddr*)_To.native_address(), _To.native_size());
}

int ts::socket::receive_from_some(void * _Data, size_t _DataLen, ip_end_point & _From, socket_flags flags) throw()
{
	totalBytesReceived += _DataLen;
	return ::recvfrom(_fd, (char*)_Data, _DataLen, (int)flags, (sockaddr*)_From.native_address(), (socklen_t*)&_From.native_size());
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

size_t ts::socket::send_to(const void * _Data, size_t _DataLen, const ip_end_point & _To, socket_flags flags) throw(socket_exception)
{
	totalBytesSended += _DataLen;
	int rret = ::sendto(_fd, (const char*)_Data, _DataLen, (int)flags, (sockaddr*)_To.native_address(), _To.native_size());
	if (rret < 0) 
		throw socket_exception("error: socket: of send data to endpoint", get_socket_error_code());
	return rret;
}

size_t ts::socket::receive_from(void * _Data, size_t _DataLen, ip_end_point & _From, socket_flags flags) throw(socket_exception)
{
	totalBytesReceived += _DataLen;

	int rret = ::recvfrom(_fd, (char*)_Data, _DataLen, (int)flags, (sockaddr*)_From.native_address(), (socklen_t*)&_From.native_size());
	if (rret < 0) 
		throw socket_exception("error: socket: of receive from data from endpoint", get_socket_error_code());
	return rret;
}

ts::socket ts::socket::accept() throw(socket_exception)
{
	ip_end_point a;
	int e = ::accept(_fd, (sockaddr*)a.native_address(), (socklen_t*)&a.native_size());
	if (e == INVALID_SOCKET)
		throw socket_exception("error: of accept socket", get_socket_error_code());	
	return socket(e, a);
}

ts::socket * ts::socket::accept_new() throw(socket_exception)
{
	ip_end_point a(ip_address_none, 0);
	ts::socket_native_fd e = ::accept(_fd, (sockaddr*)a.native_address(), (socklen_t*)&a.native_size());
	if (e == -1)
		throw socket_exception("error: of accept socket", get_socket_error_code());
	return new socket(e, a);
}

void ts::socket::close()
{
	if (_fd == 0)
		return;
	if(_isConnected || _isListening)
		::shutdown(_fd, static_cast<int>(socket_shutdown::both));
	::_sclose(_fd);
	_fd = 0;
}

void ts::socket::shutdown(socket_shutdown _O) throw(socket_exception)
{
	if(::shutdown(_fd, static_cast<int>(_O)))
		throw socket_exception("error: of shutdown socket", get_socket_error_code());
}

ts::ip_end_point ts::socket::remote_endpoint()
{
	return _remoteEndpoint;
}

void  ts::socket::set_noblocking(bool _Enabled)  throw(socket_exception)
{
        unsigned long opt = _Enabled ? 1 : 0;
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

bool ts::socket::is_connected() const
{
	return _isConnected;
}

bool ts::socket::is_listening() const
{
	return _isListening;
}

ts::ip_end_point::ip_end_point(ip_address _Address, port port)
{
	memset(_address, 0, sizeof(_address));
	_address_size = sizeof(sockaddr_in);
	sockaddr_in* in = (sockaddr_in*)_address;
	in->sin_family = AF_INET;
	in->sin_port = htons(port);
	in->sin_addr.s_addr = _Address.native_address();
}

ts::ip_end_point::ip_end_point(ip_address_v6 _Address, port port)
{
	memset(_address, 0, sizeof(_address));
	_address_size = sizeof(sockaddr_in6);
	sockaddr_in6* in = (sockaddr_in6*)_address;
	in->sin6_family = AF_INET6;
	in->sin6_port = htons(port);
	memcpy(in->sin6_addr.s6_addr, _Address.native_address(), 16);
}

ts::ip_end_point::ip_end_point()
{
	memset(_address, 0, sizeof(_address));
	_address_size = sizeof(_address);
}

ts::address_famaly ts::ip_end_point::get_famaly() const
{
	return un_native_enum_address_famaly(((sockaddr*)_address)->sa_family);
}

const void * ts::ip_end_point::native_address() const
{
	return _address;
}

void * ts::ip_end_point::native_address()
{
	return _address;
}

std::uint32_t & ts::ip_end_point::native_size()
{
	return _address_size;
}
std::uint32_t ts::ip_end_point::native_size() const
{
	return _address_size;
}
const ts::ip_address & ts::ip_end_point::get_v4_address() const
{
	return *((ts::ip_address*)&(((sockaddr_in*)_address)->sin_addr.s_addr));
}

const ts::ip_address_v6 & ts::ip_end_point::get_v6_address() const
{
	return *((ts::ip_address_v6*)(((sockaddr_in6*)_address)->sin6_addr.s6_addr));
}

ts::port ts::ip_end_point::get_port() const
{
	return htons(((sockaddr_in*)_address)->sin_port);
}

bool ts::ip_end_point::equal(const ip_end_point & of) const
{
	if (get_famaly() == of.get_famaly())
	{
		if (get_famaly() == ts::address_famaly::internet_network)
			return (get_port() == of.get_port()) && (of.get_v4_address() == get_v4_address());
		if (get_famaly() == ts::address_famaly::internet_network_v6)
			return (get_port() == of.get_port()) && (of.get_v6_address() == get_v6_address());	
	}
	return false;

}

bool ts::ip_end_point::operator!=(const ip_end_point & of) const
{
	return !equal(of);
}

bool ts::ip_end_point::operator==(const ip_end_point & of) const
{
	return equal(of);
}

ts::ip_address_v6 ts::ip_address_v6::from_string(const std::string & _String)
{
	ip_part tmp[16];
	if (::ts_inet_pton(native_enum_address_famaly(ts::address_famaly::internet_network_v6), _String.c_str(), &tmp) <= 0)
		throw socket_exception("error: ivalid address format");
	return ts::ip_address_v6(tmp);
}

ts::ip_address ts::ip_address::from_string(const std::string & _String)
{
	ip_part tmp[4];
	if(::ts_inet_pton(native_enum_address_famaly(ts::address_famaly::internet_network), _String.c_str(), &tmp) <= 0)
		throw socket_exception("error: ivalid address format");
	return ts::ip_address(tmp);
}

std::ostream & ts::operator<<(std::ostream & _Stream, const ts::ip_address & _Addr)
{
	char tmp[256];
	if(::ts_inet_ntop(native_enum_address_famaly(ts::address_famaly::internet_network), &_Addr, tmp, sizeof(tmp)) == nullptr)
		throw ts::socket_exception("error: put address to stream");
	_Stream << tmp;
	return _Stream;
}

std::ostream & ts::operator<<(std::ostream & _Stream, const ts::ip_address_v6 & _Addr)
{
	char tmp[256];
	if(::ts_inet_ntop(native_enum_address_famaly(ts::address_famaly::internet_network_v6), &_Addr, tmp, sizeof(tmp)))
		throw ts::socket_exception("error: put address to stream");
	_Stream << tmp;
	return _Stream;
}

std::ostream & ts::operator<<(std::ostream & _Stream, const ip_end_point & _Addr)
{
	if (_Addr.get_famaly() == ts::address_famaly::internet_network)
	{
		_Stream << _Addr.get_v4_address() << ":" << _Addr.get_port();
	}
	if (_Addr.get_famaly() == ts::address_famaly::internet_network_v6)
	{
		_Stream << '[' << _Addr.get_v6_address() << "]:" << _Addr.get_port();
	}
	return _Stream;
}

ts::dns_entry ts::dns_entry::get_host_by_name(const std::string & _Name)
{
	return get_host_by_name(_Name.c_str());
}

ts::dns_entry ts::dns_entry::get_host_by_name(const char * _Name)
{
	CHEK_SOCKET;
	hostent* s = gethostbyname(_Name);
	if (s == nullptr)
		throw socket_exception("error: ivalid host name or domain name not found");	
	ts::dns_entry rezult;
	if(s->h_name != nullptr)
		rezult._name = s->h_name;
	if (s->h_addrtype == native_enum_address_famaly(ts::address_famaly::internet_network))
	{	
		rezult._fam = ts::address_famaly::internet_network;
		ts::ip_address** addr = reinterpret_cast<ts::ip_address**>(s->h_addr_list);
		while (*addr != nullptr)
		{	
    		rezult._v4addresses.push_back(**addr);
			addr++;
		}
	}
	if (s->h_addrtype == native_enum_address_famaly(ts::address_famaly::internet_network_v6))
	{
		rezult._fam = ts::address_famaly::internet_network_v6;
		ts::ip_address_v6** addr = reinterpret_cast<ts::ip_address_v6**>(s->h_addr_list);
		while (*addr != nullptr)
		{
			rezult._v6addresses.push_back(**addr);
			addr++;
		}
	}
	return rezult;

}
