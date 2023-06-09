#ifndef DL645_H
#define DL645_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#include "Protocol.h"     //¹æÔ¼¸¸Àà
#include "DataCache.h"    //Êý¾Ý»º³åÀà
#include "Device.h"       //Í¨Ñ¸Éè±¸Àà
#include "Devices.h"      //Í¨Ñ¸Éè±¸¹ÜÀíÀà
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà
#include <pthread.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_dm_api.h"

class CDL645 : public Protocol
{
public:
	CDL645();
	virtual ~CDL645();
    void	Init();
    void	Uninit();

    void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
    bool	OnSend();
    bool	OnProcess(ST_BYTE* pbuf,ST_INT len);
    bool	IsSupportEngine(ST_INT engineType);
	void    ReadData  (ST_UINT32 wAddr);
    void    SendYk(int isbool);
    bool    m_ykstatue;
    int     m_ykvalue;

	pthread_t g_mqtt_process_thread;
    pthread_t g_mqtt_recv_thread;
    uint8_t g_mqtt_process_thread_running = 0;
    uint8_t g_mqtt_recv_thread_running = 0;

    int32_t     res = 0;
    void       *dm_handle = NULL;
    void       *mqtt_handle = NULL;
    int        InitMQTThandle();
    int        mqisinit = -1;
    int32_t    demo_send_property_post(void *dm_handle);
    void       update_All_platform(int id,float fvalue);
 //   void       demo_dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata);

private:
	ST_INT m_readIndex;
    unsigned char m_addrarea[6];
};


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	PROTOCOL_API Protocol* CreateInstace();
#else
	Protocol* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // DL645_H
