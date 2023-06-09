#include "pppp.h"
Protocol * CreateInstace()
{
	return new pppp();
}
pppp::pppp()
{
    //ctor
}

pppp::~pppp()
{
    //dtor
}

void pppp::Init()
{
    m_readIndex = 0;
	memset (m_addrarea, 0, sizeof(m_addrarea));
}

void pppp::Uninit()
{
}

bool pppp::IsSupportEngine(ST_INT engineType)
{
	return true;
}

void pppp::OnRead(ST_BYTE * pbuf, ST_INT& readed)
{

}

bool pppp::OnSend()
{

	return true;
}

bool pppp::OnProcess (ST_BYTE* pbuf, ST_INT len)
{

	return true;
}
