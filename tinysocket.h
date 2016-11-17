#ifndef __TINY_SOCKET_H__
#define __TINY_SOCKET_H__

#include <ostream>
#include <cstdint>
#include <exception>
#include <mutex>
#include <utility>

namespace ts
{
	typedef int64_t socket_native_error_code;

	class socket_exception : public std::exception
	{
		char _exceptionMessgeBuffer[2048];

		socket_native_error_code _code;

	public:
		
		socket_exception(const char* message, socket_native_error_code code);

		virtual const char* what() const throw();

		socket_native_error_code formated_error_code() const throw();

		socket_native_error_code system_error_code() const throw();
	};

	typedef uint8_t ip4_part;

	typedef int64_t socket_native_fd;

	class ip_address
	{
	public:

		explicit ip_address(ip4_part _A, ip4_part _B, ip4_part _C, ip4_part _D);

		explicit ip_address(const char* _Address);

		explicit ip_address(uint32_t _NativeHotPost);

		ip_address(const ip_address& val)
			: _address(val._address)
		{}

		friend std::ostream& operator <<(std::ostream& _Stream, const ip_address& _Address);

		uint32_t native_address() const
		{
			return _address;
		}

		ip_address& operator=(const ip_address& val) 
		{
			_address = val._address;
			return *this;
		}
		bool operator !=(const ip_address& of) { return _address != of._address; }
		bool operator ==(const ip_address& of) { return _address == of._address; }

	private:
		uint32_t _address;
	};

	std::ostream& operator <<(std::ostream& _Stream, const ip_address& _Address);

	const ip_address ip_address_any(0, 0, 0, 0);

	const ip_address ip_address_loopback(0x7F, 0, 0, 0);

	const ip_address ip_address_broadcast(0xFF, 0xFF, 0xFF, 0xFF);

	const ip_address ip_address_none(0xFF, 0xFF, 0xFF, 0xFF);

	class port
	{
	public:
		port()
			: _port(0)
		{}

		port(const port& val)
			: _port(val._port)
		{}

		bool operator !=(const port& of) { return _port != of._port; }
		bool operator ==(const port& of) { return _port == of._port; }

		port& operator=(const port& val)
		{
			_port = val._port;
			return *this;
		}

		port(int _Port);
		
		inline uint16_t native_port() const
		{
			return _port;
		}

		friend std::ostream& operator <<(std::ostream& _Stream, const port& _Port);
	private:

		uint16_t _port;
	};

	std::ostream& operator <<(std::ostream& _Stream, const port& _Port);

	enum address_famaly : int
	{
		internet_network,
		internet_network_ipv6
	};

	enum socket_type : int
	{
		stream,
		dgram
	};

	enum protocol_type : int
	{
		tcp,
		udp
	};

	class socket_address
	{
	public:
		virtual ~socket_address() {}

		socket_address(const socket_address&) = default;

		socket_address& operator=(const socket_address&) = default;

		socket_address()
			: _famaly(internet_network)
		{}

		address_famaly get_famaly() const
		{
			return _famaly;
		}

		address_famaly set_famaly(const address_famaly& _Famaly)
		{
			return _famaly = _Famaly;
		}

		virtual void serialaze(void* _Dest) const = 0;

		virtual void deserialaze(const void* _Src) = 0;
	private:

		address_famaly _famaly;
	};

	class ip_socket_address : public socket_address
	{
	public:
		~ip_socket_address() {}

		ip_socket_address(const ip_socket_address& val)
			: _address(val._address),
			_port(val._port)
		{
			set_famaly(val.get_famaly());
		}

		ip_socket_address& operator=(const ip_socket_address& val)
		{
			_port = val._port;
			_address = val._address;
			return *this;
		}

		bool operator !=(const ip_socket_address& of) 
		{
			return
				(_port != of._port) &&
				(_address != of._address) &&
				(get_famaly() != of.get_famaly());
		
		}
		bool operator ==(const ip_socket_address& of) {
			return
				(_port == of._port) &&
				(_address == of._address) &&
				(get_famaly() == of.get_famaly());
		}

		ip_socket_address(ip_address _Ip, port _Port)
			: _address(_Ip), _port(_Port)
		{}

		port get_port() const
		{
			return _port;
		}

		port set_port(const port& _Port)
		{
			return _port = _Port;
		}

		ip_address get_address() const
		{
			return _address;
		}

		ip_address set_address(const ip_address& _Address)
		{
			return _address = _Address;
		}

		virtual void serialaze(void* _Dest) const;

		virtual void deserialaze(const void* _Src);
	private:

		port _port;

		ip_address _address;
	};

	enum class socket_shutdown : int
	{
		recive = 0,
		send = 1,
		both = 2
	};

	class socket
	{
	public:

		static std::size_t get_total_bytes_sended();

		static std::size_t get_total_bytes_received();

		typedef int ms_time_out;

		explicit socket(ts::address_famaly _Famaly, ts::socket_type _SocketTpye, ts::protocol_type _ProtocolType) throw(socket_exception);

		explicit socket(socket_native_fd _NativeFd, const ip_socket_address& _RemoteAddres) throw(socket_exception);

		~socket();

		void listen(int _MaxConnections) throw(socket_exception);

		void bind(const socket_address& _EndPoint) throw(socket_exception);

		socket_native_fd get_native_fd();

		void set_receive_time_out(ms_time_out _Msec) throw(socket_exception);

		void set_send_time_out(ms_time_out _Msec) throw(socket_exception);

		/*void set_option(int _OptionName, int _OptionLevel, base_option* option);

		base_option* get_option(int _OptionName, int _OptionLevel);*/

		void connect(const socket_address& _To) throw(socket_exception);

		size_t send(const void* _Data, size_t _DataLen) throw(socket_exception);

		size_t receive(void* _Data, size_t _DataLen) throw(socket_exception);
		
		size_t send_to(const void* _Data, size_t _DataLen, const socket_address& _To) throw(socket_exception);

		size_t receive_from(void* _Data, size_t _DataLen, socket_address& _From) throw(socket_exception);

		socket accept() throw(socket_exception);

		socket* accept_new() throw(socket_exception);

		void close();

		void shutdown(socket_shutdown _O) throw(socket_exception);

		ts::ip_socket_address remote_endpoint();
		
		void set_noblocking(bool _Enabled) throw(socket_exception);
		
		size_t bytes_available() throw(socket_exception);
	private:
		
		ts::ip_socket_address _endpoint;

		socket_native_fd _fd;
	};




}

#endif //__TINY_SOCKET_H__
