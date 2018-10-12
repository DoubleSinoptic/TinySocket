#include <functional>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>
#include "tinyclient_crc32.inl"
#include <iostream>
#include <thread>
#include <set>
#include "tinyfsssocket.h"

namespace ts
{

#pragma pack(push, 1)
	struct confrim_cmd
	{
		uint32_t command_index;
		ts::command_info command;
		uint32_t command_and_data_length;
	};
	struct big_package
	{
		uint32_t command_index;
		ts::command_info command;
		uint32_t command_and_data_length;
		uint8_t  data[1392];
		void applay_data(const uint8_t* data_, size_t length)
		{
			command_and_data_length = length + sizeof(confrim_cmd);
			if(data_)
				memcpy(data, data_, length);
		}

	};
#pragma pack(pop)
	class fss_socket_impl
	{
	public:
		fss_socket_impl(ts::fss_socket_proc* proc, int blockDelayInMs = 0) :
			_socket(ts::protocol_type::udp),
			_proc(proc)
		{
			if (blockDelayInMs < 0)
				throw std::invalid_argument("block delay in ms invalid time value");
			if (blockDelayInMs == 0)
				_socket.set_noblocking(true);
			else
				_socket.set_receive_time_out(blockDelayInMs);
		}

	
		ts::fss_socket_proc* _proc;
		ts::ip_end_point _serverPoint;

		struct package_state {
			big_package package;
			ts::ip_end_point to;
			std::chrono::steady_clock::time_point creationTime;
		};
		std::vector<package_state> _sendPackeges;
		ts::socket _socket;
	};
}

ts::fss_socket::fss_socket(ts::fss_socket_proc* proc, int blockDelayInMs) :
	_impl(new ts::fss_socket_impl(proc, blockDelayInMs))
{}

ts::fss_socket::~fss_socket()
{
	if(_impl)
		delete _impl;
}

uint32_t ts::cmd_hash(const char * str)
{
	return crc32(0, (const unsigned char*)str, strlen(str));
}

uint32_t ts::cmd_hash(const std::string & str)
{
	return crc32(0, (const unsigned char*)str.c_str(), str.size());
}

ts::socket * ts::fss_socket::get_socket() 
{
	return &_impl->_socket;
}

void ts::fss_socket::bind(const ts::ip_end_point& binder)
{
	_impl->_socket.bind(binder);
}

void ts::fss_socket::set_server(const ts::ip_end_point& srv)
{
	_impl->_serverPoint = srv;
}


void ts::fss_socket::cmd(cmd_index id, const command_info& cmd1, const ts::ip_end_point& to, const uint8_t* data, size_t length)
{
	int index = _impl->_sendPackeges.size();
	_impl->_sendPackeges.resize(index + 1);

	ts::fss_socket_impl::package_state& pack = _impl->_sendPackeges[index];
	pack.package.command = cmd1;
	pack.package.command_index = id;
	pack.package.applay_data(data, length);

	pack.creationTime = std::chrono::steady_clock::now();
	pack.to = to;

	_impl->_socket.send_to_some((void*)&pack.package, pack.package.command_and_data_length, to);
}

void ts::fss_socket::cmd(cmd_index index, const command_info& cmd1, const uint8_t* data, size_t length)
{
	cmd(index, cmd1, _impl->_serverPoint, data, length);
}

void ts::fss_socket::unsafe_cmd(const command_info& cmd1, const ts::ip_end_point& to, const uint8_t* data, size_t length)
{
	ts::fss_socket_impl::package_state pack;
	pack.package.command = cmd1;
	pack.package.command_index = unsafe_index;
	pack.package.applay_data(data, length);

	pack.creationTime = std::chrono::steady_clock::now();
	pack.to = to;

	_impl->_socket.send_to_some((void*)&pack.package, pack.package.command_and_data_length, to);
}

void ts::fss_socket::unsafe_cmd(const command_info& cmd1, const uint8_t* data, size_t length)
{
	unsafe_cmd(cmd1, _impl->_serverPoint, data, length);
}


bool ts::fss_socket::wait_for(uint32_t cmdName, int timeOut)
{
	auto fte = std::chrono::steady_clock::now();
	ip_end_point point;
	while (true)
	{
		for (int i = 0; i < _impl->_sendPackeges.size(); i++)
		{
			if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _impl->_sendPackeges[i].creationTime).count() >= 500)
			{
				_impl->_sendPackeges[i].creationTime = std::chrono::steady_clock::now();
				_impl->_socket.send_to_some((void*)&_impl->_sendPackeges[i].package, _impl->_sendPackeges[i].package.command_and_data_length, _impl->_sendPackeges[i].to);
			}
		}

		ts::big_package pack;
		int sz = _impl->_socket.receive_from_some((void*)&pack, sizeof(ts::big_package), point);


		if (sz < 0)
		{
			if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - fte).count() >= timeOut)
				return false;
			std::this_thread::yield();
			continue;
		}

		if (pack.command.cmd == cmdName)
		{
			if (pack.command_index != unsafe_index)
			{
				struct confrim_cmd
				{
					uint32_t command_index;
					ts::command_info command;
					uint32_t command_and_data_length;
				};
				confrim_cmd conf;
				conf.command.cmd = 0xFFFFFFFF;
				conf.command_and_data_length = sizeof(confrim_cmd);
				conf.command_index = pack.command_index;

				if (_impl->_proc->process(pack.command_index, pack.command, point, pack.data, pack.command_and_data_length - sizeof(confrim_cmd)))
					_impl->_socket.send_to_some((void*)&conf, conf.command_and_data_length, point);

			}
			else
				_impl->_proc->process(pack.command_index, pack.command, point, pack.data, pack.command_and_data_length - sizeof(confrim_cmd));
			return true;
		}
		else if (pack.command.cmd == 0xFFFFFFFF)
		{
			_impl->_sendPackeges.erase(std::remove_if(_impl->_sendPackeges.begin(), _impl->_sendPackeges.end(), [&](ts::fss_socket_impl::package_state& state)
			{
				return state.package.command_index == pack.command_index && state.to == point;
			}), _impl->_sendPackeges.end());
		}


	}
	return false;
}

void ts::fss_socket::receive_and_process(int maxProcessedPackeges)
{

	for (int i = 0; i < _impl->_sendPackeges.size(); i++)
	{
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _impl->_sendPackeges[i].creationTime).count() >= 500)
		{
			_impl->_sendPackeges[i].creationTime = std::chrono::steady_clock::now();
			_impl->_socket.send_to_some((void*)&_impl->_sendPackeges[i].package, _impl->_sendPackeges[i].package.command_and_data_length, _impl->_sendPackeges[i].to);
		}
	}

	ip_end_point point;
	for (int i = 0; i < maxProcessedPackeges; i++)
	{
		ts::big_package pack;
		int sz = _impl->_socket.receive_from_some((void*)&pack, sizeof(ts::big_package), point);
		if (sz < 0)
			break;

		if (pack.command.cmd == 0xFFFFFFFF)
		{
			_impl->_sendPackeges.erase(std::remove_if(_impl->_sendPackeges.begin(), _impl->_sendPackeges.end(), [&](ts::fss_socket_impl::package_state& state)
			{

				return state.package.command_index == pack.command_index && state.to == point;
			}), _impl->_sendPackeges.end());
		}
		else
		{
			if (pack.command_index != unsafe_index)
			{
				struct confrim_cmd
				{
					uint32_t command_index;
					ts::command_info command;
					uint32_t command_and_data_length;
				};
				confrim_cmd conf;
				conf.command.cmd = 0xFFFFFFFF;
				conf.command_and_data_length = sizeof(confrim_cmd);
				conf.command_index = pack.command_index;


				if (_impl->_proc->process(pack.command_index, pack.command, point, pack.data, pack.command_and_data_length - sizeof(confrim_cmd)))
					_impl->_socket.send_to_some((void*)&conf, conf.command_and_data_length, point);
			}
			else
				_impl->_proc->process(pack.command_index, pack.command, point, pack.data, pack.command_and_data_length - sizeof(confrim_cmd));
		

		}
	}
}