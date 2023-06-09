#ifndef CMQTT_H
#define CMQTT_H

#include "Protocol.h"     //规约父类
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "sysinifile.h"   //INI文件读取类
#include "datatype.h" //
#include "rtobjecttree.h" //
#include "Channel.h"
#include <vector>
#include "systhread.h"
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_subdev_api.h"
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

class NodeTree;

class CMqtt : public Protocol
{
    typedef struct{
        char devname[64];
        int  deviceId;
        aiot_subdev_dev_t subMsg;
    } subdev_modle;
    public:
        CMqtt();
        virtual ~CMqtt();

        void	                Init();
        void	                Uninit();
        void	                OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	            OnSend();
        ST_BOOLEAN	            OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	            IsSupportEngine(ST_INT engineType);

        void                    tranfer_YK_task(int devicId,bool bvalue);
        int                     get_devId(char *devName);
//        void                    set_subConn_state(int bv);

        time_t 		            Newcurtime,oldcurtime,ykNewTime,ykOldTime;
        Thread				    m_SendThread;
        int                     m_isConn;
        bool                    firstSend;
        bool                    ykSendDm;
        string                  ykDevName;
    protected:
        static ST_VOID			*WorkTaskProc(ST_VOID *param);
    private:
        void                    *mqtt_handle = NULL;
        void                    *subdev_handle = NULL;
        void                    *dm_handle = NULL;



        void                    get_var_data(char * dename,float *fd);
        void                    Init_Channels();
        void                    Init_Protocols();
        void                    AddSubModle(string addresex,string devname,int deviceId);
        void                    Init_Subdev();
        int                     Init_MQTT_Conn();
        void                    Send_dm_data();
        void                    Subdev_batch_login();


        List<Channel>           *m_Channels;
        List<ProtocolBase *>	m_pProtocols;
        vector<subdev_modle>    m_subdev;
        List<ProtocolBase *>	t_Protocols ;
        Channel                 *t_channel;

//        bool                    m_isSubConn;
};
#ifdef _WIN32
	PROTOCOL_API CMqtt* CreateInstace();
#else
	CMqtt* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CMQTT_H
