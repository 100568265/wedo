#ifndef __PORTTCPCLIENT_H__
#define __PORTTCPCLIENT_H__

#include "PortBase.h"
#include "PortTcp.h"

#include "boost/shared_ptr.hpp"

namespace wedo {
    class socket;
};

class PortTcpClient : public PortBase, public PortTcp
{
public:
    typedef boost::shared_ptr<wedo::socket> socket_ptr;

    PortTcpClient (Channel *channel, PortInfo *portInfo);
    ~PortTcpClient();
    ST_VOID					Init  ();
    ST_VOID					Uninit();
    ST_VOID					Open  ();
    ST_VOID					Close ();
    ST_BOOLEAN				Send  (ST_UINT64 portAddr, ST_BYTE * pBuf, ST_UINT size);

    ST_VOID					Recv  ();

    ST_VOID                 Restart4Gmodule();
    ST_BOOLEAN              RunShellScript(const ST_CHAR *fileName);
    ST_INT                  ping_status(char *ip);
    static ST_VOID *        RUNDialSript(ST_VOID *param);
    static ST_VOID *        pingTaskProc(ST_VOID *param);
    ST_VOID                 checkPing();
    ST_CHAR		m_RemoteAddress[65];
    int                     lostNum;
    ST_BOOLEAN              isPWork;
    ST_BOOLEAN              firstConnect;         //用于拔下天线，管理机失去网络，装回天线恢复网络，但程序无法恢复socket连接（怀疑此操作使得ip变更）
                                                  //所以每次ping通ip后，关闭socket吗，实现重连

    char                    gprsOnFileName[64];  //脚本名字 位于program目录下gprs-off gprs-on dial-on
    char                    dialOnFileName[64];

    ST_VOID                 readProgramList();

private:
    enum State_t {
        NoConnect,
        Connecting,
        Connected,
        Disconnecting
    };
    inline State_t  State () const { return (State_t)_state; }
    inline void     SetState (State_t s) { _state = s; }

    struct	sockaddr_in		m_Server_addr;

    int         _state;
    socket_ptr  _socket;

    struct sockaddr_storage _dest_addr;

    Thread	                m_4gThread;
    Thread                  pingThread;
    clock_t	                oldcurtime, newcurtime;
};

#endif // __PORTTCPCLIENT_H__
