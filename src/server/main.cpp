#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include "../generated/bicyclade.pb.h"
#include "websocket.hpp"
#include <iostream>
#include <set>

/*#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>*/
#include <websocketpp/common/thread.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;
using bicyclade::Action;
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
/* on_open insert connection_hdl into channel
 * on_close remove connection_hdl from channel
 * on_message queue send to all channels
 */

class Server {
private:
    condition_variable m_action_cond;
	std::queue<action> m_actions;
    con_list m_connections;
    mutex m_action_lock;
    broadcast_server serv;
public:


void start() {
	try {
		serv.init(&m_action_cond, &m_actions, &m_connections, &m_action_lock);
		thread s(bind(&broadcast_server::run,&serv,9002));
		process_messages();
		s.join();
	} catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}

}
void process_messages() {
	while(1) {
		unique_lock<mutex> lock(m_action_lock);

		while(m_actions.empty()) {
			m_action_cond.wait(lock);
		}

		action a = m_actions.front();
		m_actions.pop();

		lock.unlock();

		if (a.type == SUBSCRIBE) {
			m_connections.insert(a.hdl);
		} else if (a.type == UNSUBSCRIBE) {
			m_connections.erase(a.hdl);
		} else if (a.type == MESSAGE) {

			Action act;
			act.ParseFromString(a.msg->get_payload());
			serv.broadcast(act);

		} else {
			// undefined.
		}
	}
}
};

int main() {
	Server server;
	server.start();
}
