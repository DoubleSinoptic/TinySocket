#include "tinyrpc.h"

template<typename T, typename Integral>
constexpr bool valid_index(const T& getsizearray, Integral x)
{
	return x >= 0 && getsizearray.size() > x;
}

#define iix(x) increment_index(x)
#define pix(x) pre_increment_index(x)
#define paddix(x) pre_add_index(x)
namespace ts{

	std::vector<class rpc_entity*> entitys(1, ((class rpc_entity*) - 1));


	entity_id find_empty_entity()
	{
		for (size_t i = 0; i < entitys.size(); i++)
			if (entitys[i] == nullptr)
			{
				entitys[i] = (rpc_entity*)(-1);
				return i;
			}
		entitys.push_back((rpc_entity*)(-1));
		return entitys.size() - 1;
	}

	rpc_listener* server;
	rpc_client*   client;


	rpc_entity::rpc_entity() :
		_thisId(-1)
	{}

	rpc_entity::~rpc_entity()
	{
		if (_thisId != -1)
			if(_thisId >= 0 && _thisId <= entitys.size())
				entitys[_thisId] = nullptr;
	}

	void rpc_entity::set_id(entity_id id)
	{
		if (_thisId != -1)
			entitys[_thisId] = nullptr;
		_thisId = id;
		if (entitys.size() <= _thisId)
			entitys.resize(_thisId + 1);
		if (entitys[_thisId] != nullptr && entitys[_thisId] != ((rpc_entity*)-1))
			throw std::runtime_error("entity collision detetcion");
		entitys[_thisId] = this;
	}

	entity_id rpc_entity::allocate_id_and_set()
	{
		if (_thisId != -1)
			entitys[_thisId] = nullptr;
		_thisId = find_empty_entity();
		return _thisId;
	}

	entity_id rpc_entity::allocate_id()
	{
		return find_empty_entity();
	}

    entity_id rpc_entity::get_id() const
	{
		return _thisId;
	}

	void rpc_entity::on_deleted()
	{
	}

	void rpc_entity::call(uint32_t rpcName, user_id userId, ts::binary_stream & responce)
	{
		if (server)
			server->proc._socket.cmd(iix(server->proc._users[userId]->send_next), { rpcName, 0, _thisId }, server->proc._users[userId]->external_address, responce.data(), responce.stored_length());
		else
			throw std::runtime_error("not allow");
	}

	void rpc_entity::call_unsafe(uint32_t rpcName, user_id userId, ts::binary_stream & responce)
	{
		if (server)
			server->proc._socket.unsafe_cmd({ rpcName, 0, _thisId }, server->proc._users[userId]->external_address, responce.data(), responce.stored_length());
		else
			throw std::runtime_error("not allow");
	}

	void rpc_entity::call(uint32_t rpcName, ts::binary_stream & responce)
	{
		if (server)
		{
			for (auto& i : server->proc._users)
				if (i != nullptr)
					server->proc._socket.cmd(iix(i->send_next), { rpcName, 0, _thisId }, i->external_address, responce.data(), responce.stored_length());
		}	
		else
			client->proc._socket.cmd(iix(client->proc.send_next), { rpcName, client->proc._user_id, _thisId }, client->proc._serverPoint, responce.data(), responce.stored_length());
		
	}

	void rpc_entity::call_unsafe(uint32_t rpcName, ts::binary_stream & responce)
	{
		if (server)
		{
			for (auto& i : server->proc._users)
				if (i != nullptr)
					server->proc._socket.unsafe_cmd({ rpcName, 0, _thisId }, i->external_address, responce.data(), responce.stored_length());
		}		
		else
			client->proc._socket.unsafe_cmd({ rpcName, client->proc._user_id, _thisId }, client->proc._serverPoint, responce.data(), responce.stored_length());
	}

	bool rpc_client::rpc_client_proc::process(uint32_t cmdIndex, const ts::command_info & cmd, const ts::ip_end_point & point, uint8_t * data, size_t length)
	{
		if (cmdIndex != unsafe_index)
		{
			if (cmdIndex != this->recv_next)
			{
				return false;
			}
			else
				pix(this->recv_next);
		}

		ts::binary_stream d(data, length, 0);
		if (cmd.entity_id != 0)
		{		
			if(valid_index(entitys, cmd.entity_id) && entitys[cmd.entity_id])
				entitys[cmd.entity_id]->rpc(cmd.user_index, cmd.cmd, d);
		}
		else if (cmd.cmd == ts::cmd_hash("::connect_ok"))
		{
			_user_id = cmd.user_index;
			_is_connected = true;
			_serverPoint = point;
		}
		else if (cmd.cmd == ts::cmd_hash("::error"))
		{
			std::string errorMsg = d.read_string();
			throw std::runtime_error(errorMsg);
		}
		else if (cmd.cmd == ts::cmd_hash("::disconnect_ok"))
		{
			_is_connected = false;
		}
		return true;
	}

	void insert_rpc_client(rpc_client* clicent) 
	{
		if(server)
			delete server;
		if(client)
			delete client;
		client = clicent;
	}

	void insert_rpc_server(rpc_listener* servser)
	{
		if (server)
			delete server;
		if (client)
			delete client;
		server = servser;
	}

	user_id get_current_user_id()
	{
		if (server)
			return 0;
		else
			return client->get_my_id();
	}

	void clear_entities()
	{
		for (size_t i = 1; i < entitys.size(); i++) 
		{
			if (entitys[i] != nullptr)
				entitys[i]->on_deleted();
		}
		entitys.resize(1);
	}

	SGE_EXPORT size_t users_count()
	{
		if(server)
			return server->users_count();
		return 0;
	}

	SGE_EXPORT rpc_entity ** begin_entities()
	{
		return (rpc_entity **)entitys.data();
	}

	SGE_EXPORT size_t entities_count()
	{
		return entitys.size();
	}

	rpc_client::rpc_client_proc::rpc_client_proc(int delayMs) :
		_socket(this, delayMs), _is_connected(false), _user_id(0)
	{

	}

	rpc_client::rpc_client(int delayMs) :
		proc(delayMs)
	{}

	rpc_client::~rpc_client()
	{
		disconnect();
	}

	user_id rpc_client::get_my_id() const
	{
		return proc._user_id;
	}

	void rpc_client::connect(const ts::ip_end_point & point)
	{
		proc._socket.set_server(point);
		proc._socket.cmd(iix(proc.send_next), { ts::cmd_hash("::connect") }, nullptr, 0);
		if (!proc._socket.wait_for(ts::cmd_hash("::connect_ok"), 4000))
			throw std::runtime_error("error of connect to point");
	}

	void rpc_client::disconnect()
	{
		if (proc._is_connected)
		{
			proc._socket.cmd(iix(proc.send_next), { ts::cmd_hash("::disconnect"), proc._user_id }, nullptr, 0);
			proc._is_connected = false;
		}
	}

	void rpc_client::process_commands(int maxCommands)
	{
		proc._socket.receive_and_process(maxCommands);
		long long aver = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - update_timer).count();
		if (aver >= 1)
		{
			update_timer = std::chrono::steady_clock::now();
			proc._socket.unsafe_cmd({ ts::cmd_hash("::confrim_connection"), proc._user_id }, nullptr, 0);
		}
	}

	size_t rpc_listener::users_count() const
	{
		size_t acc = 0;
		for (size_t i = 0; i < proc._users.size(); i++) 
		{
			if (proc._users[i] != nullptr)
				acc++;
		}
		return acc;
	}

	rpc_listener::rpc_listener(int delayMs, int maxUsers) :
		proc(delayMs, maxUsers, this)
	{
		update_timer = std::chrono::steady_clock::now();
	}

	void rpc_listener::bind(const ts::ip_end_point & point)
	{
		proc._socket.bind(point);
	}

	void rpc_listener::process_commands(int maxCommands)
	{
		proc._socket.receive_and_process(maxCommands);
		long long aver = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - update_timer).count();
		if (aver >= 5)
		{
			update_timer = std::chrono::steady_clock::now();
			proc.check_user_alive();
		}
	}
}

size_t ts::rpc_listener::rpc_listener_proc::users_count() const
{
	return _users.size();
}

ts::rpc_listener::rpc_listener_proc::rpc_listener_proc(int delayMs, uint32_t maxUsers, rpc_listener* meListener) :
	_socket(this, delayMs), _users(maxUsers + 1), _meListener(meListener)
{

}

uint32_t ts::rpc_listener::rpc_listener_proc::allocate_user_id()
{
	for (uint32_t i = 1; i < _users.size(); i++)
	{
		if (_users[i] == nullptr)
		{
			_users[i] = (rpc_user*)-1;
			return i;
		}
	}
	return 0;
}

void ts::rpc_listener::rpc_listener_proc::delete_user(uint32_t i)
{
	if (i == 0)
		return;
	if (_users[i] != nullptr)
	{
		_socket.cmd(iix(_users[i]->send_next), { ts::cmd_hash("::disconnect_ok") }, _users[i]->external_address, 0, 0);
		std::cout << "deleted user: " << i << ": " << _users[i]->external_address << "\n";
		if (_meListener->on_client_disconnected)
			_meListener->on_client_disconnected(_meListener, _users[i]);
		delete _users[i];
		_users[i] = nullptr;
	}
}

bool ts::rpc_listener::rpc_listener_proc::check_user_id(uint32_t id)
{
	if (id > 0 && id < _users.size() && _users[id] != nullptr)
		return true;
	return false;
}

void ts::rpc_listener::rpc_listener_proc::check_user_alive()
{
	for (auto i : _users)
	{
		if (i != nullptr)
		{
			long long aver = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - i->last_packege).count();
			if (aver >= 5)
			{
				delete_user(i->index);
			}
		}

	}
}

bool ts::rpc_listener::rpc_listener_proc::process(uint32_t cmdIndex, const ts::command_info & cmd, const ts::ip_end_point & point, uint8_t * data, size_t length)
{
	ts::binary_stream request(data, length, 0);
	ts::package_stream<64> responce;

	if (cmd.entity_id != 0 && cmd.user_index != 0)
	{
		if (check_user_id(cmd.user_index) && _users[cmd.user_index]->external_address == point)
		{
			if (cmdIndex != unsafe_index)
			{
				if (cmdIndex != _users[cmd.user_index]->recv_next)
					return false;
				else
					pix(_users[cmd.user_index]->recv_next);
			}
				

			if (valid_index(entitys, cmd.entity_id))
				entitys[cmd.entity_id]->rpc(cmd.user_index, cmd.cmd, request);
		}
		else
		{
			responce.write_string("invalid user point or id");
			_socket.unsafe_cmd({ ts::cmd_hash("::error") }, point, responce.data(), responce.stored_length());
		}
	}
	else if (cmd.user_index != 0)
	{
		if (check_user_id(cmd.user_index) && _users[cmd.user_index]->external_address == point)
		{
			if (cmdIndex != unsafe_index)
			{
				if (cmdIndex != _users[cmd.user_index]->recv_next)
					return false;
				else
					pix(_users[cmd.user_index]->recv_next);
			}

			if (cmd.cmd == ts::cmd_hash("::confrim_connection"))
			{
				_users[cmd.user_index]->last_packege = std::chrono::steady_clock::now();
			}
			else if (cmd.cmd == ts::cmd_hash("::disconnect"))
			{
				delete_user(cmd.user_index);
			}
		}
		else
		{
			responce.write_string("invalid user point or id");
			_socket.unsafe_cmd({ ts::cmd_hash("::error") }, point, responce.data(), responce.stored_length());
		}
	




	}
	else
	{
		if (cmd.cmd == ts::cmd_hash("::connect"))
		{
			bool hasFind = false;
			for (auto& i : this->_users)
				if (i != nullptr)
					if (i->external_address == point)
					{
						hasFind = true;
						break;
					}
			if (!hasFind)
			{
				uint32_t newUser = allocate_user_id();
				if (newUser == 0)
				{
					responce.write_string("users limit detected");
					_socket.unsafe_cmd({ ts::cmd_hash("::error") }, point, responce.data(), responce.stored_length());
				}
				else
				{
					_users[newUser] = new rpc_user();
					_users[newUser]->external_address = point;
					_users[newUser]->last_packege = std::chrono::steady_clock::now();
					_users[newUser]->index = newUser;
					/*И тут бы ещё проверочку на безопасность unsafe_index'a*/
					_users[newUser]->recv_next = paddix(cmdIndex);
					_socket.cmd(iix(_users[newUser]->send_next), { ts::cmd_hash("::connect_ok"), newUser }, point, nullptr, 0);

					std::cout << "connected user: " << newUser << ": " << point << "\n";
					if (_meListener->on_client_connected)
						_meListener->on_client_connected(_meListener, _users[newUser]);
				}
			}
			else
			{
				responce.write_string("you user already connected");
				_socket.unsafe_cmd({ ts::cmd_hash("::error") }, point, responce.data(), responce.stored_length());
			}

			

		}
	}


	return true;
}
