#ifndef TRANSTABLE_H
#define TRANSTABLE_H

#include "datatype.h"
#include "rtobject.h"

typedef struct _tagTRVariable
{
    ST_DUADDR Addr;
    ST_FLOAT Coefficient;
    ST_FLOAT PreValue;
    ST_INT ItemType;
    ST_INT Modify;
}TRVariable;

class TransferTable : public AddrTable
{
public:
    TransferTable();
    TransferTable(ST_UINT maxNum);
    virtual ~TransferTable();
    ST_INT AddTRVariable      (ST_DUADDR *key, TRVariable *pVar);
    ST_INT RemoveTRVariable   (ST_DUADDR *key);
    TRVariable *GetTRVariable (ST_DUADDR *key);

    inline ST_INT    typeId () const { return TableType; }
    inline const char* name () const { return TableName; }
public:
    ST_CHAR TableName[64];
    List<ST_DUADDR> *m_pVarAddrs;

    ST_INT  TableType;
};

class TransferTables
{
public:
    TransferTables();
    virtual ~TransferTables();
    TransferTable *GetTable(ST_INT tableid) const;
    TransferTable *GetTable(const ST_CHAR * tableName) const;

    ST_BOOLEAN AddTable    (TransferTable * table);
    ST_BOOLEAN RemoveTable (const ST_CHAR * tableName);
    ST_BOOLEAN IsExistTable(const ST_CHAR * tableName) const;
public:
    List<TransferTable> *m_pTables;
};

#endif // TRANSTABLE_H
