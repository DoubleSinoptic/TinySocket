#ifndef __TINY_BINARY_STREAM_H__
#define __TINY_BINARY_STREAM_H__


#include <string>
#include <vector>
#include <utility>
#include "tinysocket.h"

#ifndef ASSERT_THROW_H
#define ASSERT_THROW_H
#define AssertThrow(x, msg) if(!bool(x)) throw std::runtime_error(msg);
#endif

namespace ts 
{
	
	class binary_stream
	{
		uint16_t capacity;
		uint8_t* start;
		uint16_t index;
	public:
		inline binary_stream(uint8_t* _start, uint16_t _capacity, uint16_t indexator) : 
			start(_start), capacity(_capacity), index(indexator)
		{}

		inline const uint8_t* const_data() const { return start; }
		inline uint8_t* data() { return start; }
		inline const size_t length() const { return capacity; }

		inline size_t stored_length() const { return index; }

		inline void reset()
		{
			index = 0;
		}
		inline void seek_beg(uint16_t off)
		{
			index = off;
		}
		inline void seek_cur(uint16_t off)
		{
			index += off;
		}

		template<typename T>
		inline void write_object(const T& obj)
		{
			AssertThrow((index + sizeof(T)) <= capacity, "buffer io write error buffer out of range");
			uint8_t* pointer = start + index;
			*reinterpret_cast<T*>(pointer) = obj;
			index += sizeof(T);			
		}

		template<typename T>
		inline T read_object()
		{
			AssertThrow((index + sizeof(T)) <= capacity, "buffer io read error buffer out of range");
			uint8_t* pointer = start + index;
			const T val = *reinterpret_cast<const T*>(pointer);
			index += sizeof(T);
			return val;
		}

		void write_data(const uint8_t* dt, size_t length)
		{
			write_uint32(length);
			for (uint32_t i = 0; i < length; i++)
				write_uint8(dt[i]);
		}

		int read_data(uint8_t* dt, size_t capacity)
		{
			uint32_t sz = read_uint32();
			AssertThrow(sz <= capacity, "buffer io write error buffer out of range");
			for (uint32_t i = 0; i < sz; i++)
				dt[i] = read_uint8();
			return sz;
		}
		uint8_t* read_data_pointer(size_t& length)
		{
			length = read_uint32();
			AssertThrow(length <= capacity, "buffer io write error buffer out of range");
			uint8_t* d = start + index;
			seek_cur(length);
			return d;
		}

		void write_vector(const std::vector<uint8_t>& str)
		{
			write_uint32(str.size());
			for (uint32_t i = 0; i < str.size(); i++)
				write_uint8(str[i]);
		}

		std::vector<uint8_t> read_vector()
		{
			uint32_t sz = read_uint32();
			std::vector<uint8_t> plz;
			plz.reserve(sz);
			for (uint32_t i = 0; i < sz; i++)
				plz.push_back(read_uint8());
			return std::move(plz);
		}

		void write_string(const std::string& str)
		{
			write_uint32(str.size());
			for (uint32_t i = 0; i < str.size(); i++)
				write_uint8(str[i]);
		}

		std::string read_string()
		{		
			uint32_t sz = read_uint32();
			std::string plz(sz, 'A');
			for (uint32_t i = 0; i < sz; i++)
				plz[i] = read_uint8();
			return std::move(plz);
		}


		inline std::int8_t read_int8() { return read_object<std::int8_t>(); }
		inline std::int16_t read_int16() { return read_object<std::int16_t>(); }
		inline std::int32_t read_int32() { return read_object<std::int32_t>(); }
		inline std::int64_t read_int64() { return read_object<std::int64_t>(); }

		inline std::uint8_t read_uint8() { return read_object<std::uint8_t>(); }
		inline std::uint16_t read_uint16() { return read_object<std::uint16_t>(); }
		inline std::uint32_t read_uint32() { return read_object<std::uint32_t>(); }
		inline std::uint64_t read_uint64() { return read_object<std::uint64_t>(); }

		inline void write_int8(std::int8_t	d) { write_object(d); }
		inline void write_int16(std::int16_t d) { write_object(d); }
		inline void write_int32(std::int32_t d) { write_object(d); }
		inline void write_int64(std::int64_t d) { write_object(d); }

		inline void write_uint8(std::uint8_t d) { write_object(d); }
		inline void write_uint16(std::uint16_t d) { write_object(d); }
		inline void write_uint32(std::uint32_t d) { write_object(d); }
		inline void write_uint64(std::uint64_t d) { write_object(d); }
	};

	const binary_stream null_binary_stream = { nullptr, 0, 0 };

	template<uint16_t Reserv>
	class package_stream : public binary_stream
	{
		uint8_t xdata[Reserv];
	public:
		package_stream() : binary_stream(xdata, Reserv, 0)
		{}
	};

	template<size_t N>
	class write_caster
	{
		char buffer[N];
		int current;
	public:
		write_caster() : current(0) {}

		template<typename T>
		void write(char* data, size_t ln, const T& callback)
		{
			while (ln)
			{
				char* currentBuffer = buffer + current;
				size_t currentBufferLength = N - current;
				int affectiveLength = ln < currentBufferLength ? ln : currentBufferLength;
				memcpy(currentBuffer, data, affectiveLength);
				ln -= affectiveLength;
				data += affectiveLength;
				current += affectiveLength;
				if (current == N)
				{
					callback(buffer, current);
					current = 0;
				}
			}
		}

		template<typename T>
		void flush(const T& callback) {
			if (current != 0)
				callback(buffer, current);
		}

	};
}

#endif