#ifndef RTOBJECT_H
#define RTOBJECT_H

#include "datatype.h"
#include "sysgenlist.h"
#include "hashtable.h"

#ifdef _WIN32
#include <oaidl.h>
#endif

#define VARTYPE_NULL 0xffff

#ifdef _WIN32
typedef ST_VOID (TXJ_STDCALL *NodeValueChangedA)(const ST_CHAR* fullname,VARIANT var);
#endif

typedef ST_VOID (TXJ_STDCALL *NodeValueChanged)(const ST_CHAR* fullname,ST_VARIANT &var);


class  AddrTable:public HashTable
{
public:
    AddrTable(ST_UINT maxNum);
protected:
    ST_ULONG GetHashTableValue(ST_VOID *key);
    ST_INT CompareKey(ST_VOID *key1,ST_VOID *key2);
    ST_VOID *CloneKey(ST_VOID *key);
};

class NameTable:public HashTable
{
public:
    NameTable(ST_UINT maxNum);
protected:
    ST_ULONG GetHashTableValue(ST_VOID *key);
    ST_INT   CompareKey(ST_VOID *key1,ST_VOID *key2);
    ST_VOID  *CloneKey(ST_VOID *key);
};


class NodeTree;

class NodeBase
{
public:
    NodeBase();
    NodeBase(ST_CHAR *name);
    virtual ~NodeBase();
public:
    ST_VOID				Init();
    ST_VOID				SetName(ST_CHAR *name);
    ST_CHAR				*GetName();
    ST_VOID				AddNode(NodeBase *node);
    ST_CHAR				*GetFullName(ST_CHAR *fullName);
    ST_INT				GetFullNameLength();
    ST_VOID				SetParentNode(NodeBase *parentNode);
    ST_VOID				SetNodeTree(NodeTree *objTree);
    NodeBase			*GetParentNode();
	NodeBase			*GetChildNode(ST_INT index);
    ST_INT				GetChildNodeCount();
    virtual ST_INT		GetChildNameNodeCount()=0;
    virtual ST_INT		GetChildValueNodeCount()=0;
    ST_VOID				RemoveNode(NodeBase *node);
    ST_VOID				Clear();
    virtual ST_INT		GetNodeType() = 0;
	virtual List<NodeBase>    *GetValueNodes()=0;
protected:
    List<NodeBase>		*m_Nodes;   //linweiming
    NodeBase			*m_parentNode;
    NodeTree			*m_pTree;
    ST_CHAR				*m_Name;
	List<NodeBase>		*m_ValueNodes; //linweiming
};

class NameNode:public NodeBase
{
public:
    NameNode();
    NameNode(ST_CHAR *name);
    ~NameNode();
    ST_INT			GetNodeType();
	ST_INT			GetChildNameNodeCount();
    ST_INT			GetChildValueNodeCount();
	List<NodeBase>   *GetValueNodes();
};
/*
typedef struct Attribute
{
	union
	{
		ST_INT attribute;
		struct _Attribute
		{
			ST_BYTE bsave:1;			//是否存盘
			ST_BYTE bsoundwarning:1;	//是否声音报警
			ST_BYTE bwaningprint:1;	    //越线报警是否打印
			ST_BYTE bmanualvalue:1;	    //手动值，采样数据不会更新
			ST_BYTE bqf:1;				//是否取反
			ST_BYTE breserver0:1;
			ST_BYTE hlcheck:1;			//合理值检查
			ST_BYTE changeclue:1;		//数据改变提示，比如遥信变位闪烁或者其他的
			ST_BYTE needsample:1;		//需要经过采样该点的值才是有效的，和avalied搭配使用
		}tagAttribute;
	};
}ST_Attribute;

typedef struct State
{
	union
	{
		ST_INT state;
		struct _tagState
		{
			ST_BYTE changeclue:1;
			ST_BYTE avalied:1;		//点是否有效（数据已经经过更新了needsample搭配使用）
			ST_BYTE overlimit:2;	//0:无越线 1：下限 2：上限
			ST_BYTE overlimitlevel:4; //越限等级
		}tagState;
	};
}ST_State;
*/

typedef struct  VariableObjectInfo
{
	ST_INT id;
	ST_CHAR name[64];
	ST_INT type;
	ST_INT overlimit;
	ST_INT overlimitstate;
	ST_INT datalen;
	ST_INT decimal;
	ST_INT storagedistance;
	ST_FLOAT zero;
	ST_FLOAT offset;
	ST_FLOAT maxvaliedvalue;
	ST_FLOAT minvaliedvalue;
	ST_FLOAT prevalue;
	ST_INT locked;
	ST_INT fromtype;
	ST_INT fromobject;		// source device  id
	ST_INT fromItemaddr; 	// indexes; 已定义设备中的设备点序号或者说ID
	ST_INT fromconnect;		// source channel id
	ST_INT area;
	ST_FLOAT coefficient;
	/*
	union
	{
		ST_INT access;
		struct tagAccess
		{
			ST_BYTE read:1;
			ST_BYTE write:1;
		};
	};*/
	//ST_State state;
	//ST_State userstate;
	//ST_Attribute attribute;
	ST_UINT32 wReserved1;
	ST_UINT32 wReserved2;
	ST_UINT32 wReserved3;
	ST_INT filtersavetype;
	ST_FLOAT filtersaveminvalue;
	ST_FLOAT filtersavemaxvalue;
	ST_CHAR filtersavevariable[64];
	ST_CHAR eventtype[12];
	ST_INT apptype;
}ST_Variable;


class ValueNode:public NodeBase
{
public:
    ValueNode();
    ValueNode(ST_CHAR *name);
    ~ValueNode();
    ST_INT			 GetNodeType();
    ST_DUADDR		*SetAddr(const ST_DUADDR & addr);
    ST_DUADDR		*GetAddr();
    ST_INT			 SetValue(ST_VARIANT var);
    ST_VARIANT		 GetValue();
    ST_VOID			 SetValueType(ST_INT type);
    ST_INT			 GetValueType();
	ST_INT			 GetChildNameNodeCount();
    ST_INT			 GetChildValueNodeCount();
	List<NodeBase>   *GetValueNodes();

private:
    ST_VARIANT		m_value;
    ST_DUADDR	   *m_pAddr;
public:
	ST_Variable     *m_pVarObject;
};


#endif // RTOBJECT_H
