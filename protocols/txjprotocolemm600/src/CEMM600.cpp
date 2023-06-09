#include "CEMM600.h"

static const ST_UINT m_crc16_table[256]=
{
	0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
	0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
	0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
	0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
	0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
	0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
	0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
	0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
	0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
	0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
	0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
	0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
	0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
	0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
	0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
	0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
	0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
	0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
	0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
	0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
	0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
	0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
	0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
	0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
	0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
	0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
	0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
	0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
	0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
	0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
	0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
	0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

ST_UINT GetCRC16(const ST_BYTE *pbData, ST_INT nSize)
{
	ST_UINT crc =0xffff;
	for (ST_INT i =0; i < nSize; i++)
	{
		crc =m_crc16_table[(crc & 0xff) ^ (*pbData++)] ^ (crc >> 8);
	}
	return crc;
}


CEMM600::CEMM600()
{
    //ctor
}

CEMM600::~CEMM600()
{
    //dtor
}

CEMM600* CreateInstace()
{
    return new CEMM600();
}

ST_BOOLEAN	CEMM600::IsSupportEngine(ST_INT engineType)
{
    return 1;
}

void	CEMM600::Init()
{
    m_readIndex = 0;
}
void	CEMM600::Uninit()
{

}
void	CEMM600::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
	if(!this->GetCurPort())
		return ;
	
	ST_INT	len = this->GetCurPort()->PickBytes(pbuf, 5, 2000);
	if(len < 5) {
		ShowMessage ("Insufficient data length.");
		this->GetCurPort()->Clear();
		return ;
	}

	ST_INT star = 0;
	for(; star < len; ++star) {
		if(pbuf[star] == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
			break;
	}
	if(star > 0) {
		//star大于0，说明有乱码， 把之前的乱码丢掉
		this->GetCurPort()->ReadBytes(pbuf, star);
	}
	if(star == len) {
		//全是乱码
//		this->ShowReceiveFrame(pbuf,len);
		this->GetCurPort()->Clear();
		return;
	}
	ST_BYTE fuccode = pbuf[1 + star];
	if((fuccode & 0xf0) == 0x80) {
		len = 5;
	}
	else if(fuccode == 0x05) {
		len = 8;
	}
	else if(fuccode == 0x02) {
		len = 7;
	}
	else if(fuccode == 0x03) {
		//读数据返回
		ST_BYTE readCount = pbuf[2 + star];
		len = readCount + 5;
	}
	else {
		ShowMessage ("Not found func code!");
		this->GetCurPort()->Clear();
	}
	ST_INT nlen = this->GetCurPort()->PickBytes(pbuf, len, 1000);
	if(nlen == len)
	{
		this->GetCurPort()->ReadBytes(pbuf, len);
		ST_UINT16 wCRC = GetCRC16(&pbuf[0], len-2);
		ST_UINT16 nCRC = pbuf[len-2] + pbuf[len-1] * 256;
		if (wCRC == nCRC)
			readed = len;
		else {
			readed = 0;
			ShowMessage ("Check error!");
		}
		return;
	}
	else {
		ShowMessage ("Insufficient data length.");
		this->GetCurPort()->Clear();
	}
}

ST_BOOLEAN	CEMM600::OnSend()
{
	m_bTask = false;
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			switch (m_curTask.taskCmdCode) {
				case 1: {
					SendYK(m_curTask.taskValue);
				} break;
				default: {
					m_curTask.taskResult.resultCode = 0;
					m_curTask.isTransfer = 1;
					Transfer(&m_curTask);
					memset(&m_curTask,0,sizeof(m_curTask));
				} break;
			}
		}
	}

	switch(m_readIndex++)
	{
		case 0:
			SendReadCmd(0x03,0x01,0x2c);
			break;
		case 1:
			SendReadCmd(0x03,0x73,0x06);
			break;
		case 2:
			SendReadCmd(0x02,0x56,0x01);
			break;
		default:
		break;
	}
	if(m_readIndex>=3) m_readIndex =0;
	return true;
}

ST_BOOLEAN	CEMM600::OnProcess(ST_BYTE* pbuf,ST_INT len)
{
	if (m_bTask) {
		if(!strcmp(m_curTask.taskCmd, "devicecontrol"))
		{
			m_curTask.taskResult.resultCode = 0;
		    m_curTask.isTransfer = 1;
		    Transfer(&m_curTask);
			memset(&m_curTask, 0, sizeof(m_curTask));
			return true;
		}
	}

	if (this->GetDevice()->GetDeviceInfo()->DataAreasCount <= 0) {
		ShowMessage ("Device Template is Empty!");
		return true;
	}

	if((len-5) == (0x2c*2))
	{
		ST_INT nIndexVarYC=0;
		ST_INT i=0;
		for(i=0;i<0x1c;i++)
		{
            if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0)
                this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient = 1;
			if(pbuf[3+i*2]&0x80)
			{
				ST_FLOAT fvalue =-1* (0xffff - (pbuf[3+i*2]*256 + pbuf[3+i*2+1]) +1 )*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
				this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
			}
			else
			{
				ST_FLOAT fvalue =(pbuf[3+i*2]*256 + pbuf[3+i*2+1])*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
				this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
			}
		}
		for(i=0;i<8;i++)
		{
            if(this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient == 0)
                this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient = 1;
			ST_FLOAT dvalue = ((pbuf[59+i*4]*256 + pbuf[59+i*4+1])+(pbuf[59+i*4+2]*256 + pbuf[59+i*4+3])*10000)*this->GetDevice()->GetDeviceInfo()->DataAreas[0].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)dvalue);
		}
	}
	else if((len-5) == (0x06*2))
	{
		int nIndexVarYC=100;

		for(int i=0;i<0x06;i++)
		{
			ST_DeviceDataArea* dda = this->GetDevice()->GetDeviceInfo()->DataAreas;
			if (dda[1].itemCount < nIndexVarYC)
				continue;

            if(dda[1].items[nIndexVarYC].coeficient == 0)
                dda[1].items[nIndexVarYC].coeficient = 1;
			ST_FLOAT fvalue = (pbuf[3+i*2]*256 + pbuf[3+i*2+1]) * dda[1].items[nIndexVarYC].coeficient;
			this->UpdateValue(nIndexVarYC++,(ST_FLOAT)fvalue);
		}
	}
    else if((len-5) == (0x01*2))
	{
		ST_INT j =0;
		ST_INT nIndexVarYX=200;
        ST_BYTE by =0x01;
        ST_BYTE byValue = 0x00;
        for(j=0;j<8;j++)
        {
            byValue = ((by<<j)&pbuf[4])?0x01:0x00;
            this->UpdateValue(nIndexVarYX++,(ST_BYTE)byValue);
        }
         by =0x01;
        for(j=0;j<8;j++)
        {
            byValue = ((by<<j)&pbuf[3])?0x01:0x00;
            this->UpdateValue(nIndexVarYX++,(ST_BYTE)byValue);
        }
	}
	return true;
}

void CEMM600::SendReadCmd(ST_BYTE code,ST_UINT16 readAddr,ST_UINT16 count)
{
	ST_BYTE sendbuf[8] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = code;
	sendbuf[2] = (ST_BYTE)(readAddr>>8);
	sendbuf[3] = (ST_BYTE)readAddr;
	sendbuf[4] = (ST_BYTE)(count>>8);
	sendbuf[5] = (ST_BYTE)count;
	*(ST_UINT16*)(sendbuf + 6) = GetCRC16(sendbuf,6);
	this->Send(sendbuf,8);
}

void  CEMM600::SendYK(ST_BOOLEAN bIsOn)
{
	ST_BYTE sendbuf[8] = {0};
	sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	sendbuf[1] = 0x05;
	sendbuf[2] = 0x10;
	sendbuf[3] = (ST_BYTE)(!bIsOn);
	sendbuf[4] = 0xFF;
	sendbuf[5] = 0x00;
	*(ST_UINT16*)(sendbuf + 6) = GetCRC16(sendbuf, 6);
	this->Send(sendbuf,8);
}
