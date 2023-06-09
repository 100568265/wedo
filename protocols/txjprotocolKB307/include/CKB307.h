#ifndef CKB307_H
#define CKB307_H

#include "Protocol.h"
#include "DataCache.h"
#include "Device.h"
#include "Devices.h"
#include "sysinifile.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif
class CKB307 : public Protocol
{
    public:
        CKB307();
        virtual ~CKB307();
        void	Init();
        void	Uninit();
        void	OnRead(ST_BYTE* pbuf,ST_INT& readed);
        ST_BOOLEAN	OnSend();
        ST_BOOLEAN	OnProcess(ST_BYTE* pbuf,ST_INT len);
        ST_BOOLEAN	IsSupportEngine(ST_INT engineType);
    protected:
    private:
        ST_INT m_curreadIndex;
        ST_INT m_nIndex;
        void GetYC(void);
        void GetYX(void);
};
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API CKB307* CreateInstace();
#else
	CKB307* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
#endif // CKB307_H
