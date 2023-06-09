#include "Cmodbusrtu.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"

#define sDebug	if (true) wedoDebug (SysLogger::GetInstance()).noquote

Cmodbusrtu::Cmodbusrtu()
{
                //ctor
}

Cmodbusrtu::~Cmodbusrtu()
{
                //dtor
}

Cmodbusrtu* CreateInstace()
{
    return new Cmodbusrtu();
}
//返回1为半双工
ST_BOOLEAN	CModbusRTU::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void	Cmodbusrtu::Init()
{
	m_bTask = false;
	m_curreadIndex = 0;
	m_readIndex = 0;
}

void	Cmodbusrtu::Uninit()
{

}

void       Cmodbusrtu::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
                readed = 0;
                if(! this->GetCurPort())
                                return;
                //检查有无设备模板
                if(m_curreadIndex < 0 || m_curreadIndex >= this->GetDevice()->GetDeviceInfo()->DataAreasCount)
                {
		ShowMessage ("No configuration device template.");
		m_curreadIndex = 0;
		this->GetCurPort()->Clear();
		return;
	}
	//设置超时时间等于通讯间隔，低于三秒则设置为三秒
	ST_INT  interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	interval = (interval > 3000 ? interval : 3000);
	//长度小于五则判断设备长度不足
	ST_INT len = this->GetCurPort()->PickBytes(pbuf, 5, interval);
	if(len < 5)
                {
		ShowMessage ("Insufficient data length");
		this->GetCurPort()->Clear();
		return;
	}
	//寻找报文第一位(设备地址)
	ST_INT star = 0;
	for(; star < len; ++star)
                {
		if(pbuf[star] == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
                                                break;
	}
	//star大于0，说明有乱码， 把之前的乱码丢掉
	if(star > 0)
                {
		this->GetCurPort()->ReadBytes(pbuf, star);
	}
	//star等于数据长度，则全是乱码，清除缓存
	if(star == len)
                {
		ShowMessage ("Garbled code, clear buffer.");
		this->GetCurPort()->Clear();
		return;
	}
	len = this->GetCurPort()->PickBytes(pbuf, 5, interval);
	ST_BYTE fuccode = pbuf[1 + star];//功能码，找到报文头后 第二位
	if(fuccode == 0x03)
	{
		len = 5+pbuf[2];
	}
	else if(fuccode == 0x10)
	{
		len = 8;
	}
//	else if((fuccode == 0x06) && m_bTask)
//	{
//		len = m_curTask.taskParamLen + 6;
//	}
	else if(fuccode == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].readCode)
	{
		//读数据返回
		ST_BYTE readCount = pbuf[2 + star];
		len = readCount + 5;
	}
	else if(fuccode == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->DataAreas[m_curreadIndex].writeCode)
	{
		//写数据返回
		len = 8;
	}
	else
	{
		ShowMessage ("Not Found Function Code!");
		this->GetCurPort()->Clear();
		return;
	}
	ST_INT nlen = this->GetCurPort()->PickBytes(pbuf, len, 2000);
	if(nlen == len)
	{
		this->GetCurPort()->ReadBytes(pbuf, len);
		ST_UINT16 wCRC = get_crc16(&pbuf[0], len-2);
		ST_UINT16 nCRC = pbuf[len-2] + pbuf[len-1] * 256;
		if(wCRC == nCRC)
		{
			readed = len;
			return;
		}
	//	else
	//	{
	//		ShowMessage ("Check error!");
	//		this->GetCurPort()->Clear();
	//		return;
	//	}
	}
	else
	{
		ShowMessage ("Insufficient data length.");
		this->GetCurPort()->Clear();
	}


}

void CModbusRTU::SendYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn)
{
	ST_BYTE sendbuf[32] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x06;
	FillWORD(sendbuf + 2,writeAddr);
	sendbuf[4] = (bIsOn ? 0xFF: 0x00);
	sendbuf[5] = 0x00;
	*(ST_UINT16*)(sendbuf + 6) = get_crc16(sendbuf,6);
	this->Send(sendbuf,8);

}

void CModbusRTU::SendPreYK(ST_UINT writeAddr,ST_BOOLEAN bIsOn)
{
	ST_BYTE sendbuf[256];
	if(sendbuf[4]==0x01)//遥控合闸预令
	{
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x06;
	if(bIsOn) writeAddr = 0x0080;
	//else writeAddr = 0x070C;
    sendbuf[2] = (writeAddr&0xff00)>>8;
	sendbuf[3] = writeAddr&0x00ff;
	sendbuf[4] = 0x01;
	sendbuf[5] = 0x55;
	}
	else if(sendbuf[4]==0x02)//遥控分闸预令
	{
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x06;
	if(bIsOn) writeAddr = 0x0080;
	//else writeAddr = 0x070C;
    sendbuf[2] = (writeAddr&0xff00)>>8;
	sendbuf[3] = writeAddr&0x00ff;
	sendbuf[4] = 0x02;
	sendbuf[5] = 0x55;
	}
	else if(sendbuf[4]==0x03)//遥控试跳预令
	{
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x06;
	if(bIsOn) writeAddr = 0x0080;
	//else writeAddr = 0x070C;
    sendbuf[2] = (writeAddr&0xff00)>>8;
	sendbuf[3] = writeAddr&0x00ff;
	sendbuf[4] = 0x03;
	sendbuf[5] = 0x55;
	}
	*(ST_UINT16*)(sendbuf + 6) = get_crc16(sendbuf,6);
	this->Send(sendbuf,8);
	m_curTask.taskResult.resultCode = 0;
    m_curTask.isTransfer = 1;
    Transfer(&m_curTask);
    Memset(&m_curTask, 0, sizeof(m_curTask));
}



