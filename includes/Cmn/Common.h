
#ifndef __CMN_COMMON_H__
#define __CMN_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CMN_VERSION_NUMBER	20220049UL
#define CMN_VERSION_STR		"2.22.49.0321"
#define CMN_VERSION_CHECK(major, minor, patch) \
		(((major) * 10000000UL) + ((minor) * 10000UL) + (patch))
// Describes the version number in XXYYYZZZZ format such that: is the sub-minor version, is the minor version, and is the major version.
//		(CMN_VERSION_NUMBER % 10000UL)((CMN_VERSION_NUMBER / 10000UL) % 1000UL)(CMN_VERSION_NUMBER / 10000000UL)
// CMN_VERSION_NUMBER is (major * 10000000UL) + (minor * 10000UL) + patch.
// can be used like #if (CMN_VERSION_NUMBER >= CMN_VERSION_CHECK(4, 4, 0))

unsigned long	cmn_libversion_number	(void);
const char *	cmn_libversion_str		(void);
// can be used like \
// assert(cmn_libversion_number() == CMN_VERSION_NUMBER);
// assert(strcmp(cmn_libversion_str(), CMN_VERSION_STR) == 0);


#ifdef __cplusplus
} // extern "C"
#endif

#endif //__CMN_COMMON_H__
