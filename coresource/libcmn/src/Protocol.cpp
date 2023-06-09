//#include "stdafx.h"
#include "Protocol.h"
#include "Channel.h"
#include "Device.h"

Protocol::Protocol()
{
	m_bBreak=false;
}

Protocol::~Protocol()
{

}

ST_INT Protocol::GetVariableValue(ST_CHAR *fullName,ST_VARIANT &var)
{
	return GetVariableValueByName(fullName,var);
}

ST_INT Protocol::GetVariableValue(ST_DUADDR addr,ST_VARIANT &var)
{
	return GetVariableValueByAddr(addr,var);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_BYTE value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
	CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_UINT32 value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_UINT64 value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_CHAR value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_INT16 value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_INT value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_FLOAT value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_DOUBLE value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_CHAR *value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

ST_VOID Protocol::UpdateValue(ST_INT addrNo,ST_DECIMAL value,ST_INT channelId,ST_INT deviceId)
{
    if(channelId == -1 || deviceId == -1) {
        if(m_pChannel && m_pDevice) {
            channelId = m_pChannel->GetChannelInfo()->ChannelID;
            deviceId  = m_pDevice->GetDeviceInfo()->DeviceId;
        }
    }
    CmnRtInterface::UpdateValue(0, channelId, deviceId, addrNo, value);
}

//List<ST_Variable> *Protocol::GetVariables(ST_CHAR *fullName,ST_INT addr)
//{
//	return CmnRtInterface::GetVariables(fullName,addr);
//}


