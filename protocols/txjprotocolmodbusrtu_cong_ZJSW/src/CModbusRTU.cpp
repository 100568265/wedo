#include "CModbusRTU.h"
#include "Device.h"
#include "Channel.h"
#include "EngineBase.h"
#include <sys/time.h>
#include "tinyxml.h"
#include <stdio.h>

static const uint16_t crc16_table[256] = {
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

inline void set_crc16_low (uint8_t *pdata, int nsize)
{
	if (nsize < 0) return;
	uint16_t crc_value = get_crc16 (pdata, nsize);
	memcpy (pdata + nsize, &crc_value, sizeof(crc_value));
}

enum FunctionCode  {
	FC_READ_DO_STATE       = 0x01,
	FC_READ_DI_STATE       = 0x02,
	FC_READ_INPUT_REGISTER = 0x03,
	FC_READ_KEEP_REGISTER  = 0x04,
	FC_WRITE_COIL          = 0x05,
	FC_WRITE_REGISTER      = 0x06,
	FC_WRITE_COIL_MUIL     = 0x0F,
	FC_WRITE_REGISTER_MUIL = 0x10
};

enum ExceptionCode {
	EC_NORMAL              = 0x00,
	EC_ILLEGAL_FUNC        = 0x01,
	EC_ILLEGAL_DATA_ADDR   = 0x02,
	EC_ILLEGAL_VALUE       = 0x03,
	EC_SLAVE_FAILURE       = 0x04,
	EC_CONFIRM             = 0x05,
	EC_SLAVE_BUSYNESS      = 0x06,
	EC_PARITY_ERROR        = 0x08
};

int check_func_code (ST_BYTE * pdata, int& len)
{
	len = 0;
	switch (pdata[1]) {
		case FC_READ_DO_STATE:
		case FC_READ_DI_STATE:
		case FC_READ_INPUT_REGISTER:
		case FC_READ_KEEP_REGISTER:
		case FC_WRITE_REGISTER:
		case FC_WRITE_COIL: {
			len = 8;
			return EC_NORMAL;
		}
		case FC_WRITE_COIL_MUIL: {
			int num   = pdata[4] * 256 + pdata[5];
			int count = num / 8 + (num % 8 ? 1 : 0);
			if (num < 1 || num > 0x07B0 || count != pdata[6])
				return EC_ILLEGAL_VALUE;

			len = count + 9;
			return EC_NORMAL;
		}
		case FC_WRITE_REGISTER_MUIL: {
			/*int num   = pdata[4] * 256 + pdata[5];
			int count = 2 * num;
			if (num < 1 || num > 0x7B || count != pdata[6])
				return EC_ILLEGAL_VALUE;
*/
			len = pdata[6] + 9;
			return EC_NORMAL;
		}
	}
	return EC_ILLEGAL_FUNC;
}

CModbusRTU * CreateInstace()
{
	return new CModbusRTU();
}

CModbusRTU::CModbusRTU()
{
    datacenter = new DataCenter();
    Memset(m_historyData,0,sizeof(m_historyData));
}

CModbusRTU::~CModbusRTU()
{
    delete datacenter;
}

void	CModbusRTU::Init()
{
	m_bTask = false;
}

void	CModbusRTU::Uninit()
{

}

void	CModbusRTU::OnRead(ST_BYTE* pbuf,ST_INT& readed)
{
		readed = 0;
	do {
		if(! this->GetCurPort ())
			break;

		int32_t	len = this->GetCurPort()->PickBytes(pbuf, 8, 1000);
		if(len < 8) {
			this->GetCurPort()->Clear();
			break;
		}
		int32_t star = 0;
		for (; star < len; ++star) {
			if (pbuf[star] == (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
				break;
		}
		if(star == len) {
			ShowMessage ("All are garbled. Clear buffer.");
			this->GetCurPort()->Clear();
			break;
		}
		if(star > 0) {
			ShowMessage ("Has garbled. Discard garbled.");
			this->GetCurPort()->ReadBytes(pbuf, star);
		}
		len = this->GetCurPort()->PickBytes(pbuf, 8, 2000);
		if (*pbuf != (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
			break;

		int need_len = 0;
		int ec = check_func_code (pbuf, need_len);
		if (ec) {
			ErrorResponse (pbuf[1], ec);
			this->GetCurPort()->Clear();
			break;
		}
		len = this->GetCurPort()->PickBytes(pbuf, need_len, 2000);
		if (len != need_len) {
			ShowMessage ("Insufficient data length.");
			this->GetCurPort()->Clear();
			break;
		}
		len = this->GetCurPort()->ReadBytes(pbuf, need_len, 1000);
		if (pbuf[need_len - 2] + pbuf[need_len - 1] * 256 != get_crc16(pbuf, need_len - 2))
		{
			ShowMessage ("Check error!");
			this->GetCurPort()->Clear();
			break;
		}
		readed = len;
	} while (0);
}

ST_BOOLEAN	CModbusRTU::OnSend()
{
	// m_bTask = false;
	if(this->HasTask() && this->GetTask(&m_curTask))
	{
		if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
		{
			if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
			{
				return true;
			}
		}
	}
	if (!curEngineType)
	    sleep (1);     // 全双工通道时睡眠以空出CPU资源

    datacenter->inserAllDataInXml();

	return true;
}

ST_BOOLEAN	CModbusRTU::IsSupportEngine(ST_INT engineType)
{
	curEngineType = engineType;
    return true;
}

/*
enum FunctionCode  {
	FC_READ_DO_STATE       = 0x01,
	FC_READ_DI_STATE       = 0x02,
	FC_READ_INPUT_REGISTER = 0x03,
	FC_READ_KEEP_REGISTER  = 0x04,
	FC_WRITE_COIL          = 0x05,
	FC_WRITE_REGISTER      = 0x06,
	FC_WRITE_COIL_MUIL     = 0x0F,
	FC_WRITE_REGISTER_MUIL = 0x10
};
*/
ST_BOOLEAN	CModbusRTU::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
	switch(pbuf[1]) {
		case FC_READ_DO_STATE: {
			ErrorResponse (pbuf[1], EC_ILLEGAL_FUNC);
		} break;
		case FC_READ_DI_STATE: {
			SendDIStateData       (pbuf, len);
		} break;
		case FC_READ_INPUT_REGISTER: {
		    if(pbuf[2]==0x01 && pbuf[3]==0x00)
            {
                SendHistoryRegisterData(pbuf,len);
            }
            else
                SendInputRegisterData (pbuf, len);
		} break;
		case FC_READ_KEEP_REGISTER: {
			SendKeepRegisterData  (pbuf, len);
		} break;
		case FC_WRITE_COIL: {
			//ErrorResponse (pbuf[1], EC_ILLEGAL_FUNC);
			TaskHandlerToLower(pbuf,len);
		} break;
		case FC_WRITE_REGISTER: { //0x06
			//ErrorResponse (pbuf[1], EC_ILLEGAL_FUNC);
            GetHistoryData(pbuf);
		} break;
		case FC_WRITE_COIL_MUIL: {
			ErrorResponse (pbuf[1], EC_ILLEGAL_FUNC);
		} break;
		case FC_WRITE_REGISTER_MUIL: {
			//ErrorResponse (pbuf[1], EC_ILLEGAL_FUNC);
			checkTime(pbuf);
		} break;
	}
	return true;
}

bool CModbusRTU::CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list)
{
	if (!this->GetDevice())
		return false;
	List<TransferTable> *trantables = this->GetDevice()->m_transTable.m_pTables;
	if ( !trantables) {
		return false;
	}
	if (trantables->GetCount() <= 0) {
		return false;
	}
	int iter = 0;
	for (; iter < trantables->GetCount(); ++iter)
	{
		if (index == trantables->GetItem(iter)->typeId())
			break;
	}
	if ((table = trantables->GetItem(iter)) == NULL) {
		return false;
	}
	if ((list = table->m_pVarAddrs) == NULL) {
		return false;
	}
	return true;
}

template<typename T>
inline T variant_trans_handler (const ST_VARIANT& var)
{
	switch(var.vt) {
		case VALType_SByte  : return (T)var.cVal ;
		case VALType_Byte   : return (T)var.bVal ;
		case VALType_Int32  : return (T)var.iVal ;
		case VALType_UInt32 : return (T)var.uiVal;
		case VALType_Int64  : return (T)var.lVal ;
		case VALType_UInt64 : return (T)var.ulVal;
		case VALType_Float  : return (T)var.fVal ;
		case VALType_Double : return (T)var.dtVal;
	}
	return 0;
}

void CModbusRTU::SendDIStateData (ST_BYTE * pbuf, ST_INT len)
{

	int state_num = pbuf[5] + pbuf[4] * 256;
	if (state_num < 1 || state_num > 0x07D0) {
		ErrorResponse (pbuf[1], EC_ILLEGAL_VALUE);
		return ;
	}

	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (FC_READ_DI_STATE, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		list_size = 0;
	}

	ST_UINT16 data_addr = pbuf[3] + pbuf[2] * 256;
	if (list_size < data_addr + state_num) {
		ErrorResponse (pbuf[1], EC_ILLEGAL_DATA_ADDR);
		return ;
	}

	ST_BYTE data[256] = {0};
	data[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[1] = FC_READ_DI_STATE;
	data[2] = state_num / 8 + (state_num % 8 ? 1 : 0);

	ST_BYTE * dataptr = data + 2;

	ST_VARIANT stvar;
	for (ST_UINT16 index = 0; index < state_num; ++index)
	{
		if ((index % 8) == 0)
			++dataptr;

		ST_DUADDR  *duaddr = tablelist->GetItem(index + data_addr);
		TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
		memset (&stvar, 0, sizeof (stvar));
		ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		bool tempvar = variant_trans_handler<bool> (stvar); //stvar.bVal;
		if (TRVar && TRVar->Coefficient == -1.0) {
			tempvar = !tempvar;
		}
		*dataptr |= ((ST_BYTE)tempvar << (index % 8));
	}
	set_crc16_low (data, data[2] + 3);
	this->Send    (data, data[2] + 5);
}

void  CModbusRTU::SendHistoryRegisterData (ST_BYTE * pbuf, ST_INT len)
{
    printf("m_hisdlist[2][3][4][5]:0x%x 0x%x 0x%x 0x%x\n",m_historyData[2],m_historyData[3],m_historyData[4],m_historyData[5]);

    ST_UINT16 regi_num = pbuf[5] + pbuf[4] * 256;
    ST_UINT16 data_addr = (pbuf[3] + pbuf[2] * 256)-0x0100;

    ST_BYTE sendbuf[256] = {0};
    sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[1] = FC_READ_INPUT_REGISTER;
    sendbuf[2] = regi_num * 2;
    Memcpy(&sendbuf[3],&m_historyData[data_addr*2],regi_num*2);

    set_crc16_low(sendbuf, sendbuf[2]+3);
    this->Send(sendbuf, sendbuf[2]+5);
}

//0x03
void CModbusRTU::SendInputRegisterData (ST_BYTE * pbuf, ST_INT len)
{
    ST_BYTE databuf[256];
    Memset(databuf,0,sizeof(databuf));
    assemblydata(databuf);
    ST_UINT16 regi_num = pbuf[5] + pbuf[4] * 256;
    ST_UINT16 data_addr = pbuf[3] + pbuf[2] * 256;

    ST_BYTE sendbuf[256] = {0};
    sendbuf[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    sendbuf[1] = FC_READ_INPUT_REGISTER;
    sendbuf[2] = regi_num * 2;
/*    if(sizeof(databuf)<data_addr*2||sizeof(databuf)<regi_num*2)
    {

    }*/
    Memcpy(&sendbuf[3],&databuf[data_addr*2],regi_num*2);

    set_crc16_low(sendbuf, sendbuf[2]+3);
    this->Send(sendbuf, sendbuf[2]+5);
/*
	ST_UINT16 regi_num = pbuf[5] + pbuf[4] * 256;
	if (regi_num < 1 || regi_num > 0x7D || regi_num / 2 < 1)
	{
		ErrorResponse (pbuf[1], EC_ILLEGAL_VALUE);
		return ;
	}

	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (FC_READ_INPUT_REGISTER, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		list_size = 0;
	}

	ST_UINT16 data_addr = pbuf[3] + pbuf[2] * 256;
	if (list_size < data_addr / 2 + regi_num / 2) {
		ErrorResponse (pbuf[1], EC_ILLEGAL_DATA_ADDR);
		return ;
	}

    ST_BYTE data[256] = {0};
    data[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
    data[1] = FC_READ_INPUT_REGISTER;
    data[2] = (regi_num / 2) * 4;

    ST_BYTE * dataptr = data + 3;

    ST_VARIANT stvar;
    for (ST_UINT16 index = data_addr / 2;
    	index < (data_addr + regi_num) / 2; ++index)
    {
    	ST_DUADDR  *duaddr = tablelist->GetItem(index);
    	TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
    	memset (&stvar, 0, sizeof (stvar));
    	ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		ST_FLOAT tempvar = variant_trans_handler<float> (stvar);//stvar.fVal;
		if (TRVar && TRVar->Coefficient != 0.0) {
			tempvar *= TRVar->Coefficient;
		}

		ST_UINT32     *transptr = (ST_UINT32*)&tempvar;
		*dataptr++ = (*transptr >> 0x18) & 0xFF;
		*dataptr++ = (*transptr >> 0x10) & 0xFF;
		*dataptr++ = (*transptr >> 0x08) & 0xFF;
		*dataptr++ = (*transptr >> 0x00) & 0xFF;
    }
    set_crc16_low(data, data[2]+3);
    this->Send(data, data[2]+5);
*/
}

void CModbusRTU::assemblydata(ST_BYTE *dbuf)
{
    dbuf[0] = 0x00;
    dbuf[1] = 0x00;

    calculationValue(getdData2tableList(0),getdData2tableList(1),getdData2tableList(2),getdData2tableList(3),&dbuf[2]);//2 3 4 5 总水表行度
    fv2UIntt16((getfData2tableList(4)+getfData2tableList(5)+getfData2tableList(6))+getfData2tableList(7),&dbuf[6]);//6 7         总水表瞬时流量

    //低区机泵状态
    //fv2UIntt16(getfData2tableList(8),&dbuf[8]);     //低区机泵状态
    waterStatus(1,&dbuf[8]);
    fv2UIntt16(getfData2tableList(9),&dbuf[10]);    //低区1#机泵电流
    fv2UIntt16(getfData2tableList(10),&dbuf[12]);    //低区2#机泵电流
    fv2UIntt16(getfData2tableList(11),&dbuf[14]);    //低区3#机泵电流
    fv2UIntt16(getfData2tableList(12),&dbuf[16]);   //低区变频器频率
    fv2UIntt16(getfData2tableList(13),&dbuf[18]);   //低区设定压力
    fv2UIntt16(getfData2tableList(14),&dbuf[20]);   //低区出水压力
    dv2UIntt32(getdData2tableList(15),&dbuf[22]);   //低区电量行度  22 23 24 25
    dbuf[26] = 0x00;dbuf[27] = 0x00;dbuf[28] = 0x00;dbuf[29] = 0x00;  //备用

    //中区机泵状态
    //fv2UIntt16(getfData2tableList(16),&dbuf[30]);   //中区机泵状态
    waterStatus(2,&dbuf[30]);
    fv2UIntt16(getfData2tableList(17),&dbuf[32]);   //中区1#机泵电流
    fv2UIntt16(getfData2tableList(18),&dbuf[34]);   //中区2#机泵电流
    fv2UIntt16(getfData2tableList(19),&dbuf[36]);   //中区3#机泵电流
    fv2UIntt16(getfData2tableList(20),&dbuf[38]);   //中区变频器频率
    fv2UIntt16(getfData2tableList(21),&dbuf[40]);   //中区设定压力
    fv2UIntt16(getfData2tableList(22),&dbuf[42]);   //中区出水压力
    dv2UIntt32(getdData2tableList(23),&dbuf[44]);   //中区电量行度  44 45 46 47
    dbuf[48] = 0x00;dbuf[49] = 0x00;dbuf[50] = 0x00;dbuf[51] = 0x00;  //备用

    //高区机泵状态
    //fv2UIntt16(getfData2tableList(24),&dbuf[52]);   //高区机泵状态
    waterStatus(3,&dbuf[52]);
    fv2UIntt16(getfData2tableList(25),&dbuf[54]);   //高区1#机泵电流
    fv2UIntt16(getfData2tableList(26),&dbuf[56]);   //高区2#机泵电流
    fv2UIntt16(getfData2tableList(27),&dbuf[58]);   //高区3#机泵电流
    fv2UIntt16(getfData2tableList(28),&dbuf[60]);   //高区变频器频率
    fv2UIntt16(getfData2tableList(29),&dbuf[62]);   //高区设定压力
    fv2UIntt16(getfData2tableList(30),&dbuf[64]);   //高区出水压力
    dv2UIntt32(getdData2tableList(31),&dbuf[66]);   //高区电量行度  66 67 68 69
    dbuf[70] = 0x00;dbuf[71] = 0x00;dbuf[72] = 0x00;dbuf[73] = 0x00;  //备用

    //超高区机泵状态
    //fv2UIntt16(getfData2tableList(32),&dbuf[74]);   //超高区机泵状态
    waterStatus(4,&dbuf[74]);
    fv2UIntt16(getfData2tableList(33),&dbuf[76]);   //超高区1#机泵电流
    fv2UIntt16(getfData2tableList(34),&dbuf[78]);   //超高区2#机泵电流
    fv2UIntt16(getfData2tableList(35),&dbuf[80]);   //超高区3#机泵电流
    fv2UIntt16(getfData2tableList(36),&dbuf[82]);   //超高区变频器频率
    fv2UIntt16(getfData2tableList(37),&dbuf[84]);   //超高区设定压力
    fv2UIntt16(getfData2tableList(38),&dbuf[86]);   //超高区出水压力
    dv2UIntt32(getdData2tableList(39),&dbuf[88]);   //超高区电量行度  88 89 90 91
    dbuf[92] = 0x00;dbuf[93] = 0x00;dbuf[94] = 0x00;dbuf[95] = 0x00;  //备用

    calculationValue(getdData2tableList(15),getdData2tableList(23),getdData2tableList(31),getdData2tableList(39),&dbuf[96]);//96 97 98 99 总电量行度
    fv2UIntt16(getfData2tableList(40),&dbuf[100]);   //低区进水压力

}
//   FF FF
void  CModbusRTU::waterStatus(int pos,ST_BYTE *pbuf)
{
    if(pos== 1)
    {
        if(getfData2tableList(41)==1)pbuf[1] = pbuf[1] | 0x01;   //1#泵状态
        if(getfData2tableList(41)==3)pbuf[1] = pbuf[1] | 0x02;
        if(getfData2tableList(41)==2)pbuf[1] = pbuf[1] | 0x04;

        if(getfData2tableList(42)==1)pbuf[1] = pbuf[1] | 0x08;    //2#泵状态
        if(getfData2tableList(42)==3)pbuf[1] = pbuf[1] | 0x10;
        if(getfData2tableList(42)==2)pbuf[1] = pbuf[1] | 0x20;

        if(getfData2tableList(43)==1)pbuf[1] = pbuf[1] | 0x40;   //3#泵状态
        if(getfData2tableList(43)==3)pbuf[1] = pbuf[1] | 0x80;

        if(getfData2tableList(43)==2)pbuf[0] = pbuf[0] | 0x01;

        if(getfData2tableList(45)==1)pbuf[0] = pbuf[0] |  0x02;  //手动/自动
                                                       //电源故障  无法采集
        if(getfData2tableList(8)==2)pbuf[0] = pbuf[0] |  0x08;   //低区变频器故障
    }
    if(pos== 2)
    {//中区
        if(getfData2tableList(46)==1)pbuf[1] = pbuf[1] | 0x01;   //1#泵状态
        if(getfData2tableList(46)==3)pbuf[1] = pbuf[1] | 0x02;
        if(getfData2tableList(46)==2)pbuf[1] = pbuf[1] | 0x04;

        if(getfData2tableList(47)==1)pbuf[1] = pbuf[1] | 0x08;    //2#泵状态
        if(getfData2tableList(47)==3)pbuf[1] = pbuf[1] | 0x10;
        if(getfData2tableList(47)==2)pbuf[1] = pbuf[1] | 0x20;

        if(getfData2tableList(48)==1)pbuf[1] = pbuf[1] | 0x40;   //3#泵状态
        if(getfData2tableList(48)==3)pbuf[1] = pbuf[1] | 0x80;

        if(getfData2tableList(48)==2)pbuf[0] = pbuf[0] | 0x01;

        if(getfData2tableList(50)==1)pbuf[0] = pbuf[0] | 0x02;  //手动/自动
                                                       //电源故障  无法采集
        if(getfData2tableList(16)==2)pbuf[0] = pbuf[0] | 0x08;   //中区变频器故障
    }
    if(pos== 3)
    {//高区
        if(getfData2tableList(51)==1)pbuf[1] |= 0x01;   //1#泵状态
        if(getfData2tableList(51)==3)pbuf[1] |= 0x02;
        if(getfData2tableList(51)==2)pbuf[1] |= 0x04;

        if(getfData2tableList(52)==1)pbuf[1] |= 0x08;    //2#泵状态
        if(getfData2tableList(52)==3)pbuf[1] |= 0x10;
        if(getfData2tableList(52)==2)pbuf[1] |= 0x20;

        if(getfData2tableList(53)==1)pbuf[1] |= 0x40;   //3#泵状态
        if(getfData2tableList(53)==3)pbuf[1] |= 0x80;

        if(getfData2tableList(53)==2)pbuf[0] |= 0x01;

        if(getfData2tableList(55)==1)pbuf[0] |= 0x02;  //手动/自动
                                                       //电源故障  无法采集
        if(getfData2tableList(24)==2)pbuf[0] |= 0x08;   //高区变频器故障
    }
    if(pos== 4)
    {
        if(getfData2tableList(56)==1)pbuf[1] |= 0x01;   //1#泵状态
        if(getfData2tableList(56)==3)pbuf[1] |= 0x02;
        if(getfData2tableList(56)==2)pbuf[1] |= 0x04;

        if(getfData2tableList(57)==1)pbuf[1] |= 0x08;   //2#泵状态
        if(getfData2tableList(57)==3)pbuf[1] |= 0x10;
        if(getfData2tableList(57)==2)pbuf[1] |= 0x20;

        if(getfData2tableList(58)==1)pbuf[1] |= 0x40;   //3#泵状态
        if(getfData2tableList(58)==3)pbuf[1] |= 0x80;

        if(getfData2tableList(58)==2)pbuf[0] |= 0x01;

        if(getfData2tableList(60)==1)pbuf[0] |= 0x02;  //手动/自动
                                                       //电源故障  无法采集
        if(getfData2tableList(32)==2)pbuf[0] |= 0x08;  //超高区变频器故障
    }
}

void     CModbusRTU::calculationValue(double d1,double d2,double d3,double d4,ST_BYTE *dataptr)
{
    double totoal  = d1+d2+d3+d4;
    dv2UIntt32(totoal,dataptr);
}

void    CModbusRTU::fv2UIntt16(float fvalue,ST_BYTE *dataptr)
{
    ST_UINT16 tmp16 = (ST_UINT16)fvalue;
    ST_UINT16 *transptr = (ST_UINT16*)&tmp16;
    *dataptr++  = (*transptr >> 0x08) & 0xFF;
    *dataptr++ = (*transptr >> 0x00) & 0xFF;
}

void    CModbusRTU::fv2UIntt32(float fvalue,ST_BYTE *dataptr)
{
    ST_UINT32 tmp16 = (ST_UINT32)fvalue;
    ST_UINT32     *transptr = (ST_UINT32*)&tmp16;
    *dataptr++ = (*transptr >> 0x18) & 0xFF;
    *dataptr++ = (*transptr >> 0x10) & 0xFF;
    *dataptr++ = (*transptr >> 0x08) & 0xFF;
    *dataptr++ = (*transptr >> 0x00) & 0xFF;
}

void    CModbusRTU::dv2UIntt32(double dvalue,ST_BYTE *dataptr)
{
    ST_UINT32 tmp16 = (ST_UINT32)dvalue;
    ST_UINT32     *transptr = (ST_UINT32*)&tmp16;
    *dataptr++ = (*transptr >> 0x18) & 0xFF;
    *dataptr++ = (*transptr >> 0x10) & 0xFF;
    *dataptr++ = (*transptr >> 0x08) & 0xFF;
    *dataptr++ = (*transptr >> 0x00) & 0xFF;
}

ST_FLOAT CModbusRTU::getfData2tableList(int index)
{
    TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (FC_READ_INPUT_REGISTER, trantable, tablelist))
		return 0;

    if(index>(tablelist->GetCount())){
        ShowMessage("index more than tablelist Count!");
        return 0;
    }

    ST_VARIANT stvar;
    ST_DUADDR  *duaddr = tablelist->GetItem(index);
    TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
    memset (&stvar, 0, sizeof (stvar));
    ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
    if (nR < 0) stvar.ulVal = 0;

    ST_FLOAT tempvar = variant_trans_handler<float> (stvar);//stvar.fVal;
/*    if (TRVar && TRVar->Coefficient != 0.0) {
        tempvar *= TRVar->Coefficient;
    }*/
    return tempvar;
}

ST_DOUBLE CModbusRTU::getdData2tableList(int index)
{
    TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (FC_READ_INPUT_REGISTER, trantable, tablelist))
		return 0;

    if(index>(tablelist->GetCount())){
        ShowMessage("index more than tablelist Count!");
        return 0;
    }

    ST_VARIANT stvar;
    ST_DUADDR  *duaddr = tablelist->GetItem(index);
    TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
    memset (&stvar, 0, sizeof (stvar));
    ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
    if (nR < 0) stvar.ulVal = 0;

    ST_DOUBLE tempvar = variant_trans_handler<double> (stvar);//stvar.fVal;
/*    if (TRVar && TRVar->Coefficient != 0.0) {
        tempvar *= TRVar->Coefficient;
    }*/
    return tempvar;
}



void CModbusRTU::SendKeepRegisterData (ST_BYTE * pbuf, ST_INT len)
{
	ST_UINT16 regi_num = pbuf[5] + pbuf[4] * 256;
	if (regi_num < 1 || regi_num > 0x7D || regi_num / 4 < 1)
	{
		ErrorResponse (pbuf[1], EC_ILLEGAL_VALUE);
		return ;
	}
	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (FC_READ_KEEP_REGISTER, trantable, tablelist))
		return ;

	int32_t list_size = tablelist->GetCount();
	if (list_size <= 0) {
		list_size = 0;
	}

	ST_UINT16 data_addr = pbuf[3] + pbuf[2] * 256;
	if (list_size < data_addr / 4 + regi_num / 4) {
		ErrorResponse (pbuf[1], EC_ILLEGAL_DATA_ADDR);
		return ;
	}

	ST_BYTE data[256] = {0};
	data[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[1] = FC_READ_KEEP_REGISTER;
	data[2] = (regi_num / 4) * 8;

	ST_BYTE * dataptr = data + 3;

	ST_VARIANT stvar;
	for (ST_UINT16 index = data_addr / 4;
		index < (data_addr + regi_num) / 4; ++index)
	{
    	ST_DUADDR  *duaddr = tablelist->GetItem(index);
    	TRVariable *TRVar  = trantable->GetTRVariable(duaddr);
    	memset (&stvar, 0, sizeof (stvar));
    	ST_INT nR = GetVariableValueByAddr(*duaddr, stvar);
		if (nR < 0) stvar.ulVal = 0;

		double tempvar = variant_trans_handler<double> (stvar);
		switch (stvar.vt) {
			case VALType_Double:
				tempvar = stvar.dtVal; break;
			case VALType_Float:
				tempvar = stvar.fVal;  break;
		}
		if (TRVar && TRVar->Coefficient != 0.0) {
			tempvar *= TRVar->Coefficient;
		}

		ST_UINT64     *transptr = (ST_UINT64*)&tempvar;
		*dataptr++ = (*transptr >> 0x38) & 0xFF;
		*dataptr++ = (*transptr >> 0x30) & 0xFF;
		*dataptr++ = (*transptr >> 0x28) & 0xFF;
		*dataptr++ = (*transptr >> 0x20) & 0xFF;
		*dataptr++ = (*transptr >> 0x18) & 0xFF;
		*dataptr++ = (*transptr >> 0x10) & 0xFF;
		*dataptr++ = (*transptr >> 0x08) & 0xFF;
		*dataptr++ = (*transptr >> 0x00) & 0xFF;
	}
	set_crc16_low (data, data[2] + 3);
	this->Send    (data, data[2] + 5);
}

void CModbusRTU::ErrorResponse (ST_BYTE fc, ST_BYTE ec)
{
	ST_BYTE data[8] = {0};
	data[0] = this->GetDevice()->GetDeviceInfo()->Address;
	data[1] = 0x80 + fc;
	data[2] = ec;
	set_crc16_low (data, 3);

	this->Send (data, 5);
}


void CModbusRTU::TaskHandlerToLower(ST_BYTE * pbuf, ST_INT len)
{
    if(!TransToLower(pbuf))
    {
        ErrorResponse (pbuf[1], FC_WRITE_COIL);
        return ;
    }
    ST_BYTE data[8];
    data[0] = this->GetDevice()->GetDeviceInfo()->Address;
	data[1] = 0x05;
	data[2] = pbuf[2];
	data[3] = pbuf[3];
	data[4] = pbuf[4];
	data[5] = pbuf[5];
    set_crc16_low (data, 6);
    this->Send(data,8);

}
//01 05 00 00 FF 00 8C 3A
ST_BOOLEAN  CModbusRTU::TransToLower(ST_BYTE *pbuf)
{
    ST_UINT16 reg_index = (pbuf[2]*256+pbuf[3])-0x6001;
/*	if (regi_num < 1 || regi_num > 0x7D || regi_num / 4 < 1)
	{
		ErrorResponse (pbuf[1], EC_ILLEGAL_VALUE);
		return ;
	}*/

	TransferTable *trantable   = NULL;
	List<ST_DUADDR> *tablelist = NULL;

	if (! CheckTransferTableExist (FC_WRITE_COIL, trantable, tablelist))
		return false;

    int tCount = tablelist->GetCount();
    if(reg_index>=tCount)
    {
//        ErrorResponse (pbuf[1], FC_WRITE_COIL);
		return false;
    }
    ST_BOOLEAN bVal = false;
    if(pbuf[4] == 0xFF)
        bVal = true;

    ST_DUADDR  *pda = tablelist->GetItem(reg_index);
    //发送遥控选择
    ProtocolTask *task1 		= new ProtocolTask;
	task1->isTransfer 		= true;
	task1->transChannelId 	= -1;
	task1->transDeviceId 	= pda->device;
	task1->taskAddr 		= pda->addr;
	task1->taskValue 		= bVal;
    task1->taskCmdCode       = 0x00; //遥控执行
    strncpy(task1->taskCmd, "devicecontrol", sizeof (task1->taskCmd));
	Transfer(task1);
	delete task1;

//    sleep(5000);
	//发送遥控执行
	ProtocolTask *task2 		= new ProtocolTask;
	task2->isTransfer 		= true;
	task2->transChannelId 	= -1;
	task2->transDeviceId 	= pda->device;
	task2->taskAddr 			= pda->addr;
	task2->taskValue 		= bVal;
    task2->taskCmdCode       = 0x01; //遥控执行
    strncpy(task2->taskCmd, "devicecontrol", sizeof (task2->taskCmd));
    //if(!strcmp(m_curTask.taskCmd,"devicecontrol"))
	Transfer(task2);
	delete task2;


	return true;
}



void CModbusRTU::checkTime(ST_BYTE *pbuf)
{
    struct tm *p = new struct tm();
	struct timeval tv;
	struct timezone tz;
	gettimeofday (&tv , &tz);//获取时区保存tz中

    int c_year = pbuf[7]*256+pbuf[8];
    int c_mon = pbuf[9]*256+pbuf[10];
    int c_day = pbuf[11]*256+pbuf[12];
    int c_hour = pbuf[13]*256+pbuf[14];
    int c_min = pbuf[15]*256+pbuf[16];
    int c_sec = pbuf[17]*256+pbuf[18];

	p->tm_year = c_year - 1900;
	p->tm_mon = c_mon-1;;
	p->tm_mday = c_day;
	p->tm_hour = c_hour;
	p->tm_min = c_min;
	p->tm_sec = c_sec;
	time_t utc_t = mktime(p);
//	delete(p);
	tv.tv_sec = utc_t;
	tv.tv_usec = 0;
	settimeofday (&tv, &tz);

    if(settimeofday (&tv, &tz) < 0)
    {
        this->ShowMessage("set time error!");
    }
    else
    {
        this->ShowMessage("set time successful!");
        CTRespon();
    }

}

void CModbusRTU::CTRespon()
{
    ST_BYTE sendbuf[18];
    sendbuf[0] = 0x01;
    sendbuf[1] = 0x10;
    sendbuf[2] = 0x00;
    sendbuf[3] = 0x32;
    sendbuf[4] = 0x00;
    sendbuf[5] = 0x06;
    set_crc16_low (sendbuf, 6);
    this->Send(sendbuf,8);
}

void CModbusRTU::getbefordayString(int bTime,char *dest)
{
    time_t now;
    struct tm  *ts;
    //char yearchar[80];
    now = time(NULL);
    ts = localtime(&now);
    ts->tm_mday = ts->tm_mday-bTime;
    mktime(ts); /* Normalise ts */
    strftime(dest, sizeof(dest), "%Y%m%d", ts);
}

void  CModbusRTU::GetHistoryData(ST_BYTE *pbuf)
{
    int day = (pbuf[4]&0xF0)>>4;
    int times = (pbuf[4]&0x0F)*256+(pbuf[5]);

    Memset(m_historyData,0,sizeof(m_historyData));
    int res = datacenter->getHistoryData(day,times,m_historyData);
    if(res == -1){
        ErrorResponse (pbuf[1], EC_ILLEGAL_VALUE);
		return ;
    }

    //printf("m_hisdlist[2][3][4][5]:0x%x 0x%x 0x%x 0x%x\n",m_historyData[2],m_historyData[3],m_historyData[4],m_historyData[5]);

    ST_BYTE data[256] = {0};
	data[0] = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;
	data[1] = FC_WRITE_REGISTER;//0x06
	data[2] = 0x01;
	data[3] = 0x00;
	data[4] = pbuf[4];
	data[5] = pbuf[5];
	set_crc16_low (data, 6);
    this->Send(data,8);

}



