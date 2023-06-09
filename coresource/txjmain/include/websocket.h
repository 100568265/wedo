
#ifndef _WEB_SOCKET_H_
#define _WEB_SOCKET_H_

#include <boost/function/function3.hpp>

struct mg_connection;
namespace wedo {

class ws_server
{
public:
	typedef boost::function<void(mg_connection*, int, void*)> ev_handler_t;

	explicit ws_server(const char * address, const ev_handler_t& callback);
	~ws_server();

	void start ();
	void stop  ();
private:
	class _ws_server_private * _p;
};

}; // namespace wedo

#endif // _WEB_SOCKET_H_
