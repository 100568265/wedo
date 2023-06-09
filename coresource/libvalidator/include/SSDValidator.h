
#ifndef _SSD_VALIDATOR_H_
#define _SSD_VALIDATOR_H_

#include "ValidatorBase.h"

class SSDValidator : public ValidatorBase
{
public:
	SSDValidator();
	virtual ~SSDValidator();

	std::string Auth ();
private:

};


#endif // _SSD_VALIDATOR_H_
