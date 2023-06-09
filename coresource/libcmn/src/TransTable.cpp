#include "TransTable.h"
#include "sysstring.h"

TransferTable::TransferTable()
    :AddrTable(10)
{
    m_pVarAddrs=new List<ST_DUADDR>();
}

TransferTable::TransferTable(ST_UINT maxNum)
    :AddrTable(maxNum)
{
    m_pVarAddrs=new List<ST_DUADDR>();
}
TransferTable::~TransferTable()
{

}
ST_INT TransferTable::AddTRVariable(ST_DUADDR *key,TRVariable *pVar)
{
    if(key){
        if(ContainsKey(key)) return SD_FAILURE;
        m_pVarAddrs->Add(key);
        return SetHashObject(key,pVar);
    }
    else{
        return SD_FAILURE;
    }
}

ST_INT TransferTable::RemoveTRVariable(ST_DUADDR *key)
{
    if(key){
        m_pVarAddrs->Remove(key);
        return SetHashObject(key,NULL);
    }
    else{
        return SD_FAILURE;
    }
}

TRVariable *TransferTable::GetTRVariable(ST_DUADDR *key)
{
    return (TRVariable*)GetHashObject(key);
}


TransferTables::TransferTables()
{
    m_pTables=new List<TransferTable>();
}

TransferTables::~TransferTables()
{

}

ST_BOOLEAN TransferTables::AddTable(TransferTable* table)
{
    if(table!=NULL){
        if(Strlen(table->TableName)>0){
            if(!IsExistTable(table->TableName)){
                m_pTables->Add(table);
            }
            return true;
        }
    }
    return false;
}

TransferTable *TransferTables::GetTable(ST_INT tableid) const
{
    for (int i = 0; i < m_pTables->GetCount(); ++i)
    {
        TransferTable *table = m_pTables->GetItem(i);
        if (table && table->typeId() == tableid)
            return table;
    }
    return NULL;
}

TransferTable *TransferTables::GetTable(const ST_CHAR * tableName) const
{
    if(Strlen(tableName)>0){
       for(int i=0;i<m_pTables->GetCount();i++){
            TransferTable *table=m_pTables->GetItem(i);
            if(table!=NULL){
                if(Strcmp(table->TableName,tableName)==0){
                    return table;
                }
            }
        }
    }
    return NULL;
}

ST_BOOLEAN TransferTables::RemoveTable(const ST_CHAR * tableName)
{
    TransferTable *findTable=NULL;
    for(int i=0;i<m_pTables->GetCount();i++){
        TransferTable *table=m_pTables->GetItem(i);
        if(table!=NULL){
            if(Strcmp(table->TableName,tableName)==0){
                findTable=table;
                break;
            }
        }
    }
    if(findTable!=NULL){
        m_pTables->Remove(findTable);
        return true;
    }
    return false;
}

ST_BOOLEAN TransferTables::IsExistTable(const ST_CHAR * tableName) const
{
    for(int i = 0; i < m_pTables->GetCount(); i++) {
        TransferTable *table = m_pTables->GetItem(i);
        if(table && Strcmp(table->name(), tableName) == 0)
            return true;
    }
    return false;
}

