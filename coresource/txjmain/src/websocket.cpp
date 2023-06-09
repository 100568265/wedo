
#include "websocket.h"

#include "mongoose.h"
#include "wedo/thread/thread.h"
#include "wedo/thread/this_thread.h"

#include <boost/bind.hpp>

namespace wedo {

struct _ws_server_private
{
    typedef ws_server::ev_handler_t ev_handler_t;

	std::string _addr;
	ev_handler_t _handler;
	thread _thr;
	bool _running;
    struct mg_connection * _nc;
    struct mg_mgr _mgr;

	_ws_server_private(const char * address, const ev_handler_t& callback):
		_addr(address),
		_handler(callback),
		_thr(boost::bind(&_ws_server_private::poll, this)),
		_running(false),
		_nc(0)
		{
			mg_mgr_init(&_mgr, this);
		}

	~_ws_server_private()
	{
		_running = false;
		_thr.join();
		mg_mgr_free(&_mgr);
	}

	void poll ()
	{
		while (_running) {
			if (!_nc) {
			    _nc = mg_bind(&_mgr, _addr.c_str(), _ws_server_private::ev_handler);
			    if (_nc)
			    	mg_set_protocol_http_websocket(_nc);
			    else
			    	wedo::this_thread::msleep(1000);
			}
			else
				mg_mgr_poll(&_mgr, 1000);
		}
	}

	static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
	{
		try {
            _ws_server_private * data = static_cast<_ws_server_private*>(nc->mgr->user_data);
			if (data) data->_handler(nc, ev, ev_data);
		}
	    catch (std::exception& ex)
	    {
	    	fprintf(stderr, "exception caught in websocket server. reason: %s\n", ex.what());
	    }
	    catch (...) {
			fprintf(stderr, "unknown exception caught in websocket server.");
	    	throw;
	    }
	}
};

ws_server::ws_server(const char * address, const ev_handler_t& callback):
_p(new _ws_server_private(address, callback))
{
}

ws_server::~ws_server()
{
	delete _p;
}

void ws_server::start()
{
	_p->_running = true;
	_p->_thr.start();
}

void ws_server::stop ()
{
	_p->_running = false;
	_p->_thr.join();
}

}; // namespace wedo
