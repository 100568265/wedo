
#include "Cmn/Common.h"

// IMPLEMENTATION-OF: The cmn_libversion_number() function
// returns an integer equal to CMN_VERSION_NUMBER.
unsigned long	cmn_libversion_number(void)
{
	return CMN_VERSION_NUMBER;
}

const char cmn_version_str[] = CMN_VERSION_STR;

// IMPLEMENTATION-OF: The cmn_libversion_str() function returns
// a pointer to the to the cmn_version_str[] string constant.
const char *    cmn_libversion_str   (void)
{
	return cmn_version_str;
}
