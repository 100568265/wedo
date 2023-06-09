
#ifndef _ATSHA_VALIDATOR_H_
#define _ATSHA_VALIDATOR_H_

#include "ValidatorBase.h"

class ATSHAValidator : public ValidatorBase
{
public:
	ATSHAValidator();
	virtual ~ATSHAValidator();

	std::string Auth ();
private:

};

#endif // _ATSHA_VALIDATOR_H_
