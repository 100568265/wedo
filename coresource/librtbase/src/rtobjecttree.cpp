#include "rtobjecttree.h"
#include "sysstring.h"
#include "syslogger.h"
#include <math.h>

NodeTree::NodeTree() : m_NameTable(g_MaxNode), m_AddrTable(g_MaxNode)
{
    splitChar = '.';
    NodeValueChanged_Callback  = NULL;
#ifdef _WIN32
    NodeValueChanged_CallbackA = NULL;
#endif
	m_pLogger = SysLogger::GetInstance();
	m_Nodes   = new List<NodeBase>();
	m_pVariables = new List<ST_Variable>();
}

NodeTree::~NodeTree()
{

}

ST_INT NodeTree::GetNameNodeCount (const ST_CHAR * fullName)
{
    if(fullName == NULL || *fullName == '\0') {
		int count = this->m_Nodes->GetCount();
		for(int i = 0; i < m_Nodes->GetCount(); i++) {
			count += 1;//linweiming  m_Nodes->GetItem(i)->GetChildNameNodeCount();
		}
        return count;
    }
    else {
        NodeBase *node = GetNodeByName(fullName);
        if(node == NULL) return 0;
		return node->GetChildNameNodeCount();
    }
}

ST_INT NodeTree::GetValueNodeCount(const ST_CHAR * fullName)
{
    if(fullName == NULL || *fullName == '\0') {
        return 0;
    }
    else {
        NodeBase *node = GetNodeByName(fullName);
        if(node == NULL) return 0;
		return node->GetChildValueNodeCount();
    }
}

ST_INT NodeTree::GetNodeName(const ST_CHAR * fullName, ST_INT index, ST_CHAR * name)
{
    if(fullName == NULL || *fullName == '\0') {
        if(index < 0 || index > m_Nodes->GetCount()) {
            return SD_NODEOUT;
        }
        else {
			NodeBase *node = m_Nodes->GetItem(index);
			if(node == NULL) return SD_NODEOUT;
            Strcpy(name, m_Nodes->GetItem(index)->GetName());
			return node->GetNodeType();
        }
    }
    else {
        NodeBase *parentNode = GetNodeByName(fullName);
        if(parentNode == NULL) return SD_NOFOUND;
        if(index < 0 || index > parentNode->GetChildNodeCount()) return SD_NODEOUT;
		NodeBase *node = parentNode->GetChildNode(index);
		if(node == NULL) return SD_NOFOUND;
        Strcpy(name, node->GetName());
		return node->GetNodeType();
    }
}

NodeBase *NodeTree::GetNodeByName(const ST_CHAR * fullName)
{
    return (NodeBase*)m_NameTable.GetHashObject((void*)fullName);
}

NodeBase *NodeTree::GetNodeByAddr(const ST_DUADDR & addr)
{
    return (NodeBase*)m_AddrTable.GetHashObject((void*)&addr);
}

List<ST_Variable> *NodeTree::GetVariables(const ST_CHAR * fullName, ST_INT addr)
{
	m_pVariables->Clear();
	if(fullName == NULL || *fullName == '\0') {
		for(int i = 0; i < m_Nodes->GetCount(); i++) {
			List<NodeBase> *nodes = m_Nodes->GetItem(i)->GetValueNodes();
			for(int j=0 ; j<nodes->GetCount(); j++) {
				ValueNode *valueNode = dynamic_cast<ValueNode*>(nodes->GetItem(j));
				if(valueNode == NULL) continue;
				if(valueNode->m_pVarObject == NULL) continue;
				if(addr != -1) {
					if(valueNode->GetAddr()->addr == addr) {
						m_pVariables->Add(valueNode->m_pVarObject);
						return m_pVariables;
					}
				}
				else {
					m_pVariables->Add(valueNode->m_pVarObject);
				}
			}
		}
	}
	else {
		NodeBase* nameNode = GetNodeByName(fullName);
		if(nameNode == NULL) return m_pVariables;
		List<NodeBase> *nodes = nameNode->GetValueNodes();
		if(nodes->GetCount() == 0) return m_pVariables;
		for(int i = 0; i < nodes->GetCount(); i++) {
			ValueNode *valueNode = dynamic_cast<ValueNode*>(nodes->GetItem(i));
			if(valueNode == NULL) continue;
			if(valueNode->m_pVarObject == NULL) continue;
			if(addr != -1) {
				if(valueNode->GetAddr()->addr == addr) {
					m_pVariables->Add(valueNode->m_pVarObject);
					return m_pVariables;
				}
			}
			else {
				m_pVariables->Add(valueNode->m_pVarObject);
			}
		}
	}
	return m_pVariables;
}

ST_INT NodeTree::AddNameTable(ST_CHAR *key, NodeBase *node)
{
    if(key) {
        if(m_NameTable.ContainsKey(key)) return SD_FAILURE;
        return m_NameTable.SetHashObject(key, node);
    }
    else {
        return SD_FAILURE;
    }
}

ST_INT NodeTree::RemoveNameTable(ST_CHAR *key)
{
    if(key) {
        return m_NameTable.SetHashObject(key, NULL);
    }
    else {
        return SD_FAILURE;
    }
}

ST_INT NodeTree::AddAddrTable(ST_DUADDR *key, NodeBase *node)
{
    if(key) {
        if(m_AddrTable.ContainsKey(key)) return SD_FAILURE;
        return m_AddrTable.SetHashObject(key, node);
    }
    else {
        return SD_FAILURE;
    }
}

ST_INT NodeTree::RemoveAddrTable(ST_DUADDR *key)
{
    if(key) {
        return m_AddrTable.SetHashObject(key, NULL);
    }
    else {
        return SD_FAILURE;
    }
}

ST_INT NodeTree::GetNodeValue(const ST_CHAR *fullName, ST_VARIANT &value)
{
    NodeBase *node = GetNodeByName(fullName);
    if(node == NULL) return SD_NOFOUND;
    if(node->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode *valueNode = dynamic_cast<ValueNode*>(node);
	if(valueNode == NULL) return SD_FALSE;
    value = valueNode->GetValue();
    return SD_TRUE;
}

ST_INT NodeTree::GetNodeValueByAddr(const ST_DUADDR & addr, ST_VARIANT & value)
{
    NodeBase * node = GetNodeByAddr(addr);
    if(node == NULL) return SD_NOFOUND;
    if(node->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode *valueNode = dynamic_cast<ValueNode*>(node);
	if(valueNode == NULL) return SD_FALSE;
    value = valueNode->GetValue();
    return SD_TRUE;
}

ST_INT NodeTree::SetNodeValue(const ST_CHAR * fullName, const ST_VARIANT & value)
{
    NodeBase *node = GetNodeByName(fullName);
    if (node == NULL) return SD_NOFOUND;
    ST_INT ret = SD_FAILURE;
    if (node->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode *valueNode = dynamic_cast<ValueNode*>(node);
	if (valueNode == NULL) return SD_FALSE;
	try {
		ret = valueNode->SetValue(value);
	}
	catch (...) {
		m_pLogger->LogWarn ("Exception [ValueNode::SetValue]");
	}
    if (ret == SD_SUCCESS) {
		try {
			OnNodeValueChanged(fullName, value);
		}
        catch (std::exception& e) { m_pLogger->LogWarn (e.what()); }
		catch (...) {
			m_pLogger->LogWarn ("Exception [OnNodeValueChanged : %s]", fullName);
		}
    }
    return ret;
}

ST_INT NodeTree::SetNodeValue(const ST_CHAR * fullName, ST_Variable * pObj)
{
    NodeBase *node = GetNodeByName(fullName);
    if(node == NULL) return SD_NOFOUND;
    ST_INT ret = SD_FAILURE;
    if(node->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode * valueNode = dynamic_cast<ValueNode*>(node);
	if(valueNode == NULL) return SD_FALSE;
	valueNode->m_pVarObject = pObj;
    return SD_SUCCESS;
}

ST_INT NodeTree::SetNodeValueByAddr(const ST_DUADDR & addr, const ST_VARIANT & value)
{
    NodeBase *node = GetNodeByAddr(addr);
    if(node == NULL) return SD_NOFOUND;
    ST_INT ret = SD_FAILURE;
    if(node->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode *valueNode = dynamic_cast<ValueNode*>(node);
	if(valueNode == NULL) return SD_FALSE;
	try {
		ret = valueNode->SetValue(value);
	}
	catch(...) {
		m_pLogger->LogWarn ("Exception [ValueNode::SetValue]");
	}
    if(ret == SD_SUCCESS) {
		try {
		    ST_CHAR fullName[256] = {0};
			OnNodeValueChanged(node->GetFullName(fullName), value);
		}
        catch (std::exception& e) { m_pLogger->LogWarn (e.what()); }
		catch(...) {}
    }
    return ret;
}

ST_INT NodeTree::GetNodeType(const ST_CHAR * fullName)
{
    NodeBase *node = GetNodeByName(fullName);
    if(node == NULL) return SD_NOFOUND;
    return node->GetNodeType();
}

ST_INT NodeTree::GetNodeValueType(const ST_CHAR * fullName, ST_INT &valType)
{
    NodeBase *node = GetNodeByName(fullName);
    if(node == NULL) return SD_NOFOUND;
    if(node->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode *valueNode = dynamic_cast<ValueNode*>(node);
	if(valueNode == NULL) return SD_FALSE;
    valType = valueNode->GetValueType();
    return SD_TRUE;
}

ST_INT NodeTree::SetNodeValueType(const ST_CHAR *fullName, ST_INT valType)
{
    NodeBase *node = GetNodeByName(fullName);
    if(node == NULL) return SD_NOFOUND;
    if(node->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode *valueNode = dynamic_cast<ValueNode*>(node);
	if(valueNode == NULL) return SD_FALSE;
    valueNode->SetValueType(valType);
    return SD_TRUE;
}

NodeBase* NodeTree::AddNode(NodeBase *parentNode, NodeBase *node)
{
    NodeBase* result = NULL;
    if(node == NULL) return result;
    if(node->GetName() == NULL) return result;
    m_Mutex.Lock();
    node->SetParentNode(parentNode);
    node->SetNodeTree(this);
#ifdef _WIN32
	ST_CHAR fullName[256] = {0};
#else
    ST_INT fullNameLen = node->GetFullNameLength() * 2;
    ST_CHAR fullName[fullNameLen];
#endif
    node->GetFullName(fullName);
    if(AddNameTable(fullName, node))
    {
        if(parentNode) {
            parentNode->AddNode(node);
        }
        else {
            m_Nodes->Add(node);
        }
        result = node;
    }
    m_Mutex.UnLock();
    return result;
}

NodeBase* NodeTree::AddNameNode(NodeBase *parentNode, ST_CHAR *name)
{
    if (parentNode && parentNode->GetNodeType() != NodeType_Name)
        return NULL;

    ST_INT strLen = Strlen(name);
    if (parentNode != NULL) {
        strLen += parentNode->GetFullNameLength();
    }
    strLen *= 2;

#ifdef _WIN32
	ST_CHAR fullName[256] = {0};
#else
    ST_CHAR fullName[strLen];
#endif

    Strcpy (fullName, "");
    if (parentNode != NULL) {
        parentNode->GetFullName(fullName);
        Strncat(fullName, ".", strLen);
    }
    Strncat(fullName, name, strLen);
    NodeBase *pNode = GetNodeByName(fullName);
    if(pNode) return pNode;
    NameNode *pobj = new NameNode(name);
    if (AddNode(parentNode, pobj)) {
        return pobj;
    }
    else {
        delete pobj;
		pobj = NULL;
        return NULL;
    }
}

NodeBase* NodeTree::AddNameNode(const ST_CHAR * fullName)
{
    if(fullName == NULL) return NULL;
    ST_INT fullNameLen = AlignStrlen(fullName);
#ifdef _WIN32
	ST_CHAR name[256] = {0};
    ST_CHAR parentFullName[256] = {0};
#else
    ST_CHAR name[fullNameLen];
    ST_CHAR parentFullName[fullNameLen];
#endif
    ParsePath(fullName, parentFullName, name);
    if(*parentFullName == '\0')
    {
        return AddNameNode(NULL, name);
    }
    else
    {
        NodeBase* pNode = NULL;
        pNode = GetNodeByName(parentFullName);
		if(pNode == NULL) {
			pNode = AddNameNode(parentFullName);
		}
        return AddNameNode(pNode, name);
    }
}

NodeBase* NodeTree::AddValueNode(NodeBase *parentNode, ST_CHAR *name, ST_INT valType)
{
    if(parentNode == NULL) return NULL;
    if(parentNode->GetNodeType() != NodeType_Name) return NULL;
    ST_INT fullNameLen = (parentNode->GetFullNameLength() + Strlen(name)) * 2;

#ifdef _WIN32
	ST_CHAR fullName[256] = {0};
#else
    ST_CHAR fullName[fullNameLen];
#endif

    Strcpy(fullName, "");
    parentNode->GetFullName(fullName);
    if(Strlen(fullName) == 0) return NULL;
    Strncat(fullName,  ".", fullNameLen);
    Strncat(fullName, name, fullNameLen);
    NodeBase *pNode = GetNodeByName(fullName);
    if(pNode != NULL) {
        ValueNode *valueNode = dynamic_cast<ValueNode*>(pNode);
		if(valueNode == NULL) return NULL;
        valueNode->SetValueType(valType);
        return valueNode;
    }
    else{
        ValueNode *pObj = new ValueNode(name);
        pObj->SetValueType(valType);
        if(AddNode(parentNode, pObj)) {
            return pObj;
        }
        else {
            delete pObj;
			pObj = NULL;
            return NULL;
        }
    }
}

NodeBase* NodeTree::AddValueNode(const ST_CHAR * fullName, ST_INT valType)
{
    if(fullName == NULL) return NULL;
    ST_INT fullNameLen = AlignStrlen(fullName);
#ifdef _WIN32
	ST_CHAR name[256] = {0};
    ST_CHAR parentFullName[256] = {0};
#else
    ST_CHAR name[fullNameLen];
    ST_CHAR parentFullName[fullNameLen];
#endif
    ParsePath(fullName, parentFullName, name);
    if(*name == '\0') return NULL;
    NodeBase* pNode = AddNameNode(parentFullName);
    if(pNode == NULL) return NULL;
    return AddValueNode(pNode, name, valType);
}

ST_INT NodeTree::SetNodeAddr(const ST_CHAR * fullName, const ST_DUADDR & addr)
{
    NodeBase* pNode = GetNodeByName(fullName);
    if(!pNode) return SD_NOFOUND;
    if(pNode->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode * valueNode = dynamic_cast<ValueNode*>(pNode);
    if(valueNode == NULL) return SD_NOFOUND;
    ST_DUADDR * pAddr = valueNode->SetAddr(addr);
    return AddAddrTable(pAddr, valueNode);
}

ST_INT NodeTree::GetNodeAddr(const ST_CHAR * fullName, ST_DUADDR &addr)
{
    NodeBase* pNode = GetNodeByName(fullName);
    if(!pNode) return SD_NOFOUND;
    if(pNode->GetNodeType() != NodeType_Value) return SD_NODEERR;
    ValueNode *valueNode = dynamic_cast<ValueNode*>(pNode);
    if(valueNode == NULL) return SD_NOFOUND;
	addr = *(valueNode->GetAddr());
    return SD_SUCCESS;
}

ST_INT	NodeTree::RemoveNode(NodeBase* node)
{
    ST_INT nodeCount = 0;
    if(node == NULL)
    {
        nodeCount = this->m_Nodes->GetCount();
        for(int i = 0; i < nodeCount; i++)
        {
            NodeBase *subNode = this->m_Nodes->GetItem(i);
            RemoveNode(subNode);
        }
        this->m_Nodes->Clear();
        return SD_SUCCESS;
    }
    else
    {
        nodeCount = node->GetChildNodeCount();
        for(int i = 0; i < nodeCount; i++)
        {
            NodeBase *subNode = node->GetChildNode(i);
            RemoveNode(subNode);
        }
        ST_UINT fullNameLen = node->GetFullNameLength() * 2;
#ifdef _WIN32
	ST_CHAR fullName[256] = {0};
#else
	ST_CHAR fullName[fullNameLen];
#endif

        node->GetFullName(fullName);
        RemoveNameTable(fullName);
        if(node->GetNodeType() == NodeType_Value)
        {
            ValueNode *valueNode = dynamic_cast<ValueNode*>(node);
			if(valueNode == NULL) return SD_FAILURE;
            RemoveAddrTable(valueNode->GetAddr());
        }
        node->Clear();
        return SD_SUCCESS;
    }
}

ST_INT	NodeTree::RemoveNode(const ST_CHAR * fullName)
{
    NodeBase* node = NULL;
    if(fullName != NULL && *fullName != '\0')
    {
        node = GetNodeByName(fullName);
    }
    return RemoveNode(node);
}

ST_INT NodeTree::ParsePath(const ST_CHAR * fullName, ST_CHAR *parentFullName, ST_CHAR *name)
{
    if((fullName == NULL) || (parentFullName == NULL) || (name == NULL)) return -1;
    int lastIndex = -1;
    Strcpy(parentFullName, fullName);
    Strtrim(parentFullName, splitChar);
    lastIndex = StrLastIndexOf(parentFullName, splitChar);
    if(lastIndex > 0)
    {
        // Memcpy(parentFullName,parentFullName,lastIndex);
        parentFullName[lastIndex] = '\0';
    }
    if(fullName[lastIndex + 1] != '\0')
    {
       //linweiming Strcpy(name,parentFullName + lastIndex + 1);
        Strcpy(name, fullName + lastIndex + 1);
    }
    if(lastIndex > 0)
    {
        Strtrim(parentFullName, splitChar);
    }
    else
    {
        Strcpy(parentFullName, "");
    }
    return lastIndex;
}

ST_VOID NodeTree::OnNodeValueChanged(const ST_CHAR *fullname, ST_VARIANT value)
{
    if (NodeValueChanged_Callback) {
        NodeValueChanged_Callback(fullname, value);
    }
#ifdef _WIN32
    if (NodeValueChanged_CallbackA) {
        VARIANT var = ToVARIANT(value);
        NodeValueChanged_CallbackA(fullname, var);
    }
#endif
}

ST_VOID NodeTree::RegistNodeValueChanged(NodeValueChanged valueChangedback)
{
    NodeValueChanged_Callback = valueChangedback;
}

#ifdef _WIN32
ST_VOID NodeTree::RegistNodeValueChangedA(NodeValueChangedA valueChangedback)
{
    NodeValueChanged_CallbackA = valueChangedback;
}
#endif


ST_VOID NodeTree::UnRegistNodeValueChanged()
{
    if (NodeValueChanged_Callback) {
        NodeValueChanged_Callback = NULL;
    }
#ifdef _WIN32
    if (NodeValueChanged_CallbackA) {
        NodeValueChanged_CallbackA = NULL;
    }
#endif
}

#ifdef _WIN32
VARIANT NodeTree::ToVARIANT(const ST_VARIANT & vvar)
{
	VARIANT var;
	var.vt = VT_NULL;
	if(vvar.vt != VARTYPE_NULL)
	{
		switch(vvar.vt)
		{
		case VALType_Byte:
			var.vt = VT_I1;
			var.bVal = vvar.bVal;
			break;
		case VALType_Int16:
			var.vt = VT_I2;
			var.iVal = vvar.sVal;
			break;
		case VALType_Int32:
			var.vt = VT_I4;
			var.intVal = vvar.iVal;
			break;
		case VALType_UInt16:
			var.vt = VT_UI2;
			var.uiVal = vvar.usVal;
			break;
		case VALType_UInt32:
			var.vt = VT_UI4;
			var.uintVal = vvar.uiVal;
			break;
		case VALType_Float:
			var.vt = VT_R4;
			var.fltVal = vvar.fVal;
			break;
		case VALType_Boolean:
			var.vt = VT_BOOL;
			var.boolVal = vvar.blVal;
			break;
		case VALType_String:
			var.vt = VT_BSTR;
			var.bVal = NULL;
			break;
		case VALType_Double:
			var.vt = VT_R8;
			var.dblVal = vvar.dVal;
			break;
		case VALType_Decimal:
			var.vt = VT_DECIMAL;
		/*	var.decVal.wReserved = vvar.decVal.wReserved;
			var.decVal.signscale = vvar.decVal.signscale;
			var.decVal.Hi32 = vvar.decVal.Hi32;
			var.decVal.Lo64 = vvar.decVal.Lo64;*/
			break;
		case VALType_DateTime:
			var.vt = VT_DATE;
			var.date = vvar.date;
			break;
		case VALType_Int64:
			var.vt = VT_I8;
			var.llVal = vvar.lVal;
			break;
		case VALType_UInt64:
			var.vt = VT_UI8;
			var.ullVal = vvar.ulVal;
			break;
		}
	}
	return var;
}

ST_VARIANT NodeTree::FromVARIANT(const VARIANT & var)
{
	ST_VARIANT vvar;
	vvar.vt = NULL;
	switch(var.vt)
	{
	case VT_I1:
		vvar.vt =VALType_Byte;
		vvar.bVal = var.bVal;
		break;
	case VT_I2:
		vvar.vt = VALType_Int16;
		vvar.sVal = var.iVal;
		break;
	case VT_I4:
		vvar.vt = VALType_Int32;
		vvar.iVal = var.intVal;
		break;
	case VT_UI1:
		vvar.vt = VALType_Byte;
		vvar.bVal = var.bVal;
		break;
	case VT_UI2:
		vvar.vt = VALType_UInt16;
		vvar.usVal = var.uiVal;
		break;
	case VT_UI4:
		vvar.vt = VALType_UInt32;
		vvar.uiVal = var.uintVal;
		break;
	case VT_R4:
		vvar.vt = VALType_Float;
		vvar.fVal = var.fltVal;
		break;
	case VT_BOOL:
		vvar.vt = VALType_Boolean;
		vvar.blVal = var.boolVal;
		break;
	case VT_BSTR:
		{
			vvar.vt = VALType_String;
			vvar.bVal = NULL;
		}
		break;
	case VT_R8:
		vvar.vt = VALType_Double;
		vvar.dVal = var.dblVal;
		break;
	case VT_DECIMAL:
		vvar.vt = VALType_Decimal;
		/*vvar.decVal.wReserved = var.decVal.wReserved;
		vvar.decVal.signscale = var.decVal.signscale;
		vvar.decVal.Hi32 = var.decVal.Hi32;
		vvar.decVal.Lo64 = var.decVal.Lo64;*/
		break;
	case VT_DATE:
		vvar.vt = VALType_DateTime;
		vvar.date = var.date;
		break;
	case VT_I8:
		vvar.vt = VALType_Int64;
		vvar.dVal = var.llVal;
		break;
	case VT_UI8:
		vvar.vt = VALType_UInt64;
		vvar.dVal = var.ullVal;
		break;
	}
	return vvar;
}
#endif
