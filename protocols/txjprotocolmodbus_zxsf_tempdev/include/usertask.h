
#ifndef _USER_TASK_H_
#define _USER_TASK_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string.h>

#pragma pack(push,1)

struct UserTask1
{
	char  objName[32];
	char  taskCmd[32];
	float taskValue;
	long  taskAddr0;
	long  taskAddr1;
	long  taskParamLen;
	unsigned char taskParam[512];
};

#pragma pack(pop)

#endif //_USER_TASK_H_