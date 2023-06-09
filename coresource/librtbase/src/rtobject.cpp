#include "rtobject.h"
#include "sysstring.h"
#include "sysmalloc.h"

#include <string.h>

NodeBase::NodeBase()
{
    Init();
    m_Nodes = new List<NodeBase>();
}

NodeBase::NodeBase(ST_CHAR *name)
{
    Init();
    m_Nodes = new List<NodeBase>();
    if(name == NULL) return;
    SetName(name);
}

NodeBase::~NodeBase()
{
    Free(m_Name);
}

ST_VOID NodeBase::Init()
{
    m_Name		    = NULL;
    m_parentNode	= NULL;
    m_pTree			= NULL;
}

ST_VOID NodeBase::SetName(ST_CHAR *name)
{
    Free(m_Name);
    // m_Name  = new ST_CHAR[AlignStrlen(name)];
    m_Name = (ST_CHAR*) Malloc (AlignStrlen(name));
    Strcpy(m_Name, name);
}

ST_CHAR *NodeBase::GetName()
{
    return m_Name;
}

ST_VOID NodeBase::AddNode(NodeBase *node)
{
    m_Nodes->Add(node);
}

ST_VOID NodeBase::RemoveNode(NodeBase *node)
{
    m_Nodes->Remove(node);
}

ST_VOID NodeBase::SetParentNode(NodeBase *parentNode)
{
    m_parentNode=parentNode;
}

NodeBase *NodeBase::GetParentNode()
{
    return m_parentNode;
}

ST_VOID NodeBase::SetNodeTree(NodeTree *objTree)
{
    m_pTree = objTree;
}

ST_CHAR *NodeBase::GetFullName(ST_CHAR *fullName)
{
    if(fullName==NULL) return NULL;
    if(m_parentNode) {
        m_parentNode->GetFullName(fullName);
        Strcat(fullName, ".");
    }
    else {
        Strcpy(fullName, "");
    }

    if(m_Name) {
        Strcat(fullName, m_Name);
    }
    return fullName;
}

ST_INT NodeBase::GetFullNameLength()
{
    if(m_parentNode)
    {
        return Strlen(m_Name)+ m_parentNode->GetFullNameLength();
    }
    else
    {
        return Strlen(m_Name);
    }
}

NodeBase *NodeBase::GetChildNode(ST_INT index)
{
    if(index<0 || index>m_Nodes->GetCount()) return NULL;
    return m_Nodes->GetItem(index);
}


ST_INT NodeBase::GetChildNodeCount()
{
    return m_Nodes->GetCount();
}

ST_VOID NodeBase::Clear()
{
    m_Nodes->Clear();
}


///////////////////////////////NAMENODE///////////////////////////////
NameNode::NameNode()
{

}

NameNode::NameNode(ST_CHAR *name)
    :NodeBase(name)
{

}

NameNode::~NameNode()
{

}

ST_INT NameNode::GetNodeType()
{
    return NodeType_Name;
}


ST_INT NameNode::GetChildNameNodeCount()
{
	int count=0;
	for(int i=0;i<m_Nodes->GetCount();i++){
		NodeBase *node=m_Nodes->GetItem(i);
		if(node->GetNodeType()==NodeType_Name){
			count+=1;//linweimingnode->GetChildNameNodeCount();
		}
	}
    return count;
}


ST_INT	NameNode::GetChildValueNodeCount()
{
	int count=0;
	for(int i=0;i<m_Nodes->GetCount();i++){
		NodeBase *node=m_Nodes->GetItem(i);
		if(node->GetNodeType()==NodeType_Name){
			count+=node->GetChildValueNodeCount();
		}
		else if(node->GetNodeType()==NodeType_Value){
			count++;
		}
	}
    return count;
}


List<NodeBase>  *NameNode::GetValueNodes()
{
	m_ValueNodes->Clear();
	for(int i=0;i<m_Nodes->GetCount();i++){
		NodeBase *node=m_Nodes->GetItem(i);
		if(node->GetNodeType()==NodeType_Name){
			List<NodeBase> *subNodes=node->GetValueNodes();
			for(int i=0;i<subNodes->GetCount();i++){
				m_ValueNodes->Add(subNodes->GetItem(i));
			}
		}
		else if(node->GetNodeType()==NodeType_Value){
			m_ValueNodes->Add(node);
		}
	}
	return m_ValueNodes;
}

///////////////////////////////VALUENODE///////////////////////////////
ValueNode::ValueNode()
{
    m_pAddr		    = NULL;
    InitVARIANT(&this->m_value);
    this->m_value.vt = VALType_Int32;
}

ValueNode::ValueNode(ST_CHAR *name)
    :NodeBase(name)
{
    m_pAddr		    = NULL;
    InitVARIANT(&this->m_value);
    this->m_value.vt = VALType_Int32;
}

ValueNode::~ValueNode()
{
     if(m_pAddr != NULL)
    {
        Free(m_pAddr);
        m_pAddr = NULL;
    }
}

ST_INT ValueNode::GetNodeType()
{
    return NodeType_Value;
}

ST_INT ValueNode::GetChildNameNodeCount()
{
	return 0;
}


ST_INT	ValueNode::GetChildValueNodeCount()
{
	return 0;
}

List<NodeBase>  *ValueNode::GetValueNodes()
{
	return NULL;
}

ST_DUADDR * ValueNode::SetAddr(const ST_DUADDR & addr)
{
    Free(m_pAddr);
    m_pAddr = (ST_DUADDR*)Malloc(sizeof(ST_DUADDR));
    m_pAddr->addr    = addr.addr;
    m_pAddr->connect = addr.connect;
    m_pAddr->device  = addr.device;
    m_pAddr->type    = addr.type;
    return m_pAddr;
}

ST_DUADDR *ValueNode::GetAddr()
{
    return m_pAddr;
}

ST_VARIANT ValueNode::GetValue()
{
    ST_VARIANT var=m_value;
    return var;
}

ST_INT ValueNode::SetValue(ST_VARIANT var)
{
    ST_INT res =SD_FAILURE;
    ST_VARIANT *pVar = &var;
    switch(pVar->vt)
    {
    case VALType_Byte:
        res = SetVARIANTValue_int32(&m_value,(ST_BYTE)pVar->bVal);
        break;
    case VALType_Int16:
        res = SetVARIANTValue_int32(&m_value,(ST_INT16)pVar->sVal);
        break;
    case VALType_Int32:
        res = SetVARIANTValue_int32(&m_value,(ST_INT)pVar->iVal);
        break;
    case VALType_UInt16:
        res = SetVARIANTValue_uint32(&m_value,(ST_UINT16)pVar->usVal);
        break;
    case VALType_UInt32:
        res = SetVARIANTValue_uint32(&m_value,(ST_UINT32)pVar->uiVal);
        break;
    case VALType_Float:
        res = SetVARIANTValue_float(&m_value,(ST_FLOAT)pVar->fVal);
        break;
    case VALType_Boolean:
        res = SetVARIANTValue_Boolean(&m_value,(ST_BOOLEAN)pVar->blVal);
        break;
    case VALType_String:
        res = SetVARIANTValue_String(&m_value,pVar->strVal);
        break;
    case VALType_Binary:
        return SD_FAILURE;
        break;
    case VALType_Double:
        res = SetVARIANTValue_double(&m_value,pVar->dVal);
        break;
    case VALType_Decimal:
        res = SetVARIANTValue_Decimal(&m_value,pVar->decVal);
        break;
    case VALType_DateTime:
        res = SetVARIANTValue_DateTime(&m_value,pVar->dtVal);
        break;
    case VALType_Int64:
        res = SetVARIANTValue_int64(&m_value,pVar->lVal);
        break;
    case VALType_UInt64:
        res = SetVARIANTValue_uint64(&m_value,pVar->ulVal);
        break;
    default:
        return SD_FAILURE;
        break;
    }
    return res;
}

ST_VOID ValueNode::SetValueType(ST_INT type)
{
    //if(m_value.vt != type)
    //{
        ClearVARIANT(&m_value);
        m_value.vt = type;
    //}
}

ST_INT ValueNode::GetValueType()
{
    return m_value.vt;
}


///////////////////////////////ADDRTABLE///////////////////////////////
AddrTable::AddrTable(ST_UINT maxNum)
    :HashTable(maxNum)
{
}

ST_ULONG AddrTable::GetHashTableValue(ST_VOID* key)
{
    ST_ULONG hash = 0;
    ST_DUADDR *pAddr = (ST_DUADDR*)key;
    hash = pAddr->device*1000 + pAddr->addr*10000 + pAddr->type*10 + pAddr->connect*100;
    return hash;
}

ST_INT AddrTable::CompareKey(ST_VOID *key1,ST_VOID *key2)
{
    ST_DUADDR *pAddr1 = (ST_DUADDR*)key1;
    ST_DUADDR *pAddr2 = (ST_DUADDR*)key2;
    if(pAddr1->type != pAddr2->type)return pAddr1->type -pAddr2->type;
    if(pAddr1->connect != pAddr2->connect)return pAddr1->connect - pAddr2->connect;
    if(pAddr1->device != pAddr2->device)return pAddr1->device - pAddr2->device;
    return pAddr1->addr - pAddr2->addr;
}

ST_VOID *AddrTable::CloneKey(ST_VOID *key)
{
    ST_DUADDR *pAddr = (ST_DUADDR*)key;
    ST_DUADDR *newAddr = (ST_DUADDR*)Malloc(sizeof(ST_DUADDR));
    *newAddr = *pAddr;
    return newAddr;
}


///////////////////////////////NAMETABLE///////////////////////////////
NameTable::NameTable(ST_UINT maxNum)
    :HashTable(maxNum)
{
}

ST_ULONG NameTable::GetHashTableValue(ST_VOID *key)
{
    ST_ULONG hash = 0;
    ST_ULONG c;
    char *str = (char*)key;
    while (*str != '\0' && str != NULL)
    {
        c = *str++;
        hash = (hash + (c << 4) + (c >> 4)) * 11;
    }
    return hash;
}

ST_INT NameTable::CompareKey(ST_VOID *key1,ST_VOID *key2)
{
    char* str1 = (char*)key1;
    char* str2 = (char*)key2;
    return strcmp(str1,str2);
}

ST_VOID *NameTable::CloneKey(ST_VOID *key)
{
    char *tmpstr = (char*)key;
    char *newstr = (char*)Malloc(AlignStrlen(tmpstr));
    strcpy(newstr, tmpstr);
    return newstr;
}
