#ifndef CKWZD_H
#define CKWZD_H

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

#include "Protocol.h"
class CKWZD: public Protocol
{
public:
    CKWZD();
    virtual ~CKWZD();
    void	Init();
    void	Uninit();
    void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
    ST_BOOLEAN	OnSend();
    ST_BOOLEAN	OnProcess(ST_BYTE * pbuf, ST_INT len);
    ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

    bool	processBuf(ST_BYTE* pbuf,int len);

protected:

private:

    int     m_readindex;
    void    Read_Realtime_Data();
    void    Read_Device_Data();
    void	Read_EMeter_Data();
    void	Read_History_Data();
    void    SendYK(int bValue);
    void    Analy_realtime_Data(ST_BYTE *pbuf,bool is_new);
    void    Analy_Device_Data(ST_BYTE *pbuf);
    void    Analy_EMeter_Data(ST_BYTE *pbuf);

    void    Analy_History_Data(ST_BYTE *pbuf);

    float	calculate_value(ST_BYTE *pbuf,float coeff);
    ProtocolTask m_curTask;

    unsigned char m_addrarea[7];
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	PROTOCOL_API Protocol* CreateInstace();
#else
	CKWZD* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif

#endif // CKWZD_H
