#ifndef __TINY_FSS_SOCKET_H__
#define __TINY_FSS_SOCKET_H__

#include "tinysocket.h"
#include "tinybinarystream.h"
#include <functional>

namespace ts 
{
	uint32_t cmd_hash(const char* str);
	uint32_t cmd_hash(const std::string& str);

	struct command_info
	{
		command_info(
			uint32_t _cmd,
			uint32_t _user_index,
			uint32_t _entity_id
		) :
			cmd(_cmd), user_index(_user_index), entity_id(_entity_id)
		{}

		command_info(
			uint32_t _cmd
		) :
			cmd(_cmd), user_index(0), entity_id(0)
		{}

		command_info(
			uint32_t _cmd,
			uint32_t _user_index
		) :
			cmd(_cmd), user_index(_user_index), entity_id(0)
		{}

		command_info() :
			cmd(0), user_index(0), entity_id(0)
		{}


		uint32_t cmd;
		uint32_t user_index;
		uint32_t entity_id;
	};


	class fss_socket_proc 
	{
	public:
		virtual ~fss_socket_proc() {}
		virtual bool process(const command_info& cmd, const ts::ip_end_point& point, uint8_t* data, size_t length)
		{
			return true;
		}
	};

	class fss_socket_impl;
	class fss_socket 
	{
		fss_socket_impl* _impl;
	public:
		ts::socket* get_socket();

		void bind(const ts::ip_end_point& binder);

		void set_server(const ts::ip_end_point& srv);

		fss_socket(fss_socket_proc* process, int blockDelayInMs = 0);

		~fss_socket();

		
		void cmd(const command_info& cmd, const uint8_t* data, size_t length);

		void cmd(const command_info& cmd, const  ts::ip_end_point& to, const uint8_t* data, size_t length);
	
		void unsafe_cmd(const command_info& cmd, const ts::ip_end_point& to, const uint8_t* data, size_t length);

		void unsafe_cmd(const command_info& cmd, const uint8_t* data, size_t length);

		bool wait_for(uint32_t cmd, int timeOut);

		void receive_and_process(int maxProcessedPackeges = 64);
	};
	
}

#endif