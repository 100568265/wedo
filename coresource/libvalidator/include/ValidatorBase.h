
#ifndef _VALIDATOR_BASE_H_
#define _VALIDATOR_BASE_H_

#include <string>

class ValidatorBase
{
public:
	ValidatorBase() {}
	virtual ~ValidatorBase() {}
	
	virtual std::string Auth () { return "NotDefined"; }
};

#endif // _VALIDATOR_BASE_H_