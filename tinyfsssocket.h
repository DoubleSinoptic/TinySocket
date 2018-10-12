#ifndef __TINY_FSS_SOCKET_H__
#define __TINY_FSS_SOCKET_H__

#include "Common/Defines.h"
#include "tinysocket.h"
#include "tinybinarystream.h"
#include <functional>

namespace ts 
{
	TS_EXPORT uint32_t cmd_hash(const char* str);
	TS_EXPORT uint32_t cmd_hash(const std::string& str);

	
	typedef uint32_t cmd_index;
	const cmd_index unsafe_index = 0xFFFFFFFF;

	inline cmd_index increment_index(cmd_index& index)
	{
		cmd_index l = index;
		index++;
		if (index == unsafe_index)
			index = 0;
		return l;
	}
	

	inline cmd_index pre_add_index(cmd_index index)
	{
		index++;
		if (index == unsafe_index)
			index = 0;
		return index;
	}

	inline cmd_index pre_increment_index(cmd_index& index)
	{
		index++;
		if (index == unsafe_index)
			index = 0;
		return index;
	}

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

		/*комманда котора€ будет обрабатыватьс€ в network_entity под 
		индексом entity_id, cmd не может быть равен 0xFFFFFFFF  (комманда
		подтверждени€ получени€ пакета)
		 так-как это служебные комманды*/
		uint32_t cmd;
		/*id того юзер€ котоырй создал пакет* а не переотрправил,
		сервер всегда находитс€ под индексом 0*/
		uint32_t user_index;
		/*0 - служебна€ €чейка. используетс€ дл€ валидации. 
		если проходите по всем ентит€м то начинайте с индекса 1*/
		uint32_t entity_id;
	
	};


	class TS_EXPORT fss_socket_proc
	{
	public:
		virtual ~fss_socket_proc() {}
		virtual bool process(uint32_t cmdIndex, const command_info& cmd, const ts::ip_end_point& point, uint8_t* data, size_t length)
		{
			return true;
		}
	};

	class fss_socket_impl;
	class TS_EXPORT fss_socket
	{
		fss_socket_impl* _impl;
	public:
		ts::socket* get_socket();

		void bind(const ts::ip_end_point& binder);

		void set_server(const ts::ip_end_point& srv);

		fss_socket(fss_socket_proc* process, int blockDelayInMs = 0);

		~fss_socket();

		
		void cmd(cmd_index index, const command_info& cmd, const uint8_t* data, size_t length);

		void cmd(cmd_index index, const command_info& cmd, const  ts::ip_end_point& to, const uint8_t* data, size_t length);
	
		void unsafe_cmd(const command_info& cmd, const ts::ip_end_point& to, const uint8_t* data, size_t length);

		void unsafe_cmd(const command_info& cmd, const uint8_t* data, size_t length);

		bool wait_for(uint32_t cmd, int timeOut);

		void receive_and_process(int maxProcessedPackeges = 64);
	};
	
}

#endif