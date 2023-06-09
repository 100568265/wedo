
#include "xuji103.h"

#include "iec60870_5_101_types.h"

_PROTOCOL_INTERFACE_IMPLEMENT_(XUJI103)

XUJI103::XUJI103()
{

}

XUJI103::~XUJI103()
{

}

void XUJI103::Init()
{

}

void XUJI103::Uninit()
{

}

void XUJI103::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
	readed = 0;
	do {
		if (! this->IsOpened()) {
			break;
		}

		if(! this->GetCurPort ()) {
			break;
		}

		int32_t	len = this->GetCurPort()->PickBytes(pbuf, 6, 1000);
		if(len < 6) {
			this->GetCurPort()->Clear();
			break;
		}

		int32_t star = 0;
		for(; star < len; star++)
		{
			if(pbuf[star] == 0x68)
				break;
		}
		if(star == len)
		{
			this->ShowMessage ("All Data is messy code to clear.");
			this->GetCurPort()->Clear();
			break;
		}
		if(star > 0)
		{
			this->ShowMessage ("Part Data is messy code to clear.");
			this->GetCurPort()->ReadBytes(pbuf, star);
		}

		len = this->GetCurPort()->PickBytes(pbuf, 6, 1000);
		int32_t ndatalen = ((iec104_apci*) pbuf)->length + 2;

		len = this->GetCurPort()->ReadBytes(pbuf, ndatalen);
		if(len == ndatalen) {
			readed = len;
			break;
		}
		else {
			this->ShowMessage ("Data Length is under to clear.");
			this->GetCurPort()->Clear();
		}
	} while (0);

}

bool XUJI103::OnProcess(ST_BYTE* pbuf, ST_INT len)
{

}

bool XUJI103::IsSupportEngine(ST_INT engineType)
{

}
