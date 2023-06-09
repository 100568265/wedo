
#include "../../wedo/net/socket.h"

#if defined(_WIN32)

#else
    #include <unistd.h>
	#include <sys/socket.h>
    #include <fcntl.h>
#endif

namespace wedo {

socket::socket(type fd)
:_fd(fd)
{}

socket::~socket()
{
	this->close();
}

void socket::close()
{
#if defined(_WIN32)
	::closesocket(_fd);
#else
	::close (_fd);
#endif
}

int socket::fd() const
{
    return _fd;
}

int socket::set_not_block(const socket& s)
{
#ifdef _WIN32
	unsigned long is_enalbe = 1;
	return (ioctlsocket(s._fd, FIONBIO, &is_enalbe) ? -1: 0);
#else
    int flag = fcntl(s._fd, F_GETFL, 0);
    if (flag == -1) return -1;
    return fcntl(s._fd, F_SETFL, flag | O_NONBLOCK);
#endif
}

int socket::set_cloexec(const socket& s)
{
#ifdef _WIN32
#else
    int flag = fcntl(s._fd, F_GETFL, 0);
    if (flag == -1) return -1;
    return fcntl(s._fd, F_SETFL, flag | FD_CLOEXEC);
#endif
}

int socket::enable_reuse_addr(const socket& s, int on)
{
    int optval = (on ? 1 : 0);
    return setsockopt(s._fd, SOL_SOCKET, SO_REUSEADDR, (optval_type)&optval, sizeof(optval));
}

int socket::enable_keep_alive(const socket &s, int on)
{
  int optval = on ? 1 : 0;
  return setsockopt(s._fd, SOL_SOCKET, SO_KEEPALIVE, (optval_type)&optval, sizeof(optval));
}

int socket::enable_broadcast(const socket &s, int on)
{
  int optval = on ? 1 : 0;
  return setsockopt(s._fd, SOL_SOCKET, SO_BROADCAST, (optval_type)&optval, sizeof(optval));
}

}; // namespace wedo
