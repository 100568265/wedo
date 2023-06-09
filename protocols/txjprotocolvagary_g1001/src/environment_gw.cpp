
#include "environment_gw.h"

#include "EngineBase.h"
#include "DataCache.h"    //数据缓冲类
#include "Device.h"       //通迅设备类
#include "Devices.h"      //通迅设备管理类
#include "Channel.h"

#include "boost/shared_ptr.hpp"

#include <stdio.h>

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

//=========================================================================

#pragma pack(push,1)	// 紧凑对齐

const static uint32_t start_header_value = 0x90EB90EB;

class DataFrame
{
	DataFrame();
	DataFrame(const DataFrame&);
	DataFrame operator= (const DataFrame&);
public:
	typedef DataFrame this_type;

	uint32_t start;		// 启动字符、 帧头
	uint8_t  src_type;	// 源类型
	uint8_t  dest_type;	// 目标类型
	uint16_t src_addr;	// 源地址
	uint16_t dest_addr;	// 目标地址
	uint8_t  func_code;	// 功能码
	uint16_t length;	// 数据长度；数据正文的字节数

	inline int32_t  fullLen () const { return sizeof(this_type) + length + 2; }

	inline uint16_t getCRC () const;

	inline uint8_t* getDataPtr () { return (uint8_t*)(this + 1); }
	inline uint8_t* getCRCPtr  () { return (uint8_t*)(this + 1) + length; }
};

#pragma pack(pop)		// 恢复默认内存对齐

inline uint16_t DataFrame::getCRC() const
{
	uint8_t * dataptr = (uint8_t*)(this + 1) + length;
	return dataptr[0] + dataptr[1] * 256;
}

//=========================================================================

class gw_handler
{
public:
	gw_handler() : _devsptr(0), _send_interval(0) {}
	~gw_handler() {}

//	bool send (EnvironmentGw & proto);

	bool proc (EnvironmentGw & proto, void * pbuf, int32_t len);

	Devices * _devsptr;

	// std::map<devsn, devid>
	std::map<int32_t, int32_t> _devset;

	long _send_interval;
};

/*bool gw_handler::send(EnvironmentGw & proto)
{
}*/

bool gw_handler::proc(EnvironmentGw & proto, void * pbuf, int32_t len)
{

	DataFrame * dataptr = (DataFrame*) pbuf;

    if (dataptr->length == 0)
    {
    	return true;
    }

   	if(!_devsptr)
   		_devsptr = proto.GetDevice()->GetChannel()->GetCEngine()->GetDevices();

    if (dataptr->func_code == 0x0C) {
	    std::map<int32_t, int32_t>::iterator it = _devset.find(0);
	    if (it == _devset.end()) {
	    	proto.ShowMessage("Not found main device.");
	    	return true;
	    }

	   	Device * devptr = _devsptr->GetDevice(it->second);
	   	if (! devptr) {
	   		proto.ShowMessage("Not found main device for addr.");
	   		return true;
	   	}

	   	Protocol * protoptr = (Protocol*)devptr->GetProtocol();
	   	if (!protoptr) {
	   		proto.ShowMessage("Not found main protocol instance for addr.");
	   		return true;
	   	}

	   	uint8_t * ptr = dataptr->getDataPtr();
	   	int count = *ptr++;
	   	for (int i = 0; i < count; ++i, ptr += sizeof(uint16_t))
	   	{
	   		uint16_t value = 0;
	   		memcpy (&value, ptr, sizeof (value));
	   		protoptr->UpdateValue (10000 + i, value);
	   	}
    }

    if (dataptr->func_code != 0xFF)
    	return true;

    std::string data ((const char*)dataptr->getDataPtr(), dataptr->length);

//------------------------------------------------------------------------

    int32_t devsn = 0; char order[16] = {0}; int32_t value = -1;
   	if (sscanf (data.c_str(), "=%d,%9[^0-9]%d#", &devsn, order, &value) != 3)
   	{
   		data.insert(0, "Unable to parse data, recv for: ");
   		proto.ShowMessage(data.c_str());
   		return true;
   	}

    std::map<int32_t, int32_t>::iterator it = _devset.find((int32_t)devsn);
    if (it == _devset.end()) {
    	proto.ShowMessage("Not found device for sn.");
    	return true;
    }

   	Device * devptr = _devsptr->GetDevice(it->second);
   	if (! devptr) {
   		proto.ShowMessage("Not found device for addr.");
   		return true;
   	}

   	Protocol * protoptr = (Protocol*)devptr->GetProtocol();
   	if (!protoptr) {
   		proto.ShowMessage("Not found protocol instance for addr.");
   		return true;
   	}

   	switch(order[0]) {
      	// 开关遥控返回。 忽略手动控制返回。
   		case 'm': {
   			if (value > 7) break;

   			ProtocolTask task;
   			strcpy (task.taskCmd , "devicecontrol");
   			memcpy(&task.taskAddr, order, sizeof(task.taskAddr));

   			task.taskValue   = value;
   			task.taskCmdCode = 1;
   			task.taskResult.resultCode = 0;

   			((EnvironmentGw*)protoptr)->TransferTask(task);
   		} //no break;
   		// 开关轮询返回。
   		case 's': {
   			for (int32_t index = 0; index < 16; ++index)
   			{
   				bool state = (bool)(value & (0x01 << index));
   				protoptr->UpdateValue(index, state);
   			}
   		} break;

   		// 红外遥控返回。
   		case 'a': {
   			ProtocolTask task;
   			strcpy (task.taskCmd , "devicecontrol");
   			memcpy(&task.taskAddr, order, sizeof(task.taskAddr));

   			task.taskValue   = value;
   			task.taskCmdCode = 1;
   			task.taskResult.resultCode = 0;

   			((EnvironmentGw*)protoptr)->TransferTask(task);
   		} break;
   		// 可能是水浸 或 烟感。
   		case 'k': {
   			protoptr->UpdateValue(0, 1);
   		} break;

   		// 读湿度返回
   		case 'e': {
			protoptr->UpdateValue(0, value);
   		} break;

   		// 读温度返回
   		case 'g': {
   			protoptr->UpdateValue(1, value);
   		} break;
   	}

//------------------------------------------------------------------------

//	EXplainValue(dataptr->getDataPtr(), dataptr->length);

    data.insert(0, "Recv Penetrate Data: ");
    proto.ShowMessage(data.c_str());

	return true;
}

//=========================================================================

template<typename _Ty_key, typename _Ty_value>
class channel_singleton
{
	channel_singleton() {}
	channel_singleton(const channel_singleton&);
	channel_singleton operator= (const channel_singleton&);
public:
	typedef channel_singleton this_type;

	typedef _Ty_key	  key_type;
	typedef boost::shared_ptr<_Ty_value> mapped_type;

	typedef std::map <key_type, mapped_type> set_type;

	~channel_singleton() {}

	void enrollment (key_type chlid, mapped_type value)
	{
		typename set_type::iterator it = _map.find (chlid);
		if (it == _map.end())
			_map.insert(make_pair(chlid, value));
	}

	mapped_type get(key_type chlid)
	{
		typename set_type::iterator it = _map.find (chlid);
		return ( it != _map.end() ? it->second: mapped_type() );
	}

	void unenrollment (key_type chlid)
	{
		_map.erase (chlid);
	}

	static this_type& instance()
	{
		static this_type object;
		return object;
	}

private:
	set_type _map;
};
typedef channel_singleton<int32_t, gw_handler> gw_singleton;

//=========================================================================

EnvironmentGw* CreateInstace()
{
    return new EnvironmentGw();
}

EnvironmentGw::EnvironmentGw():
_chlid(0),
_devsn(0),
_sendindex(0),
_isread(0)
{

}

EnvironmentGw::~EnvironmentGw()
{
	Uninit();
}

void EnvironmentGw::Init()
{
	_chlid = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelID;
	gw_singleton::instance().enrollment(_chlid, gw_singleton::mapped_type(new gw_handler()));

	char * parameters = this->GetDevice()->GetDeviceInfo()->Addressex;
	char devsn[16] = {0}; char order[256] = {0};

	char * sn_str = strstr (parameters, "-sn");
	if (!sn_str)
		return ;

	char * rd_str = strstr (parameters, "-r" );
	if (rd_str)
		_isread = true;

	sscanf (sn_str, "%*[^0-9]%[0-9]", devsn);
	_devsn = atoi (devsn);

	gw_singleton::mapped_type sptr(gw_singleton::instance().get(_chlid));
	if (!sptr)
		return ;
	sptr->_devset[_devsn] = this->GetDevice()->GetDeviceInfo()->DeviceId;

	ST_INT & interval = this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval;
	if (interval > 0) {
		sptr->_send_interval = interval;
		interval = 0;
	}
	_interval = sptr->_send_interval;
}

void EnvironmentGw::Uninit()
{
	gw_singleton::instance().unenrollment(_chlid);
	_chlid = 0;
	_devsn = 0;

	this->GetDevice()->GetChannel()->GetChannelInfo()->ChannelInterval = _interval;
}

void EnvironmentGw::OnRead(ST_BYTE * pbuf, ST_INT& readed)
{
    if (!this->GetCurPort())
        return ;
    readed = 0;

    int len = this->GetCurPort()->PickBytes(pbuf , 15, 1000);
    if (len < 15) {
        this->GetCurPort()->Clear();
        return ;
    }
    if (memcmp (&start_header_value, pbuf, sizeof(start_header_value)))
	{
    	ShowMessage ("Frame header error!");
        this->GetCurPort()->Clear();
        return ;
	}

    DataFrame * frameptr = (DataFrame*) pbuf;

        len = this->GetCurPort()->PickBytes(pbuf, frameptr->fullLen(), 1000);
    if (len < frameptr->fullLen()) {
    	ShowMessage ("Insufficient data length.");
        this->GetCurPort()->Clear();
        return ;
    }
    this->GetCurPort()->ReadBytes(pbuf, len);

    if (frameptr->getCRC() != get_crc16(pbuf, frameptr->fullLen() - 2))
    {
    	ShowMessage ("Check error!");
        this->GetCurPort()->Clear();
        return ;
    }
    readed = frameptr->fullLen();
}

ST_BOOLEAN EnvironmentGw::OnSend()
{
	if (this->HasTask() && this->GetTask(&_task))
	{
		Thread::SLEEP(_interval);

		if (!strcmp (_task.taskCmd, "devicecontrol"))
		{
			if (_task.taskAddr & 0x8000) {
				this->UpdateValue (_task.taskAddr & 0x7FFF, _task.taskValue);
				_task.taskResult.resultCode = 0;
				_task.isTransfer = 1;
				this->Transfer (&_task);
			} else {
				char data [32] = {0};
				char order[16] = {0};
				memcpy (order, &_task.taskAddr, sizeof(_task.taskAddr));

				snprintf(data, sizeof(data), "=%09d,%s%d#",
					_devsn, order, (int32_t)_task.taskValue);

				this->SendString(data);

			}
			Memset(&_task, 0, sizeof(_task));
			return true;
		}
	}

	if (!_isread)
		return true;

	const DeviceInfo & info = *this->GetDevice()->GetDeviceInfo();

	if (!_devsn) {
        Thread::SLEEP(_interval);
		SendAskYC();
		return true;
	}

	if (info.DataAreasCount == 0 || !info.DataAreas)
		return true;

	_sendindex = (_sendindex + 1) % info.DataAreasCount;

	char order[16] = {0};
	if (sscanf(info.DataAreas[_sendindex].areaName,
			"%[A-Z,0-9]", order) != 1)
		return true;

	Thread::SLEEP(_interval);
/*	gw_singleton::mapped_type sptr(gw_singleton::instance().get(_chlid));
	if (!sptr)
		return true;

	return sptr->send(*this);*/

	char data[32] = {0};
	snprintf (data, sizeof(data), "=%09d,%s#", _devsn, order);


	return this->SendString (data);
}

ST_BOOLEAN EnvironmentGw::OnProcess(ST_BYTE* pbuf, ST_INT len)
{
	gw_singleton::mapped_type sptr(gw_singleton::instance().get(_chlid));
	if (!sptr)
		return true;

	return sptr->proc (*this, pbuf, len);
}

ST_BOOLEAN EnvironmentGw::IsSupportEngine(ST_INT engineType)
{
    return engineType == EngineBase::ENGINE_FULLING;
}

ST_BOOLEAN EnvironmentGw::SendString(const char * data)
{
	class buffer
	{
	public:
		buffer(ST_BYTE * buf) : ptr(buf) {}
		~buffer() { delete[] ptr; }

		ST_BYTE * ptr;
	};

	int paramlen = strlen (data);

	buffer sendbuf (new ST_BYTE[paramlen + 32]);
	DataFrame * dataptr = (DataFrame*) sendbuf.ptr;

	dataptr->start     = start_header_value;
	dataptr->src_type  = 0x02;
	dataptr->dest_type = 0x01;
	dataptr->src_addr  = 0x00;
	dataptr->dest_addr = 0x00;
	dataptr->func_code = 0xFF;
	dataptr->length    = paramlen;

	memcpy (dataptr->getDataPtr(), data, paramlen);

	uint16_t crc = get_crc16 (sendbuf.ptr, sizeof(DataFrame) + dataptr->length);
	memcpy(dataptr->getCRCPtr(), &crc, sizeof(crc));

	this->Send(sendbuf.ptr, dataptr->fullLen());

	std::string msg = "Send Penetrate Data: ";
	msg += data;

	this->ShowMessage (msg.c_str());

	return true;
}

void EnvironmentGw::SendAskYC ()
{
	uint8_t data[32] = {0};
	DataFrame * dataptr = (DataFrame*) data;

	dataptr->start     = start_header_value;
	dataptr->src_type  = 0x02;
	dataptr->dest_type = 0x01;
	dataptr->src_addr  = 0x00;
	dataptr->dest_addr = 0x00;
	dataptr->func_code = 0x0C;
	dataptr->length    = 0x00;

	uint16_t crc = get_crc16 (data, sizeof(DataFrame) + dataptr->length);
	memcpy(dataptr->getCRCPtr(), &crc, sizeof(crc));

	this->Send(data, dataptr->fullLen());
}

/*
void EnvironmentGw::EXplainValue(ST_BYTE *pbuf,int len)
{
	int nDeviceNo = (pbuf[7] - '0') * 100 + (pbuf[8] - '0') * 10 + (pbuf[9] - '0'); //得到后三位的序列号

    Device * dev = this->GetDevices()->GetDeviceByAddr(nDeviceNo);
    if (!dev) {
        return ;
    }
	DeviceInfo * devinfo = dev->GetDeviceInfo();
	if (!devinfo) {
		return;
	}
	switch (pbuf[11]) {
	    case 'm':  //m值返回状态
        {
            if (devinfo->Addressex[0] != 0x3D)
            {
                float fvalue = 0;
                if (len == 14)
                    fvalue = (pbuf[12] - '0');
                if (len == 15)
                    fvalue = (pbuf[12] - '0') * 10 + (pbuf[13] - '0');
                if (fvalue > 1)
                    fvalue = 1;
                ((CProtocol*)dev->GetProtocol())->UpdateValue(0, fvalue);
            }
        } break;
        case 'p':  //P红外返回P
        case 'k':  //烟感
		{
            float fvalue = 0;
			if (len == 14)
				fvalue = (pbuf[12] - '0');
			if (len == 15)
				fvalue = (pbuf[12] - '0') * 10 + (pbuf[13] - '0');
			if (fvalue > 1)
                fvalue = 1;
			((CProtocol*)dev->GetProtocol())->UpdateValue(0, fvalue);
		} break;
        case 'e':  //湿度
        {
            float fvalue = 0;
			if (len == 14)
				fvalue = (pbuf[12] - '0');
			if (len == 15)
				fvalue = (pbuf[12] - '0') * 10 + (pbuf[13] - '0');
			((CProtocol*)dev->GetProtocol())->UpdateValue(0, fvalue);
        } break;
        case 'g':  //温度
		{
            float fvalue = 0;
			if (len == 14)
				fvalue = (pbuf[12] - '0');
			if (len == 15)
				fvalue = (pbuf[12] - '0') * 10 + (pbuf[13] - '0');
			if (fvalue > 1)
                fvalue = 1;
			((CProtocol*)dev->GetProtocol())->UpdateValue(1, fvalue);
		} break;
        case 's':  //开关状态
		{
            float fvalue = 0;
			if (len == 14)
			{
				fvalue = (pbuf[12] - '0');
				if (fvalue > 1)
					fvalue = 1;
			}
			if (len == 15)
			{
				fvalue = (pbuf[12] - '0') * 10 + (pbuf[13] - '0');
			}
			((CProtocol*)dev->GetProtocol())->UpdateValue(0, fvalue);
		} break;
	}
}
*/
