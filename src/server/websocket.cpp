#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include "websocket.hpp"
#include "../generated/bicyclade.pb.h"
#include <iostream>
#include <set>

/*#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>*/
#include <websocketpp/common/thread.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;
using bicyclade::Action;
using namespace std;

broadcast_server::broadcast_server() {
	// Initialize Asio Transport
	m_server.init_asio();

	// Register handler callbacks
	m_server.set_open_handler(bind(&broadcast_server::on_open,this,::_1));
	m_server.set_close_handler(bind(&broadcast_server::on_close,this,::_1));
	m_server.set_message_handler(bind(&broadcast_server::on_message,this,::_1,::_2));
}

void broadcast_server::init(condition_variable* cond, std::queue<action>* actions,
		con_list* connec, mutex* mut) {
	m_action_cond = cond;
	m_actions = actions;
	m_connections = connec;
	m_action_lock = mut;
}


void broadcast_server::run(uint16_t port) {
	// listen on specified port
	m_server.listen(port);

	// Start the server accept loop
	m_server.start_accept();

	// Start the ASIO io_service run loop
	try {
		m_server.run();
	} catch (const std::exception & e) {
		std::cout << e.what() << std::endl;
	}
}

void broadcast_server::on_open(connection_hdl hdl) {
	{
		lock_guard<mutex> guard(*m_action_lock);
		//std::cout << "on_open" << std::endl;
		m_actions->push(action(SUBSCRIBE,hdl));
	}
	m_action_cond->notify_one();
}

void broadcast_server::on_close(connection_hdl hdl) {
	{
		lock_guard<mutex> guard(*m_action_lock);
		//std::cout << "on_close" << std::endl;
		m_actions->push(action(UNSUBSCRIBE,hdl));
	}
	m_action_cond->notify_one();
}

void broadcast_server::on_message(connection_hdl hdl, server::message_ptr msg) {
	// queue message up for sending by processing thread
	{
		lock_guard<mutex> guard(*m_action_lock);
		//std::cout << "on_message" << std::endl;
		m_actions->push(action(MESSAGE,hdl,msg));
	}
	m_action_cond->notify_one();
}

void broadcast_server::broadcast(Action& action) {
	con_list::iterator it;
	string sria;
	action.SerializeToString(&sria);
	for (it = m_connections->begin(); it != m_connections->end(); ++it) {
		m_server.send(*it,sria,websocketpp::frame::opcode::text);
	}
}
