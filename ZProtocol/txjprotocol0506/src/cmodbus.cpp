#include "cmodbus.h"
#include "syslogger.h"
#include "Debug.h"
#include "Channel.h"

#define sDebug	if (true) wedoDebug (SysLogger::GetInstance()).noquote

static const uint16_t crc16_table[256] =
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

uint16_t get_crc16 (const uint8_t *pdata, int nsize)
{
	uint16_t crc = 0xFFFF;
	while (nsize-- > 0)
		crc = crc16_table[(crc & 0xFF) ^ (*pdata++)] ^ (crc >> 8);
	return crc;
}

inline ST_BYTE FromBCD_BYTE(ST_BYTE value)
{
	return ((value & 0xF0) >> 4) * 10 + (value & 0x0F);
}

ST_UINT16 FromBCD_WORD(ST_UINT16 value)
{
	return (ST_UINT16)FromBCD_BYTE((value & 0xFF00) >> 8) * 100 + (ST_UINT16)FromBCD_BYTE(value & 0x00FF);
}

ST_UINT32 FromBCD_DWORD(ST_UINT32 value)
{
	return (ST_UINT32)FromBCD_WORD((value & 0xFFFF0000) >> 16) * 10000 + (ST_UINT32)FromBCD_WORD(value & 0x0000FFFF);
}

inline ST_BYTE TOBCD_BYTE(ST_BYTE value)
{
	return ((value / 10) << 4) + (value % 10);
}

ST_UINT16 TOBCD_WORD(ST_UINT16 value)
{
	return ((ST_UINT16)TOBCD_BYTE((value & 0xFF00) >> 8) << 8) + (ST_UINT16)TOBCD_BYTE(value & 0x00FF);
}

ST_UINT32 TOBCD_DWORD(ST_UINT32 value)
{
	return ((ST_UINT32)TOBCD_WORD((value & 0xFFFF0000) >> 16) << 16) + (ST_UINT32)TOBCD_WORD(value & 0x0000FFFF);
}

class convert
{
public:

	template<typename T>
	static T bytes_to (const void * bytes, size_t len)
	{
		T value;
		memcpy (&value, bytes, min(sizeof(T), len));
		return value;
	}

};

cmodbus::cmodbus()
{
                //ctor
}

cmodbus::~cmodbus()
{
                //dtor
}

cmodbus* CreateInstace()//实例化
{
    return new cmodbus();
}

ST_BOOLEAN	cmodbus::IsSupportEngine(ST_INT engineType)//1半双工，0全双工
{
    return 1;
}

void	cmodbus::Init()//初始化
{
	m_bTask = false;
	m_curreadIndex = 0;
	m_readIndex = 0;
}

void	cmodbus::Uninit()
{

}
void	cmodbus::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
                readed = 0;
                if (! this->GetCurPort())
                return;
                if(m_curredIndex < 0 || m_curredIndex >= this -> GetDevice() -> GetDeviceInfo() -> DateAreasCount)
                {
                                ShowMessage ("No configration device template.");
                                m_curreadIndex = 0;
                                this ->GetCurPort() -> Clear();
                                return;
                }
                ST_INT intercal = this ->GetDevice() -> GetChannel() -> GetChannelInfo() -> Channelnterval;
                interval = 200;
                ST_INT len = this -> GetCurPort() -> PickBytes(pbuf,5,interval);
                if(len<5)
                {
                                ShowMessage ("Insufficient data length");
                                this -> GetCurPort() -> Clear();
                                return;
                }
                ST_INT star = 0;
                for(;star < len;++star)
                {
                                if(pbuf[star] == (ST_BYTE)this -> GetDevice() ->GetDeviceInfo() -> Address)
                                                break;
                }
                if(star > 0)
                {
                                this -> GetCurPort() -> ReadBytes(pbuf,star);
                }
                if(star == len)
                {
                                ShowMessage("Garbled code,clear buffer.");
                                this -> GetCurPort() -> Clear();
                                return;
                }
                len = this ->GetCurPort()->PickBytes(pbuf,5,interval);
                ST_BYTE funcode = pbuf[1+star];

                if((fuccode &0xF0) == 0x80)
                {
                                len =5;
                }
                else if(fuccode == 0x05)
                {
                                len = 8;
                }
                else if((fuccode == 0x06) && m_bTask)
                {
                                len = m_curTask.taskParamLen + 6;
                }
                else if(fuccode == (ST_BYTE)this -> GetDevice() -> GetDeviceInfo() -> DataAreas[m_curreadIndex].readCode)
                {
                                ST_BYTE readCount = pbuf[2 + star];
                                len = readCount + 5;
                }
                else if(fuccode == (ST_BYTE)this -> GetDevice() -> GetDeviceInfo() -> DataAreas[m_curreadIndex].writeCode)
                {
                                len = 8;
                }
                else
                {
                                ShowMessage("Not Fund Function Code!");
                                this -> GetCurPort() -> Clear();
                                return;
                }
                ST_INT nlen = this -> GetCurPort() -> PickBytes(pbuf,len,2000);
                if(nlen == len)
                {
                                this -> GetCurPort() -> ReadBytes(pbuf,len);
                                ST_UINT16 wCRC = get_crc16(&pbuf[0],len-2);
                                ST_UINT16 nCRC = pbuf[len-2] + pbuf[len-1] * 256;
                                if(wCRC == nCRC)
                                {
                                                readed = len;
                                                return;
                                }
                }
                else
                {
                                ShowMessage("Insufficient data length.");
                                this -> GetCurPort() -> Clear();
                }
}

ST_BOOLEAN cmodbus::OnSend()
{
                if(this -> GetCurPort())
                                this -> GetCurPort ->Clear();
                m_bTask = false;
                if(this ->HasTask() && this ->GetTask(&m_curTask))
                                if(!strcmp(m_curTask.taskCmd,"singleread"))
                {
                                SendReadCmd((ST_BYTE)m_curTask . taskCmdCode , m_curTask . taskAddr , m_curTask . taskAddr1);
                                m_bTask = true;
                }
                else if(! strcmp(m_curTask,taskCmd,"devicecontrol"))
                {
                                if(m_curTask.taskCmdCode == 0)
                                                SendPreYK(m_curTesk.taskAddr,m_curTask.taskValue);
                                else if(m_curTask.taskCmdCode == 1)
                                                SendYK(m_curTask.taskAddr,m_curTask.taskValue);
                                else if(m_curTask.taskCmdCode == 2)
                                                SendReadCmd((ST_BYTE)m_curTask.taskCmdCode,m_curTask.taskAddr,m_curTask.taskAddr1);
                                else
                                {
                                                m_curTask.taskResult.resultCode = 0;
                                                m_curTask.isTransfer = 1;
                                                Transfer(&m_curTask);
                                                memset(&m_curTask,0,sizeof(m_curTask));
                                                return false;
                                }
                                m_bTask = true;
                }
                else if(!strcmp(m_curTask.taskCmd,"singlewrite"))
                {

                                if(m_curTask.taskCmdCode == 2)
                                {
                                ST_UINT16 wIndex = m_curTask.taskParam[1];
                                wIndex = (wIndex << 8) | m_curTask.taskParam[0];
                                SendYT(m_curTask.taskAddr,wIndex,m_curTask.taskValue?1:0);
                                }
                                else
                                                SendSingleWriteCmd((ST_FLOAT)m_curTask.TaskValue,m_curTask.taskAddr,m_curTask.taskAddr1);
                                m_bTask = true;
                }
                else if(!stramp(m_curTaskCmd,"multiwrite"))
                {
                                SendWriteCmd(m_curTask.taskParam , m_curTask.taskParamLen , m_curTask.taskAddr);
                                m_bTask = true;
                }
                const DeviceInfo* info = this -> GetDevice() -> GetDeviceInfo();
                if(info && info -> DataAreasCount)
                {

                                m_readIndex = 0;
                m_curreadIndex = m_readIndex++;
                SendReadCmd(info -> DataAreas[m_curredIndex].readCode,(ST_UINT16)info -> DataAreas[m_curreadIndex].addr,
                            info -> DataAreas[m_curreadIndex].dataUnitLen > 1 ?
                            info -> DataAreas[m_curreadIndex].len / info -> DataAreas[m_curreadIndex].dataUnitlen : info -> DataAreas[m_curreadIndex].len);
                }
                return true;
}

ST_BOLLEAN cmodbus::OnProcess(ST_BYTE*pbuf,ST_INT len)
{
                if(m_bTask)
                {
                                if(!strcmp(m_curTask.taskCmd,"singlewrite") || !strcmp(m_curTask.taskCmd,"multiwrite"))
                                {
                                                m_curTask.taskResult.resultCode = 0;
                                                m_curTask.isTransfer = 1;
                                                Transfer(&m_curTask);
                                                Memset(&m_curTask,0,sizeof(m_curTask));
                                                return true;
                                }
                                else if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
                                {
                                                m_curTask.taskResult.resultCode = 0;
                                                m_curTask.isTransfer = 1;
                                                Transfer(&m_curTask);
                                                Memset(m_curTask,0,sizeof(m_curTask));
                                                return true;
                                }
                                else if(!strcmp(m_curTask.taskCmd,"singleread"))
                                {
                                                m_curTask.taskResult.resultCode = 0;
                                                m_curTask.taskResult.resultDataLen = pbuf[2];
                                                mecpy(m_curTask.taskResult.resultData,&pbuf[3],m_curTask.taskResult.resultDataLen);
                                                m_curTask.isTransfer = 1;
                                                Transfer(&m_curTask);
                                                Memset(&m_curTask,0,sizeof(m_curTask));
                                                return true;
                                }
                                else
                                {
                                                m_curTask.taskResult.resultCode = 0;
                                                m_curTask .isTransfer= 1;
                                                Transfer(&m_curTask);
                                                Memset(&m_curTask,0,sizeof(m_curTask));
                                                return true;
                                }
                }
                else if(m_curreadIndex >= 0 && m_curreadIndex < this -> GetDevice() -> GetDeviceInfo() -> DataAreasCount
                        && pbuf[1] == (ST_BYTE)this ->GetDevice() -> GetDeviceInfo() -> DataAreas[m_curreadIndex].readCode)
                {
                                ST_BYTE count = pubf[2];
                                ProcessMemory(&pbuf[3],count);
                }
                return true;
}

void cmodbus::ProcessMemory(ST_BYTE* buf,ST_BYTE count)
{
                ST_INT itemsize = this -> GetDevice() -> GetDeviceInfo() -> DataAreas[m_curreadIndex].itemCout;
                for(ST_INT k = 0 ; k < itemsize ; k++)
                {
                                const ST_DataAreasItem & itemref = this -> GetDevice() -> GetDeviceInfo() -> DataAreas[m_curreadIndex].items[k];
                                if(count < (itemref.addr + itemref.dataLen))
                                                continue;
                                switch(itrmref.dataType)
                                {
                                                case VALType_Char:
                                                case VALType_SByte:
                                                                ProcessMemorySByte              (buf,itemref) ; break;
                                                case VALType_Int16:
                                                                ProcessMemoryInt16              (buf,itemref) ; break;
                                                case VALType_Int32:
                                                                ProcessMemoryInt32              (buf,itemref) ; break;
                                                case VALType_Byte:
                                                                ProcessMemoryByte               (buf,itemref) ; break;
                                                case VALType_UInt16:
                                                                ProcessMemoryUInt16             (buf,itemref) ; break;
                                                case VALType_UInt32:
                                                                ProcessMemoryUInt32             (buf,itemref) ; break;
                                                case VALType_Float:
                                                                ProcessMemorySingle             (buf,itemref) ; break;
                                                case VALType_Boolean:
                                                                ProcessMemoryBoolean            (buf,itemref) ; break;
                                                case VALType_String:
                                                                ProcessMemoryString             (buf,itemref) ; break;
                                                case VALType_Binary:
                                                                ProcessMemoryBytes              (buf,itemref) ; break;
                                                case VALType_Double:
                                                                ProcessMemoryDouble             (buf,itemref) ; break;
                                                case VALType_Decimal:
                                                                ProcessMemoryDecimal            (buf,itemref) ; break;
                                                case VALType_DateTime:
                                                                ProcessMemoryDateTime           (buf,itemref) ; break;
                                                case VALType_Int64:
                                                                ProcessMemoryInt64              (buf,itemref) ; break;
                                                case VALType_UInt64:
                                                                ProcessMemoryUInt64             (buf,itemref) ; break;
                                                default : break;
                                }
                }
}

inline ST_UINT16 bswap16 (ST_UINT16 value)//高八位和低八位互换位置12->21
{
	return (((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8));
}
inline ST_UINT32 bswap32 (ST_UINT32 value)//四字节倒序，1234->4321
{
	return (((ST_UINT32)bswap16(value & 0x0000FFFF) << 16) | bswap16((value & 0xFFFF0000) >> 16));
}
inline ST_UINT64 bswap64 (ST_UINT64 value)//八字节倒序，12345678->87654321
{
	return (((ST_UINT64)bswap32(value & 0x00000000FFFFFFFF) << 32) | bswap32((value & 0xFFFFFFFF00000000) >> 32));
}
inline ST_UINT32 wswap32 (ST_UINT32 value)//两两字节换位，1234->3412
{
	return (((value & 0x0000FFFF) << 16) | ((value & 0xFFFF0000) >> 16));
}
inline ST_UINT64 wswap64 (ST_UINT64 value)//12345678->78563412
{
	return (((ST_UINT64)wswap32(value & 0x00000000FFFFFFFF) << 32) | wswap32((value & 0xFFFFFFFF00000000) >> 32));
}

void cmodbus::ProcessMemorySByte(ST_BYTE* buf,const ST_DataAreaItem& itemref)
{
                if(itemref.dataLen == 1)
                {
                                ST_BYTE value = *(buf + itemref.addr);
                                if(itemref.endBit - itemref.beginBit < 7)
                                {
                                                value = (value & (0xFF << (itemref.beginBit - 1))) & (0xFF >> (8 - itemref.endBit));
                                                value = value >> (itemref.beginBit - 1);
                                }
                                if(itemref.coeficient < 0)
                                {
                                                value = !value;
                                }
                                this -> UpdateValue(itemref.id,(ST_BYTE)value);
                }
                else if(itemref.dataLen == 2)
                {
                                ST_INT16 value = 0x0000;
                                if(itemref.codeType == 1)
                                {
                                                memcpy (&value,buf + itemref.addr,sizeof(value));
                                                *((ST_UINT16)*&value) = bswap16(*((ST_UINT16*)&value));
                                }
                                else
                                {

                                }
                }
}








