
#ifndef _RT_VALUE_TRANSFER_H_
#define _RT_VALUE_TRANSFER_H_

class RtValueTransfer
{
	RtValueTransfer();
public:
	~RtValueTransfer();

	void Init (const char * address);

	void Start ();
	void Stop  ();

	static RtValueTransfer& Instance();
private:
	struct _RtValueTransferPrivate * _p;
};

#endif // _RT_VALUE_TRANSFER_H_