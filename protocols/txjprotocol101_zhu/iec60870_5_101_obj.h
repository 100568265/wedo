
#ifndef __IEC_60870_5_101_OBJ_H
#define __IEC_60870_5_101_OBJ_H

#include "iec60870_5_101_types.h"
#include <cstring>
#include <cassert>
// #include <exception>

NAMESPACE_BEGIN_IEC101

struct Option
{
	Option () : outime(2000), version(2002), laddr(1),
		paddr(1), fullcode(32767), yc_type(TYP::M_ME_NC_1) {}

	int32_t outime;
	int16_t version;// 2002 or 1997
	uint8_t laddr;  // link address
	uint8_t paddr;
	int16_t fullcode;
	TYP::type yc_type;
};

class Parser
{
public:
	Parser (Option* optr = 0) : optionptr(optr) {
		_rep.err_no = 0x01;
	}
	Parser (uint8_t* dptr, Option* optr = 0) : optionptr(optr) {
		if (CheckError (dptr)) {
			_rep.data  = 0;
			_rep.size  = 0;
			_rep.err_no = 0x10;
		}
		else {
			_rep.data  = dptr;
			_rep.size  = GetSize (dptr);
			_rep.err_no = 0;
		}
	}
	virtual ~Parser() {}

	TYP::type typeId () const {
		if (! isLongFrame())
			return TYP::Undefined;

		return (TYP::type)(((APDU<0>*)_rep.data)->asduh.type);
	}

	CtrlField ctrlField() const {
		if (isLongFrame())
			return ((APCI*)_rep.data)->cf;
		else
			return ((Fixedframe*)_rep.data)->cf;
	}

	inline bool isLongFrame() const {
		if (*_rep.data == 0x68)
			return true;
		return false;
	}

	inline bool isError () const { return _rep.err_no; }

	static uint8_t GetSize (const uint8_t* data) {
		if (*data == 0x10) return 5;
		if (*data == 0x68) return data[1] +6;
		return 0;
	}
	static uint8_t GetCheckSum (uint8_t* data) {
		int16_t  len  = 0;
		uint8_t* iter = 0;
		if (*data == 0x10) {
			iter = data + 1; len = 2;
		}
		if (*data == 0x68) {
			iter = data + 4; len = data[1];
		}
		uint8_t sum = 0;
		while (len-- > 0)
			sum += *iter++;
		return sum;
	}
	inline static bool IsCSError (uint8_t* data) {
		return data[GetSize(data) - 2] != GetCheckSum (data);
	}
	static bool CheckError (uint8_t* data) {
		if (*data == 0x10 && data[4] != 0x16)
			return true;
		if (*data == 0x68 && data[1] != data[2] && data[0] != data[3])
			return true;

		return IsCSError (data);
	}
private:
	struct Rep {
		uint8_t* data;
		int16_t  size;
		int16_t  err_no;
	} _rep;
	Option* optionptr;
};

NAMESPACE_ENDED_IEC101

#endif // __IEC_60870_5_101_OBJ_H
