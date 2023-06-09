#ifndef JASON_H
#define JASON_H


#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"
#include "rtobjecttree.h"
#include <map>
#include <pthread.h>

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
//extern "C"
//{
#endif
using namespace std;

struct MQTTMSG{
    int     ACTION;//0:publish 1:subscribe
    char    *Topic;
    int     payloadlen;
	void*   payload;
	int     qos;
};
struct ACTIONTYPE{
    enum {
        PUBLISHMSG = 0x00,
        SUBSCRIBE = 0x01,
        UNSUBSCRIBE = 0x02
    };
};

class Jason : public Protocol
{
    public:
        Jason();
        virtual ~Jason();

        void	Init();
        void	Uninit();

        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        bool	OnSend();
        bool	OnProcess(ST_BYTE* pbuf,ST_INT len);
        bool	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        void  getVariableValue();

        void  SendDagaMsg(char *cpayload);
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API CIEC101* CreateInstace();
#else
	Jason* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // JASON_H
