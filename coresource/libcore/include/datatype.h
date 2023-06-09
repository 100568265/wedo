#ifndef DATATYPE_H_INCLUDED
#define DATATYPE_H_INCLUDED

#include <stddef.h>

//  class my_class
//  {
//  public:
//      // Default-constructible
//      WEDO_DEFAULTED_CLASS_FUNCTION(my_class(), {})
//      // Copying prohibited
//      WEDO_DELETED_CLASS_FUNCTION(my_class(my_class const&))
//      WEDO_DELETED_CLASS_FUNCTION(my_class& operator= (my_class const&))
//  };
#if __cplusplus > 201100L
#   define WEDO_DEFAULTED_CLASS_FUNCTION(func, body) func = default;
#else
#   define WEDO_DEFAULTED_CLASS_FUNCTION(func, body) func body;
#endif

#if __cplusplus > 201100L
#	define WEDO_DELETED_CLASS_FUNCTION(func) func = delete;
#else
#   define WEDO_DELETED_CLASS_FUNCTION(func) private: func;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_FALSE = 0,
    SD_TRUE  = 1
} Bool;

#define SD_SUCCESS 	   1
#define SD_FAILURE 	   0

#define SD_EXISTED 	  -2
#define SD_NOFOUND 	  -3
#define SD_NODEERR    -4
#define SD_NOTINIT    -5
#define SD_NODEOUT    -6   //over limit

const static union { char c[4]; unsigned long mylong; } endian_test = {{ 'l', '?', '?', 'b' }};
#define ENDIANNESS ((char)endian_test.mylong)
// can be used like if (ENDIANNESS == 'l') {...}

#define SD_BIG_ENDIAN		0
#define SD_LITTLE_ENDIAN	1

#ifndef NULL
    #define NULL (void*(0))
#endif

#ifndef INFINITE
	#define INFINITE        0xFFFFFFFF  // Infinite timeout
#endif


#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32
	#define TXJ_STDCALL __stdcall
#elif __i386__
	#define TXJ_STDCALL __attribute__((__stdcall__))
#else
	#define TXJ_STDCALL 
#endif


#define SD_CONST

#define	SYSTEM_SEL_MSOFT	 0x0001    //micorsoft
#define SYSTEM_SEL_LINUX	 0x0008    //linux

#if !defined(MSOFT)
#define	MSOFT		SYSTEM_SEL_MSOFT
#endif
#if !defined(LINUX)
#define	LINUX		SYSTEM_SEL_LINUX
#endif


/************************************************************************/
/* WINDOWS NT							                                */
/************************************************************************/
#if defined(_WIN32)				                 /* VC++, 32-Bit		*/
#include <WINNT.H>
//#define SD_BYTE_ORDER	SD_LITTLE_ENDIAN
#define SYSTEM_SEL   	SYSTEM_SEL_MSOFT
#define SD_END_STRUCT

/* We can tolerate machine-dependent sizes for these types		*/
typedef wchar_t    ST_WCHAR

#endif  // defined(_WIN32)


/************************************************************************/
/* LINUX							                                */
/************************************************************************/
#if defined(__linux__)

//#define SD_BYTE_ORDER	SD_LITTLE_ENDIAN
#define SD_END_STRUCT

#endif	/* linux */

/************************************************************************/
/* UNIX							                                */
/************************************************************************/
#if defined(__unix__)

//#define SD_BYTE_ORDER	SD_LITTLE_ENDIAN
#define SD_END_STRUCT

#endif	/* unix	*/


#define ST_VOID            void

typedef char          	   ST_CHAR;
typedef signed short 	   ST_SHORT;
typedef signed int   	   ST_INT;
typedef signed long  	   ST_LONG;
typedef unsigned char 	   ST_UCHAR;
typedef unsigned short 	   ST_USHORT;
typedef unsigned int 	   ST_UINT;
typedef unsigned long      ST_ULONG;
#if defined(_MSC_VER) && _MSC_VER < 1300
typedef ULONGLONG		   ST_ULONGLONG;
#else
typedef unsigned long long ST_ULONGLONG;
#endif
typedef double 			   ST_DOUBLE;
typedef float 			   ST_FLOAT;

/* General purpose return code						    */
typedef signed int 		   ST_RET;

#if __cplusplus < 201100L
/* We need specific sizes for these types				*/
typedef signed char 	   ST_INT8;
typedef signed short 	   ST_INT16;
typedef signed long 	   ST_INT32;
#if defined(_MSC_VER) && _MSC_VER < 1300
typedef LONGLONG           ST_INT64;
#else
typedef signed long long   ST_INT64;
#endif
typedef unsigned char 	   ST_UINT8;
typedef unsigned short 	   ST_UINT16;
typedef unsigned long 	   ST_UINT32;
#if defined(_MSC_VER) && _MSC_VER < 1300
typedef ULONGLONG		   ST_UINT64;
#else
typedef unsigned long long ST_UINT64;
#endif
typedef bool 			   ST_BOOLEAN;
typedef unsigned char 	   ST_BYTE;

#else // __cplusplus < 201100L

#include <stdint.h>
typedef int8_t	           ST_INT8;
typedef int16_t 	       ST_INT16;
typedef int32_t 	       ST_INT32;
#if defined(_MSC_VER) && _MSC_VER < 1300
typedef LONGLONG           ST_INT64;
#else
typedef int64_t            ST_INT64;
#endif
typedef uint8_t 	       ST_UINT8;
typedef uint16_t 	       ST_UINT16;
typedef uint32_t 	       ST_UINT32;
#if defined(_MSC_VER) && _MSC_VER < 1300
typedef ULONGLONG		   ST_UINT64;
#else
typedef uint64_t           ST_UINT64;
#endif
typedef bool 			   ST_BOOLEAN;
typedef uint8_t 	       ST_BYTE;
#endif // __cplusplus < 201100L


typedef struct _tagDECIMAL {
    ST_UINT16 wReserved;
    union {
        struct {
            ST_UINT8 scale;
            ST_UINT8 sign;
        };
        ST_UINT16 signscale;
    };
    ST_UINT32 hi32;
    union {
        struct {
            ST_UINT32 lo32;
            ST_UINT32 mi32;
        };
        ST_DOUBLE lo64;
    };
} ST_DECIMAL;

typedef struct _tagVARIANT
{
	union
    {
		struct
		{
			ST_UINT16 vt;
			ST_UINT16 wReserved1;
            ST_UINT16 wReserved2;
            ST_UINT16 wReserved3;
			union
			{
				ST_INT64 lVal;
                ST_INT32 iVal;
				ST_INT16 sVal;
				ST_FLOAT fVal;
				ST_DOUBLE dVal;
				ST_DOUBLE date;
				ST_BYTE  bVal;
				ST_BOOLEAN blVal;
				ST_DOUBLE dtVal;
				ST_CHAR cVal;
				ST_CHAR *strVal;
				ST_UINT64 ulVal;
				ST_UINT32 uiVal;
				ST_UINT16 usVal;
			};
		};
		ST_DECIMAL decVal;
	};
} ST_VARIANT;

typedef struct _tagDUAddr
{
    ST_INT type;
    ST_INT connect;
    ST_INT device;
    ST_INT addr;
} ST_DUADDR;

enum NodeType
{
    NodeType_Name  = 0,
    NodeType_Value = 1
};

enum VariableType
{
	VALType_SByte    = 0x0,
	VALType_Int16    = 0x1,
	VALType_Int32    = 0x2,
	VALType_Byte     = 0x3,
	VALType_UInt16   = 0x4,
	VALType_UInt32   = 0x5,
	VALType_Float    = 0x6,
	VALType_Boolean  = 0x7,
	VALType_String   = 0x8,
	VALType_Binary   = 0x9,
	VALType_Double   = 0xA,
	VALType_Decimal  = 0xB,
	VALType_DateTime = 0xC,
	VALType_Int64    = 0xD,
	VALType_UInt64   = 0xE,
    VALType_Char     = 0xF
};


extern ST_VOID InitVARIANT  (ST_VARIANT *pVar);
extern ST_VOID ClearVARIANT (ST_VARIANT *pVar);
extern ST_VOID CopyVARIANT  (ST_VARIANT *pDstVar, ST_VARIANT *pSrcVar);

extern ST_INT  SetVARIANTValue_int32    (ST_VARIANT *pVar, ST_INT32   val);
extern ST_INT  SetVARIANTValue_int64    (ST_VARIANT *pVar, ST_INT64   val);
extern ST_INT  SetVARIANTValue_uint32   (ST_VARIANT *pVar, ST_UINT32  val);
extern ST_INT  SetVARIANTValue_uint64   (ST_VARIANT *pVar, ST_UINT64  val);
extern ST_INT  SetVARIANTValue_float    (ST_VARIANT *pVar, ST_FLOAT   val);
extern ST_INT  SetVARIANTValue_double   (ST_VARIANT *pVar, ST_DOUBLE  val);
extern ST_INT  SetVARIANTValue_String   (ST_VARIANT *pVar, ST_CHAR*  pVal);
extern ST_INT  SetVARIANTValue_DateTime (ST_VARIANT *pVar, ST_DOUBLE  val);
extern ST_INT  SetVARIANTValue_Boolean  (ST_VARIANT *pVar, ST_BOOLEAN val);
extern ST_INT  SetVARIANTValue_Decimal  (ST_VARIANT *pVar, ST_DECIMAL val);



extern const ST_UINT32 TXJ_VERSION ;
extern const char * TXJ_VERSION_STR;
#define TXJ_VERSION_CHECK(major, minor, patch) (((major)<<24)|((minor)<<16)|(patch))
// TXJ_VERSION is (major << 24) + (minor << 16) + patch.
// can be used like #if (TXJ_VERSION >= TXJ_VERSION_CHECK(4, 4, 0))

#ifdef __cplusplus
}
#endif

#endif // DATATYPE_H_INCLUDED
