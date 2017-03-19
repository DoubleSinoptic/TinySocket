#ifndef __TINY_SOCKET_H__
#define __TINY_SOCKET_H__

#include <ostream>
#include <cstdint>
#include <exception>
#include <mutex>
#include <utility>
#include <cstring>

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

	typedef uint8_t ip_part;

	typedef int64_t socket_native_fd;

	class ip_address_v6
	{
	public:
		explicit ip_address_v6(
			ip_part part0, ip_part part1, ip_part part2, ip_part part3,
			ip_part part4, ip_part part5, ip_part part6,ip_part part7,
			ip_part part8, ip_part part9, ip_part part10, ip_part part11,
			ip_part part12, ip_part part13, ip_part part14, ip_part part15)
		{
			_address[0] = part0;
			_address[1] = part1;
			_address[2] = part2;
			_address[3] = part3;

			_address[4] = part4;
			_address[5] = part5;
			_address[6] = part6;
			_address[7] = part7;

			_address[8] = part8;
			_address[9] = part9;
			_address[10] = part10;
			_address[11] = part11;

			_address[12] = part12;
			_address[13] = part13;
			_address[14] = part14;
			_address[15] = part15;
		}
		explicit ip_address_v6(ip_part* value) 
		{
			std::memcpy(_address, value, 16);
		}

		explicit ip_address_v6(const char* _Address) 
		{
			throw socket_exception("", -1);
		}

		ip_address_v6(const ip_address_v6& val) 
		{
			std::memcpy(_address, val._address, 16);
		}

		friend std::ostream& operator <<(std::ostream& _Stream, const ip_address_v6& _Address) 
		{
			for (int i = 0; i < 16; i++)
				_Stream << std::hex << _Address._address[i] << (i != 15) ? ":" : "";
			return _Stream;
		}

		const ip_part* native_address() const
		{
			return _address;
		}

		ip_address_v6& operator=(const ip_address_v6& val)
		{
			std::memcpy(_address, val._address, 16);
			return *this;
		}

		bool operator !=(const ip_address_v6& of) const 
		{ 
			return std::memcmp(_address, of._address, 16);
		}
		bool operator ==(const ip_address_v6& of) const
		{ 
			return !std::memcmp(_address, of._address, 16);
		}

	private:
		ip_part _address[16];
	};

	class ip_address
	{
	public:

		explicit ip_address(ip_part _A, ip_part _B, ip_part _C, ip_part _D);

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
		bool operator !=(const ip_address& of) const { return _address != of._address; }
		bool operator ==(const ip_address& of) const { return _address == of._address; }

	private:
		uint32_t _address;
	};

	std::ostream& operator <<(std::ostream& _Stream, const ip_address& _Address);

	const ip_address ip_address_any(0, 0, 0, 0);

	const ip_address ip_address_loopback(0x7F, 0, 0, 0);

	const ip_address ip_address_broadcast(0xFF, 0xFF, 0xFF, 0xFF);

	const ip_address ip_address_none(0xFF, 0xFF, 0xFF, 0xFF);

	typedef std::uint16_t port;
	
	enum class address_famaly : int
	{
		internet_network,
		internet_network_ipv6
	};

	enum class socket_type : int
	{
		stream,
		dgram,
		raw
	};

	enum class protocol_type : int
	{
		tcp,
		udp,
		ip,
		raw
	};


	class ip_end_point 
	{
	public:
		ip_end_point(ip_address _Address, port port);
		ip_end_point(ip_address_v6 _Address, port port);

		ip_end_point(const ip_end_point& _Address) = default;
		ip_end_point& operator =(const ip_end_point& _Address) = default;

		address_famaly get_famaly() const;

		const void* native_address() const;
		void* native_address();
		std::uint32_t& native_size();
		std::uint32_t native_size() const;

		const ip_address& get_v4_address() const;
		const ip_address_v6& get_v6_address() const;

		port get_port() const;

		bool equal(const ip_end_point& of) const;

		bool operator !=(const ip_end_point& of) const;
		bool operator ==(const ip_end_point& of) const;
	private:
		std::uint32_t _address_size;
		std::uint8_t _address[32];
	};

	enum class socket_shutdown : int
	{
		recive = 0,
		send = 1,
		both = 2
	};

	enum class socket_flags : int
	{
		none = 0x0000,
        out_of_band = 0x0001,
        peek = 0x0002,
        dont_route = 0x0004,
        truncated = 0x0100,
        control_data_truncated = 0x0200,
        broadcast = 0x0400,
        multicast = 0x0800,
        partial = 0x8000,
	};

	inline socket_flags operator & (socket_flags lhs, socket_flags rhs)
	{
		return (socket_flags)(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
	}

	inline socket_flags& operator &= (socket_flags& lhs, socket_flags rhs)
	{
		lhs = (socket_flags)(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
		return lhs;
	}

	inline socket_flags operator | (socket_flags lhs, socket_flags rhs)
	{
		return (socket_flags)(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
	}

	inline socket_flags& operator |= (socket_flags& lhs, socket_flags rhs)
	{
		lhs = (socket_flags)(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
		return lhs;
	}

	
	class socket
	{
	public:

		static std::size_t get_total_bytes_sended();

		static std::size_t get_total_bytes_received();

		typedef int ms_time_out;

		explicit socket(ts::address_famaly _Famaly, ts::socket_type _SocketTpye, ts::protocol_type _ProtocolType) throw(socket_exception);

		explicit socket(socket_native_fd _NativeFd, const ip_end_point& _RemoteAddres) throw(socket_exception);

		~socket();

		void listen(int _MaxConnections) throw(socket_exception);

		void bind(const ip_end_point& _EndPoint) throw(socket_exception);

		socket_native_fd get_native_fd();

		void set_receive_time_out(ms_time_out _Msec) throw(socket_exception);

		void set_send_time_out(ms_time_out _Msec) throw(socket_exception);

		/*void set_option(int _OptionName, int _OptionLevel, base_option* option);

		base_option* get_option(int _OptionName, int _OptionLevel);*/

		void connect(const ip_end_point& _To) throw(socket_exception);

		size_t send(const void* _Data, size_t _DataLen, socket_flags _Flags = socket_flags::none) throw(socket_exception);

		size_t receive(void* _Data, size_t _DataLen, socket_flags _Flags = socket_flags::none) throw(socket_exception);
		
		size_t send_to(const void* _Data, size_t _DataLen, const ip_end_point& _To, socket_flags _Flags = socket_flags::none) throw(socket_exception);

		size_t receive_from(void* _Data, size_t _DataLen, ip_end_point& _From, socket_flags _Flags = socket_flags::none) throw(socket_exception);

	    int send_some(const void* _Data, size_t _DataLen, socket_flags _Flags = socket_flags::none) throw(socket_exception);

		int receive_some(void* _Data, size_t _DataLen, socket_flags _Flags = socket_flags::none) throw(socket_exception);
		
		int send_to_some(const void* _Data, size_t _DataLen, const ip_end_point& _To, socket_flags _Flags = socket_flags::none) throw(socket_exception);

		int receive_from_some(void* _Data, size_t _DataLen, ip_end_point& _From, socket_flags _Flags = socket_flags::none) throw(socket_exception);

		socket accept() throw(socket_exception);

		socket* accept_new() throw(socket_exception);

		void tcp_no_delay(bool enabled)  throw(socket_exception);	

		void close();

		void shutdown(socket_shutdown _O) throw(socket_exception);

		ts::ip_end_point remote_endpoint();
		
		void set_noblocking(bool _Enabled) throw(socket_exception);
		
		size_t bytes_available() throw(socket_exception);
	private:
		
		ts::ip_end_point _endpoint;

		socket_native_fd _fd;
	};




}

#endif //__TINY_SOCKET_H__
