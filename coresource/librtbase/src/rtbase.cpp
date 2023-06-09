#include "rtbase.h"
#include "rtobjecttree.h"


//////////////////////////RTBASE//////////////////////////
ST_BYTE   g_mode  = 1;
NodeTree *g_pTree = NULL;
ST_UINT   g_MaxNode = 100000;

ST_INT Rt_Init()
{
    if(!g_pTree)
    {
        g_pTree = new NodeTree();
		return 0;
    }
	return -1;
}

ST_INT Rt_UnInit()
{
    if(!g_pTree) return 0;
    Rt_UnRegistValueChanged();
    delete g_pTree;
    g_pTree = NULL;
	return 0;
}

ST_INT Rt_GetNameNodeCount(const ST_CHAR * fullName)
{
    if(g_pTree==NULL) return SD_NOTINIT;
    return g_pTree->GetNameNodeCount(fullName);
}

ST_INT Rt_GetNodeName(const ST_CHAR * fullName, ST_INT index, ST_CHAR * name)
{
    if(g_pTree==NULL) return SD_NOTINIT;
    return g_pTree->GetNodeName(fullName, index, name);
}

ST_INT Rt_GetNodeNameByAddr(const ST_DUADDR * addr, ST_CHAR * name)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    NodeBase *pNode = g_pTree->GetNodeByAddr(*addr);
    if( pNode  == NULL) return SD_NOFOUND;
    pNode->GetFullName(name);
    return SD_TRUE;
}

ST_INT Rt_AddNameNode(const ST_CHAR * fullName)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    NodeBase *node = g_pTree->AddNameNode(fullName);
    if(  node  == NULL) return SD_FAILURE;
    return SD_SUCCESS;
}

ST_INT Rt_AddValueNode(const ST_CHAR * fullName, ST_INT type)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    NodeBase *node=g_pTree->AddValueNode(fullName,type);
    if(  node  == NULL) return SD_FAILURE;
    return SD_SUCCESS;
}

ST_INT Rt_SetNodeAddr(const ST_CHAR * fullName, ST_DUADDR   addr)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    return g_pTree->SetNodeAddr(fullName, addr);
}

ST_INT Rt_GetNodeAddr(const ST_CHAR * fullName, ST_DUADDR * addr)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    return g_pTree->GetNodeAddr(fullName, *addr);
}

ST_INT Rt_RemoveNode(const ST_CHAR * fullName)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    return g_pTree->RemoveNode(fullName);
}

//List<ST_Variable> *Rt_GetVariables(ST_CHAR * fullName, ST_INT addr)
//{
//	if(g_pTree == NULL)return NULL;
//    return g_pTree->GetVariables(fullName, addr);
//}

#ifdef _WIN32
ST_INT Rt_GetNodeValueA(const ST_CHAR * fullName, VARIANT * var)
{
    if(g_pTree == NULL) return SD_NOTINIT;
	ST_VARIANT vvar;
    int ret= g_pTree->GetNodeValue(fullName, vvar);
	var=g_pTree->ToVARIANT(vvar);
	return ret;
}

ST_INT Rt_SetNodeValueA(const ST_CHAR * fullName, VARIANT var)
{
    if(g_pTree == NULL) return SD_NOTINIT;
	ST_VARIANT vvar = g_pTree->FromVARIANT(var);
    return g_pTree->SetNodeValue(fullName, vvar);
}

ST_INT Rt_RegistValueChangedA(NodeValueChangedA valueChangedback)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    g_pTree->RegistNodeValueChangedA(valueChangedback);
    return SD_TRUE;
}
#endif

ST_INT Rt_GetNodeValue(const ST_CHAR * fullName, ST_VARIANT * var)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    int ret = g_pTree->GetNodeValue(fullName, *var);
	return ret;
}

ST_INT Rt_GetNodeValueByAddr(const ST_DUADDR * addr, ST_VARIANT * var)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    // NodeBase *pNode = g_pTree->GetNodeByAddr(*addr);
    // if( pNode  == NULL) return SD_NOFOUND;
	return g_pTree->GetNodeValueByAddr(*addr, *var);
}

ST_INT Rt_SetNodeValueByAddr(const ST_DUADDR * addr, ST_VARIANT var)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    // NodeBase *pNode = g_pTree->GetNodeByAddr(*addr);
    // if( pNode  == NULL) return SD_NOFOUND;
	return g_pTree->SetNodeValueByAddr(*addr, var);
}

ST_INT Rt_GetNodeValueType(const ST_CHAR * fullName, ST_INT * valType)
{
    if(g_pTree == NULL) return SD_NOTINIT;
	int ret = g_pTree->GetNodeValueType(fullName, *valType);
	return ret;
}

ST_INT Rt_SetNodeValue(const ST_CHAR * fullName, ST_VARIANT var)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    return g_pTree->SetNodeValue(fullName, var);
}

ST_INT Rt_SetNodeVariable(const ST_CHAR * fullName, ST_Variable * pObj)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    return g_pTree->SetNodeValue(fullName, pObj);
}

ST_INT Rt_RegistValueChanged(NodeValueChanged valueChangedback)
{
    if(g_pTree == NULL) return SD_NOTINIT;
    g_pTree->RegistNodeValueChanged(valueChangedback);
    return SD_TRUE;
}

ST_INT Rt_UnRegistValueChanged()
{
    if(g_pTree == NULL) return SD_NOTINIT;
    g_pTree->UnRegistNodeValueChanged();
    return SD_TRUE;
}


