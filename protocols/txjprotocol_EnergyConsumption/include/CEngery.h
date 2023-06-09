#ifndef CENGERY_H
#define CENGERY_H

#include "Protocol.h"     //¹æÔ¼¸¸Àà
//#include "DataCache.h"    //Êý¾Ý»º³åÀà
#include "Device.h"       //Í¨Ñ¸Éè±¸Àà
#include "Devices.h"      //Í¨Ñ¸Éè±¸¹ÜÀíÀà
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà
#include "datatype.h" //
#include "rtobjecttree.h" //
#include <list>
#include "tinyxml.h"
#include "systhread.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

using namespace std;
class NodeTree;
class BUFDataCache;
class CEngery : public Protocol
{
    public:
        CEngery();
        virtual ~CEngery();

        void	        Init();
        void	        Uninit();
        void	        OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	    OnSend();
        ST_BOOLEAN	    OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	    IsSupportEngine(ST_INT engineType);

        bool            isOpen;
        BUFDataCache    *bdCache;
        void            check_data_send();

    protected:
        static ST_VOID				*WorkTaskProc(ST_VOID *param);
    private:
        char            building_id[64];
        char            gateway_id[64];

        TiXmlElement *  creat_RootElemt(const char *typeText);

        void            send_id_validate();
        void            send_heart_beat();
        void            send_data();
        void            resend_config();
        void            anaylizeXmlData(char *xmlData);
        int             get_data_id(string type);
        TiXmlElement *  creat_elemt(string name,string txt);
        void            send_encrypt_Data(string xmlstr);

        void            sendTest();
        bool            m_regesit;
        time_t 		    heat_Newtime,heat_Oldtime,sdata_Newtime,sdata_Oldtime;
        int             sequence = 1;


        void            send_buf(ST_BYTE *pBuf,ST_INT len);
        Thread			m_WorkTread;
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API CModbusRTU* CreateInstace();
#else
	CEngery* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // CENGERY_H
