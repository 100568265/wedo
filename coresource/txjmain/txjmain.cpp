// txjmain.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "CommManager.h"

int _tmain(int argc, _TCHAR* argv[])
{

	CCommManager *cmmManager;
	cmmManager=new CCommManager();
	if(!cmmManager->ConnectDatabase()==0) return -1;
	if(!cmmManager->LoadChannel()) return -2;
	if(!cmmManager->LoadRTBase()) return -3;
	return 0;
}

