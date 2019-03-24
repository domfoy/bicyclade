#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>
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
/* on_open insert connection_hdl into channel
 * on_close remove connection_hdl from channel
 * on_message queue send to all channels
 */
enum action_type {
    SUBSCRIBE,
    UNSUBSCRIBE,
    MESSAGE
};

struct action {
    action(action_type t, connection_hdl h) : type(t), hdl(h) {}
    action(action_type t, connection_hdl h, server::message_ptr m)
      : type(t), hdl(h), msg(m) {}

    action_type type;
    websocketpp::connection_hdl hdl;
    server::message_ptr msg;
};

typedef std::set<connection_hdl,std::owner_less<connection_hdl> > con_list;

class broadcast_server {
private:

    condition_variable* m_action_cond;
	std::queue<action>* m_actions;
    con_list* m_connections;
    mutex* m_action_lock;
public:

    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl, server::message_ptr msg);
    void broadcast(Action& action);
    server m_server;
    broadcast_server();
    void init(condition_variable* cond, std::queue<action>* actions,
    		con_list* connec, mutex* mut);
    void run(uint16_t port);

};
