#ifndef __TINY_RPC_H__
#define __TINY_RPC_H__

#include "tinyfsssocket.h"
#include <chrono>
#include <iostream>
#include <map>

#define RPC_ENTITY_ARGS ts::user_id calledUser, uint32_t rpcName, ts::binary_stream& arguments

namespace ts 
{
	typedef uint32_t entity_id;
	typedef uint32_t user_id;

	typedef std::function<void(ts::binary_stream& arguments)>						rpc_callback;
	typedef std::function<void(const class rpc_user* user, ts::binary_stream& arguments)> rpc_callback_net_user;

	class TS_EXPORT rpc_client
	{
		std::chrono::steady_clock::time_point update_timer;
		class TS_EXPORT rpc_client_proc : public ts::fss_socket_proc
		{
		public:
			bool _is_connected;
			user_id _user_id;
			ts::fss_socket _socket;
			ts::ip_end_point _serverPoint;
			rpc_client_proc(int delayMs);

			cmd_index send_next = 0;
			cmd_index recv_next = 0;

			bool process(uint32_t cmdIndex, const ts::command_info& cmd, const ts::ip_end_point& point, uint8_t* data, size_t length) override;
			friend class rpc_entity;
		} proc;
		friend class rpc_entity;
	
	public:
		rpc_client(int delayMs);
		
		~rpc_client();

		user_id get_my_id() const;

		void connect(const ts::ip_end_point& point);

		void disconnect();

		void process_commands(int maxCommands = 64);

		std::function<void(rpc_client*, ts::entity_id id)> on_out_of_range_id;
	};

	struct rpc_user 
	{
		ts::ip_end_point external_address;
		std::chrono::steady_clock::time_point last_packege;
		/*индекс в масиве пользователей*/
		uint32_t index;

		cmd_index send_next = 0;
		cmd_index recv_next = 0;
	};

	class TS_EXPORT rpc_listener
	{
		std::chrono::steady_clock::time_point update_timer;

		class TS_EXPORT rpc_listener_proc : public ts::fss_socket_proc
		{
			std::vector<rpc_user*>  _users;
			ts::fss_socket _socket;
			rpc_listener* _meListener;
		public:
			size_t users_count() const;

			rpc_listener_proc(int delayMs, uint32_t maxUsers, rpc_listener* meListener);

			uint32_t allocate_user_id();

			void delete_user(uint32_t i);

			bool check_user_id(uint32_t id);

			void check_user_alive();

			bool process(uint32_t cmdIndex, const ts::command_info& cmd, const ts::ip_end_point& point, uint8_t* data, size_t length) override;

			friend class rpc_listener;
			friend class rpc_entity;
		} proc;
		friend class rpc_entity;
	public:
		size_t users_count() const;

		rpc_listener(int delayMs, int maxUsers);

		void bind(const ts::ip_end_point& point);
	
		void process_commands(int maxCommands = 64);

		std::function<void(rpc_listener*, rpc_user*)> on_client_error;
		std::function<void(rpc_listener*, rpc_user*)> on_client_connected;
		std::function<void(rpc_listener*, rpc_user*)> on_client_disconnected;
	};

	TS_EXPORT void insert_rpc_client(rpc_client* client);
	TS_EXPORT void insert_rpc_server(rpc_listener* server);
	TS_EXPORT user_id get_current_user_id();
	TS_EXPORT void clear_entities();
	TS_EXPORT size_t users_count();
	/*	
	в масиве всегда будет 1 ентити. это служебный
	нулевой. его не трогать и не изменять
	*/
	TS_EXPORT class rpc_entity** begin_entities();
	TS_EXPORT size_t entities_count();

	class TS_EXPORT rpc_entity
	{
		entity_id _thisId;
	public:
		rpc_entity();

		virtual ~rpc_entity();

		void set_id(entity_id id);

		entity_id allocate_id_and_set();

		static entity_id allocate_id();

		entity_id get_id() const;

		virtual void rpc(user_id calledUser, uint32_t rpcName, ts::binary_stream& arguments) = 0;

		virtual void on_deleted();

		void call(uint32_t rpcName, user_id userId, ts::binary_stream& argsuments);

		void call_unsafe(uint32_t rpcName, user_id userId, ts::binary_stream& argsuments);

		void call(uint32_t rpcName, ts::binary_stream& argsuments);

		void call_unsafe(uint32_t rpcName, ts::binary_stream& argsuments);

	};
}

#endif __TINY_RPC_H__