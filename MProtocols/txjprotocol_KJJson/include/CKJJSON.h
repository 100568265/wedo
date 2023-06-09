#ifndef CKJJSON_H
#define CKJJSON_H


#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类
#include "Channel.h"
#include  <string.h>

#include "datatype.h" //
#include "rtobjecttree.h" //

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;

class cJSON;

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
class CKJJSON : public Protocol
{
    public:
        CKJJSON();
        virtual ~CKJJSON();

        void	   Init();
        void	   Uninit();

        void	   OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN OnSend();
        bool	   OnProcess(ST_BYTE* pbuf,ST_INT len);
        bool	   IsSupportEngine(ST_INT engineType);
    protected:

    private:
        bool        CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list);

        time_t      Newcurtime,oldcurtime;
        int         InitDevInfo();
        bool        is_init;
        bool        is_firstLogin;  //用于判断是否已经发送上送登陆信息
        bool        is_auth;        //用于判断是否认证成功
        bool        is_sub;       //用于判断是否已经订阅了topic

        void        SendAllYCData();
        void        SendAllYXData();

        void        SendData();
        void        SendDagaMsg(char *cpayload);
        void        LoginAuth();
        void        subAuth();


        std::string m_devName;
        std::string m_token;
        std::string m_startTime;
        std::string m_id;
        std::string m_password;

        enum RemoveType
        {
            YXIndex = 0,
            YCIndex = 1,
            DbIndex = 2
        };
};


#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	CKJJSON* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CKJJSON_H
