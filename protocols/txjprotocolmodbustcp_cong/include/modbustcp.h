
#include "Protocol.h"     //规约父类
#include "TransTable.h"

#ifdef PROTOCOL_EXPORTS
	#define PROTOCOL_API __declspec(dllexport)
#else
	#define PROTOCOL_API __declspec(dllimport)
#endif

class ModbusTcp : public Protocol
{
public:

	ModbusTcp();
	virtual ~ModbusTcp();

	void	Init();
	void	Uninit();
	void	OnRead(ST_BYTE* pbuf, ST_INT& readed);
	ST_BOOLEAN	OnSend();
	ST_BOOLEAN	OnProcess(ST_BYTE* pbuf, ST_INT len);
	ST_BOOLEAN	IsSupportEngine(ST_INT engineType);

	bool CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list);

	void ErrorResponse (const ST_BYTE * pdata, ST_BYTE ec);

	void SendDIStateData (ST_BYTE * pbuf, ST_INT len);
	void SendInputRegisterData (ST_BYTE * pbuf, ST_INT len);
	void SendKeepRegisterData (ST_BYTE * pbuf, ST_INT len);
private:
};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
	PROTOCOL_API Protocol* CreateInstace();
#else
	Protocol* CreateInstace();
#endif

#ifdef __cplusplus
}
#endif
