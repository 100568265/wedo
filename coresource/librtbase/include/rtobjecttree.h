#ifndef RTOBJECTTREE_H
#define RTOBJECTTREE_H

#include "sysmutex.h"
#include "rtobject.h"


#ifdef _WIN32
#include <oaidl.h>
#endif

class NodeTree
{
public:
    NodeTree();
    virtual ~NodeTree();

    ST_INT 					GetNameNodeCount (const ST_CHAR * fullName);
	ST_INT 					GetValueNodeCount(const ST_CHAR * fullName);
    ST_INT 					GetNodeName      (const ST_CHAR * fullName, ST_INT index, ST_CHAR * name);

    NodeBase* 				AddNameNode (const ST_CHAR * fullName);
    NodeBase* 				AddNameNode (NodeBase * parentNode, ST_CHAR * nodeName);

    NodeBase* 				AddValueNode(NodeBase * parentNode, ST_CHAR * nodeName, ST_INT valType);
    NodeBase* 				AddValueNode(const ST_CHAR * fullName, ST_INT valType);

    ST_INT 					RemoveNode(NodeBase * node);
    ST_INT 					RemoveNode(const ST_CHAR * fullName);

    NodeBase* 				GetNodeByName(const ST_CHAR * fullName);
    NodeBase* 				GetNodeByAddr(const ST_DUADDR & addr);
	//addr,设备的序号,fullname="设备名"或者"通道名.设备名"
	List<ST_Variable>*		GetVariables (const ST_CHAR * fullName, ST_INT addr);

    ST_INT 					GetNodeValue (const ST_CHAR * fullName, ST_VARIANT &value);
    ST_INT                  SetNodeValue (const ST_CHAR * fullName, const ST_VARIANT &value);
    ST_INT                  SetNodeValue (const ST_CHAR * fullName, ST_Variable * pObj);
    ST_INT 				    GetNodeValueByAddr(const ST_DUADDR & addr, ST_VARIANT &value);
    ST_INT                  SetNodeValueByAddr(const ST_DUADDR & addr, const ST_VARIANT &value);

    ST_INT 					SetNodeAddr  (const ST_CHAR * fullName, const ST_DUADDR &addr);
	ST_INT 					GetNodeAddr  (const ST_CHAR * fullName, ST_DUADDR &addr);

    ST_INT 					GetNodeType  (const ST_CHAR * fullName);

    ST_INT 					GetNodeValueType  (const ST_CHAR * fullName, ST_INT &valType);
    ST_INT 					SetNodeValueType  (const ST_CHAR * fullName, ST_INT  valType);

    ST_VOID					OnNodeValueChanged(const ST_CHAR * fullname, ST_VARIANT value);

    ST_VOID					RegistNodeValueChanged (NodeValueChanged  valueChangedback);
#ifdef _WIN32
    ST_VOID					RegistNodeValueChangedA(NodeValueChangedA valueChangedback);
#endif
    ST_VOID					UnRegistNodeValueChanged();

#ifdef _WIN32
	ST_VARIANT				FromVARIANT(const VARIANT & var);
	VARIANT					ToVARIANT(const ST_VARIANT & vvar);
#endif

protected:
    ST_INT                  ParsePath(const ST_CHAR * fullName, ST_CHAR * parentName, ST_CHAR * nodeName);

    NodeBase*				AddNode(NodeBase * parentNode, NodeBase *node);
    ST_INT					AddNameTable(ST_CHAR * key,   NodeBase *node);
    ST_INT					AddAddrTable(ST_DUADDR * key, NodeBase *node);
    ST_INT					RemoveNameTable(ST_CHAR * key);
    ST_INT					RemoveAddrTable(ST_DUADDR * key);

    NodeValueChanged		NodeValueChanged_Callback;
#ifdef _WIN32
    NodeValueChangedA		NodeValueChanged_CallbackA;
#endif

protected:
    Mutex					m_Mutex;
    ST_CHAR					splitChar;
    List<NodeBase>		   *m_Nodes;
    NameTable				m_NameTable;
    AddrTable				m_AddrTable;
	class SysLogger        *m_pLogger;
	List<ST_Variable>	   *m_pVariables;
};

extern ST_BYTE				g_mode;
extern NodeTree			   *g_pTree;
extern ST_UINT				g_MaxNode;

#endif // RTOBJECTTREE_H
