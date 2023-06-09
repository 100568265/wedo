#ifndef VARIABLESAVE_H
#define VARIABLESAVE_H

#include "datatype.h"
#include "systhread.h"
#include "rtobjecttree.h"
//#include "dbmysql.h"
#include "sysinifile.h"
#include "rtbase.h"
#include <string>
#include <stdlib.h>
#include <vector>
#include "rtbase.h"
#include "SaveObject.h"

using namespace std;

class VariableStorage
{
public:
    VariableStorage();
    virtual ~VariableStorage();
//    ST_INT ConnectDatabase();
    ST_VOID               AddVariable(ST_DUADDR addr,ST_VARIANT value);
    ST_VOID               Init();
    ST_VOID               Uninit();
    ST_VOID               Start();
    ST_VOID               Stop();
    ST_VOID               Save();
protected:
    vector<SaveObject>    savelist;
private:
    ST_BOOLEAN            run;
    ST_BOOLEAN            init;
    ST_BOOLEAN            exit;
	Thread	              m_Thread;
	static ST_VOID	     *SaveProc(ST_VOID *param);

    class VariableStoragePrivate* _pri;
};

#endif // VARIABLESAVE_H
