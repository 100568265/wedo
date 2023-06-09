#ifndef HASHTABLE_H_INCLUDED
#define HASHTABLE_H_INCLUDED

#include "datatype.h"
#include "linkedlist.h"

#ifdef __cplusplus
extern "C" {
#endif

struct HashNode
{
	struct list_head nodehead;
	ST_VOID  *tag;
	ST_VOID  *key;
};

class HashTable
{
public:
	HashTable(ST_UINT maxNum);
	virtual ~HashTable();
	ST_VOID					*GetHashObject(ST_VOID *key);
	ST_INT					 SetHashObject(ST_VOID *key,ST_VOID *obj);
	ST_BOOLEAN				 ContainsKey(ST_VOID *key);
protected:
	virtual ST_ULONG		 GetHashTableValue(ST_VOID *key) = 0;
	virtual ST_INT			 CompareKey(ST_VOID *key1,ST_VOID *key2) = 0;
	virtual ST_VOID		*CloneKey(ST_VOID *key) = 0;
	struct HashNode		*GetHashNode(ST_VOID *key);
private:
	ST_UINT					 m_maxNum;
	struct list_head		*m_items;
};

#ifdef __cplusplus
}
#endif

#endif // HASHTABLE_H_INCLUDED
