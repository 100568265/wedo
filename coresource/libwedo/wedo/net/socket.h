
#ifndef _WEDO_SOCKET_H_
#define _WEDO_SOCKET_H_

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
#    pragma once
#endif

#if defined(_WIN32)
	#include <winsock2.h>
#endif

namespace wedo {

class socket
{
public:
#if defined(_WIN32)
	typedef char * optval_type;
	typedef SOCKET type;
#else
	typedef void * optval_type;
	typedef int    type;
#endif

	explicit socket(type fd);
	virtual ~socket();

	int fd() const;

	static int set_not_block(const socket& s);
    static int set_cloexec  (const socket& s);

	static int enable_reuse_addr  (const socket& s, int on);
	static int enable_keep_alive  (const socket& s, int on);
	static int enable_broadcast   (const socket& s, int on);

private:
	void close ();

	socket();
	explicit socket(const socket&);

	socket& operator= (const socket&);

	const type _fd;
};

}; // namespace wedo

#endif // _WEDO_SOCKET_H_
