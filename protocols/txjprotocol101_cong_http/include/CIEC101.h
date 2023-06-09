#ifndef _CIEC101_H_
#define _CIEC101_H_

#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"
#include "rtobjecttree.h"
#include "mongoose.h"
#include "FakeTimer.h"
#include "iec60870_5_101_types.h"
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
class CIEC101 : public Protocol
{
public:
    CIEC101();
    virtual ~CIEC101();

    void	Init();
    void	Uninit();

    void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
    bool	OnSend();
    bool	OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	IsSupportEngine(ST_INT engineType);
protected:

private:
    static ST_VOID * sendHttpPost(ST_VOID *parm);

    void  SendJsonData2Post();
    void  getVariableValue();
//    struct mg_mgr mgr;
    time_t NewStime,oldStime;
    char uuidNumber[128];
    void writeData2File(char *filename,char *jsonData);
    void sendHeatBeat();
     Thread m_thread;

//void  http_handler(struct mg_connection *nc, int ev, void *ev_data);

};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API CIEC101* CreateInstace();
#else
	CIEC101* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // _CIEC101_H_
