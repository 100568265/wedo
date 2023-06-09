#include "hashtable.h"
#include "sysmalloc.h"
#include "sysstring.h"

HashTable::HashTable(ST_UINT maxNum)
{
	m_maxNum = maxNum;
	m_items = (struct list_head*)Malloc(sizeof(struct list_head)*maxNum);
	for(ST_UINT i = 0;i < m_maxNum;i++)
	{
		m_items[i].next = m_items[i].prev = &m_items[i];
	}
}

HashTable::~HashTable()
{
	if(m_items)
	{
		Free(m_items);
	}
	m_items = NULL;
}

struct HashNode *HashTable::GetHashNode(ST_VOID *key)
{
	struct HashNode  *res = NULL;
	struct list_head  *head = NULL;
	ST_ULONG lkey = this->GetHashTableValue(key);
	lkey = lkey%m_maxNum;
	head = &m_items[lkey];
	if(list_empty(head))return res;
	list_for_each_entry(struct HashNode,res, head, nodehead)
	{
		if(CompareKey(res->key,key) == 0)
			return res;
	}
	return NULL;
}

ST_VOID *HashTable::GetHashObject(ST_VOID* key)
{
	struct HashNode * res = GetHashNode(key);
	if(!res)return NULL;
	return res->tag;
}

ST_INT HashTable::SetHashObject(ST_VOID *key,ST_VOID *obj)
{
	ST_BOOLEAN bfined = SD_FALSE;
	int res = -1;
	struct HashNode *node = NULL;
	struct list_head *head = NULL;
	ST_ULONG lkey = this->GetHashTableValue(key);
	lkey = lkey%m_maxNum;
	head = &m_items[lkey];
	if(!list_empty(head))
	{
		list_for_each_entry(struct HashNode,node, head, nodehead)
		{
			if(CompareKey(node->key,key) == 0)
			{
				bfined = SD_TRUE;
				break;
			}
		}
	}
	if(bfined)
	{
		res =SD_SUCCESS;
		if(node->tag != obj)
		{
			list_del(&node->nodehead);
			Free(node->key);
			Free(node);
			if(obj != NULL)
			{
				goto newnodeadd;
			}
		}
		return res;

	}
	else if(obj != NULL)
	{
		res =SD_SUCCESS;
		goto newnodeadd;
	}
	else
	{
		res = SD_SUCCESS;
		return res;
	}
newnodeadd:
	struct HashNode* newnode = (struct HashNode*)Malloc(sizeof(struct HashNode));
	newnode->tag = obj;
	newnode->key = CloneKey(key);
	newnode->nodehead.next = newnode->nodehead.prev = NULL;
	list_add(&newnode->nodehead, head);
	return res;
}

ST_BOOLEAN HashTable::ContainsKey(ST_VOID *key)
{
    struct HashNode *res = GetHashNode(key);
    if(res) return SD_TRUE;
    return SD_FALSE;
}



