
#include "modbustcp.h"

#include "Device.h"

#pragma pack(push,1)

struct mbap_head
{
	inline ST_UINT16 get_t_id ()
		{ return (t_h_id / 256 + (t_h_id % 256) * 256); }
	inline ST_UINT16 get_p_id ()
		{ return (ptl_id / 256 + (ptl_id % 256) * 256); }
	inline ST_UINT16 get_len ()
		{ return (length / 256 + (length % 256) * 256); }

	inline void set_len (ST_UINT16 len)
		{ length = (len / 256 + (len % 256) * 256); }

	ST_UINT16  t_h_id;
	ST_UINT16  ptl_id;
	ST_UINT16  length;
	ST_BYTE    unit_id;
};

#pragma pack(pop)

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

	enum ExceptionCode
	{
		EC_NORMAL              = 0x00,
		EC_ILLEGAL_FUNC        = 0x01,
		EC_ILLEGAL_DATA_ADDR   = 0x02,
		EC_ILLEGAL_VALUE       = 0x03,
		EC_SLAVE_FAILURE       = 0x04,
		EC_CONFIRM             = 0x05,
		EC_SLAVE_BUSYNESS      = 0x06,
		EC_INVALID_GATEWAY     = 0x0A,
		EC_DEVICE_NO_ASK       = 0x0B
	};


int check_func_code (ST_BYTE * pdata)
{
	switch (pdata[7]) {
		case FC_READ_DO_STATE:
		case FC_READ_DI_STATE:
		case FC_READ_INPUT_REGISTER:
		case FC_READ_KEEP_REGISTER:
		case FC_WRITE_REGISTER:
		case FC_WRITE_COIL: {
			return EC_NORMAL;
		}
		case FC_WRITE_COIL_MUIL: {
			int num   = pdata[10] * 256 + pdata[11];
			int count = num / 8 + (num % 8 ? 1 : 0);
			if (num < 1 || num > 0x07B0 || count != pdata[12])
				return EC_ILLEGAL_VALUE;

			return EC_NORMAL;
		}
		case FC_WRITE_REGISTER_MUIL: {
			int num   = pdata[10] * 256 + pdata[11];
			int count = 2 * num;
			if (num < 1 || num > 0x7B || count != pdata[12])
				return EC_ILLEGAL_VALUE;

			return EC_NORMAL;
		}
	}
	return EC_ILLEGAL_FUNC;
}

Protocol * CreateInstace()
{
	return new ModbusTcp();
}

ModbusTcp::ModbusTcp()
{}

ModbusTcp::~ModbusTcp()
{}

void ModbusTcp::Init ()
{}

void ModbusTcp::Uninit()
{}

void ModbusTcp::OnRead(ST_BYTE* pbuf, ST_INT& readed)
{
	readed = 0;
	if(!this->GetCurPort())
		return;

	int	len = this->GetCurPort()->PickBytes(pbuf, 7, 3000);
	if (len < 7)
	{
		this->ShowMessage("Insufficient data length!");
		this->ShowRecvFrame(pbuf, len);
		this->GetCurPort()->Clear();
		return;
	}

	mbap_head * mh_ptr = (mbap_head*) pbuf;

	if(mh_ptr->get_p_id() != 0)
	{
		this->ShowMessage("Not a modbus protocol identifier!");
		this->ShowRecvFrame(pbuf, len);
		this->GetCurPort()->Clear();
		return;
	}
	if(this->GetCurPort()->PickBytes(pbuf, 7, 2000) == 7)
	{
		if(mh_ptr->unit_id != (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address)
		{
			this->ShowMessage("Addresses do not match!");
			this->ShowRecvFrame(pbuf, len);
			this->GetCurPort()->Clear();
			return;
		}
		int datalen = mh_ptr->get_len() + 6;
		len = this->GetCurPort()->PickBytes(pbuf, datalen, 3000);
		if(datalen == len)
		{
			len = this->GetCurPort()->ReadBytes(pbuf, datalen, 1000);

			int ec = check_func_code(pbuf);
			if (ec) {
				ErrorResponse (pbuf, ec);
				this->GetCurPort()->Clear();
				return;
			}
			readed = len;
			return;
		}
		else
		{
			this->ShowMessage("Insufficient data length!");
			this->ShowRecvFrame(pbuf, len);
			this->GetCurPort()->Clear();
		}
	}
}

ST_BOOLEAN ModbusTcp::OnSend()
{
	return true;
}

bool ModbusTcp::CheckTransferTableExist (int index, TransferTable*& table, List<ST_DUADDR>*& list)
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

ST_BOOLEAN ModbusTcp::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
	switch (pbuf[7]) {
		case FC_READ_DO_STATE: {
			ErrorResponse (pbuf, EC_ILLEGAL_FUNC);
		} break;
		case FC_READ_DI_STATE: {
			SendDIStateData       (pbuf, len);
		} break;
		case FC_READ_INPUT_REGISTER: {
			SendInputRegisterData (pbuf, len);
		} break;
		case FC_READ_KEEP_REGISTER: {
			SendKeepRegisterData  (pbuf, len);
		} break;
		case FC_WRITE_COIL: {
			ErrorResponse (pbuf, EC_ILLEGAL_FUNC);
		} break;
		case FC_WRITE_REGISTER: {
			ErrorResponse (pbuf, EC_ILLEGAL_FUNC);
		} break;
		case FC_WRITE_COIL_MUIL: {
			ErrorResponse (pbuf, EC_ILLEGAL_FUNC);
		} break;
		case FC_WRITE_REGISTER_MUIL: {
			ErrorResponse (pbuf, EC_ILLEGAL_FUNC);
		} break;
	}
	return true;
}

void ModbusTcp::SendDIStateData (ST_BYTE * pbuf, ST_INT len)
{
	int state_num = pbuf[11] + pbuf[10] * 256;
	if (state_num < 1 || state_num > 0x07D0) {
		ErrorResponse (pbuf, EC_ILLEGAL_VALUE);
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

	ST_UINT16 data_addr = pbuf[9] + pbuf[8] * 256;
	if (list_size < data_addr + state_num) {
		ErrorResponse (pbuf, EC_ILLEGAL_DATA_ADDR);
		return ;
	}

	ST_BYTE data[320] = {0};
	mbap_head * mhptr = (mbap_head*) data;
	memcpy(data, pbuf, sizeof (mbap_head));
	mhptr->unit_id = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;

	data[7] = FC_READ_DI_STATE;
	data[8] = state_num / 8 + (state_num % 8 ? 1 : 0);

	mhptr->set_len(data[8] + 3);

	ST_BYTE * dataptr = data + 8;

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
	this->Send (data, mhptr->get_len() + 6);
}

void ModbusTcp::SendInputRegisterData (ST_BYTE * pbuf, ST_INT len)
{
	ST_UINT16 regi_num = pbuf[11] + pbuf[10] * 256;
	if (regi_num < 1 || regi_num > 0x7D || regi_num / 2 < 1)
	{
		ErrorResponse (pbuf, EC_ILLEGAL_VALUE);
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

	ST_UINT16 data_addr = pbuf[9] + pbuf[8] * 256;
	if (list_size < data_addr / 2 + regi_num / 2) {
		ErrorResponse (pbuf, EC_ILLEGAL_DATA_ADDR);
		return ;
	}

    ST_BYTE data[320] = {0};
	mbap_head * mhptr = (mbap_head*) data;
	memcpy(data, pbuf, sizeof (mbap_head));
	mhptr->unit_id = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;

    data[7] = FC_READ_INPUT_REGISTER;
    data[8] = (regi_num / 2) * 4;

    mhptr->set_len(data[8] + 3);

    ST_BYTE * dataptr = data + 9;

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
    this->Send (data, mhptr->get_len() + 6);

}

void ModbusTcp::SendKeepRegisterData (ST_BYTE * pbuf, ST_INT len)
{
	ST_UINT16 regi_num = pbuf[11] + pbuf[10] * 256;
	if (regi_num < 1 || regi_num > 0x7D || regi_num / 4 < 1)
	{
		ErrorResponse (pbuf, EC_ILLEGAL_VALUE);
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

	ST_UINT16 data_addr = pbuf[9] + pbuf[8] * 256;
	if (list_size < data_addr / 4 + regi_num / 4) {
		ErrorResponse (pbuf, EC_ILLEGAL_DATA_ADDR);
		return ;
	}

	ST_BYTE data[320] = {0};
	mbap_head * mhptr = (mbap_head*) data;
	memcpy(data, pbuf, sizeof (mbap_head));
	mhptr->unit_id = (ST_BYTE)this->GetDevice()->GetDeviceInfo()->Address;

	data[7] = FC_READ_KEEP_REGISTER;
	data[8] = (regi_num / 4) * 8;

	mhptr->set_len(data[8] + 3);

	ST_BYTE * dataptr = data + 9;

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
	this->Send (data, mhptr->get_len() + 6);
}

ST_BOOLEAN ModbusTcp::IsSupportEngine(ST_INT engineType)
{
	return true;
}

void ModbusTcp::ErrorResponse (const ST_BYTE * pdata, ST_BYTE ec)
{
	ST_BYTE data[16] = {0};
	mbap_head * mhptr = (mbap_head*) data;
	memcpy(data, pdata, sizeof (mbap_head));
	data[7] = 0x80 + pdata[7];
	data[8] = ec;

	mhptr->set_len(3);

	this->Send (data, mhptr->get_len() + 6);
}
