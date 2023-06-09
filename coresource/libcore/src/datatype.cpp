#include "datatype.h"
#include "sysmalloc.h"
#include "sysstring.h"
#include <stdio.h>

const ST_UINT32 TXJ_VERSION  = 0x0215002DUL;
const char * TXJ_VERSION_STR = "2.21.45.0000";

ST_VOID InitVARIANT(ST_VARIANT *pVar)
{
	Memset(pVar,0,sizeof(ST_VARIANT));
}

ST_VOID ClearVARIANT(ST_VARIANT *pVar)
{
	if(pVar->vt == VALType_String || pVar->vt == VALType_Binary)
	{
		if(pVar->strVal!=NULL)
		{
			Free(pVar->strVal);
		}
		pVar->strVal = NULL;
		pVar->wReserved1 = 0;
	}
	pVar->lVal=0;
	pVar->decVal.signscale =0;
	pVar->decVal.hi32 = 0;
	pVar->decVal.lo64 = 0;
	pVar->decVal.wReserved = 0;
}

ST_VOID CopyVARIANT(ST_VARIANT *pDstVar,ST_VARIANT *pSrcVar)
{
	Memcpy(pDstVar,pSrcVar,sizeof(ST_VARIANT));
	if(pDstVar->vt == VALType_String && pSrcVar->strVal != NULL)
	{
		pDstVar->wReserved1 = Strlen(pSrcVar->strVal) + 1;
		pDstVar->strVal = (ST_CHAR*)Malloc(pDstVar->wReserved1);
		Strcpy(pDstVar->strVal,pSrcVar->strVal);
	}
}

ST_INT SetVARIANTValue_int32(ST_VARIANT *pVar,ST_INT32 val)
{
	switch(pVar->vt)
	{
	case VALType_Byte:
		pVar->bVal = (ST_BYTE)val;
		break;
	case VALType_Int16:
		pVar->sVal = (ST_INT16)val;
		break;
	case VALType_Int32:
		pVar->iVal = (ST_INT32)val;
		break;
    case VALType_Int64:
		pVar->lVal = (ST_INT64)val;
		break;
	case VALType_UInt16:
		pVar->usVal = (ST_UINT16)val;
		break;
	case VALType_UInt32:
		pVar->uiVal = (ST_UINT32)val;
		break;
	case VALType_UInt64:
		pVar->ulVal = (ST_UINT64)val;
		break;
	case VALType_Float:
		pVar->fVal = (ST_FLOAT)val;
		break;
	case VALType_Boolean:
		pVar->blVal = (ST_BOOLEAN)val;
		break;
    case VALType_Double:
		pVar->dtVal = (ST_DOUBLE)val;
		break;
	case VALType_Decimal:
		return SD_FAILURE;
	case VALType_DateTime:
		return SD_FAILURE;
	case VALType_String:
        return SD_FAILURE;
	case VALType_Binary:
		return SD_FAILURE;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_int64(ST_VARIANT *pVar,ST_INT64 val)
{
	switch(pVar->vt)
	{
	case VALType_Byte:
		pVar->bVal = (ST_BYTE)val;
		break;
	case VALType_Int16:
		pVar->sVal = (ST_INT16)val;
		break;
	case VALType_Int32:
		pVar->iVal = (ST_INT32)val;
		break;
    case VALType_Int64:
		pVar->lVal = (ST_INT64)val;
		break;
	case VALType_UInt16:
		pVar->usVal = (ST_UINT16)val;
		break;
	case VALType_UInt32:
		pVar->uiVal = (ST_UINT32)val;
		break;
	case VALType_UInt64:
		pVar->ulVal = (ST_UINT64)val;
		break;
	case VALType_Float:
		pVar->fVal = (ST_FLOAT)val;
		break;
	case VALType_Boolean:
		pVar->blVal = (ST_BOOLEAN)val;
		break;
    case VALType_Double:
		pVar->dtVal = (ST_DOUBLE)val;
		break;
	case VALType_Decimal:
		return SD_FAILURE;
	case VALType_DateTime:
		return SD_FAILURE;
	case VALType_String:
        return SD_FAILURE;
	case VALType_Binary:
		return SD_FAILURE;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_uint32(ST_VARIANT *pVar,ST_UINT32 val)
{
    switch(pVar->vt)
	{
	case VALType_Byte:
		pVar->bVal = (ST_BYTE)val;
		break;
	case VALType_Int16:
		pVar->sVal = (ST_INT16)val;
		break;
	case VALType_Int32:
		pVar->iVal = (ST_INT32)val;
		break;
    case VALType_Int64:
		pVar->lVal = (ST_INT64)val;
		break;
	case VALType_UInt16:
		pVar->usVal = (ST_UINT16)val;
		break;
	case VALType_UInt32:
		pVar->uiVal = (ST_UINT32)val;
		break;
	case VALType_UInt64:
		pVar->ulVal = (ST_UINT64)val;
		break;
	case VALType_Float:
		pVar->fVal = (ST_FLOAT)val;
		break;
	case VALType_Boolean:
		pVar->blVal = (ST_BOOLEAN)val;
		break;
    case VALType_Double:
		pVar->dtVal = (ST_DOUBLE)val;
		break;
	case VALType_Decimal:
		return SD_FAILURE;
	case VALType_DateTime:
		return SD_FAILURE;
	case VALType_String:
        return SD_FAILURE;
	case VALType_Binary:
		return SD_FAILURE;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_uint64(ST_VARIANT *pVar,ST_UINT64 val)
{
    switch(pVar->vt)
	{
	case VALType_Byte:
		pVar->bVal = (ST_BYTE)val;
		break;
	case VALType_Int16:
		pVar->sVal = (ST_INT16)val;
		break;
	case VALType_Int32:
		pVar->iVal = (ST_INT32)val;
		break;
    case VALType_Int64:
		pVar->lVal = (ST_INT64)val;
		break;
	case VALType_UInt16:
		pVar->usVal = (ST_UINT16)val;
		break;
	case VALType_UInt32:
		pVar->uiVal = (ST_UINT32)val;
		break;
	case VALType_UInt64:
		pVar->ulVal = (ST_UINT64)val;
		break;
	case VALType_Float:
		pVar->fVal = (ST_FLOAT)val;
		break;
	case VALType_Boolean:
		pVar->blVal = (ST_BOOLEAN)val;
		break;
    case VALType_Double:
		pVar->dtVal = (ST_DOUBLE)val;
		break;
	case VALType_Decimal:
		return SD_FAILURE;
	case VALType_DateTime:
		return SD_FAILURE;
	case VALType_String:
        return SD_FAILURE;
	case VALType_Binary:
		return SD_FAILURE;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_float(ST_VARIANT *pVar,ST_FLOAT val)
{
    switch(pVar->vt)
	{
	case VALType_Byte:
		pVar->bVal = (ST_BYTE)val;
		break;
	case VALType_Int16:
		pVar->sVal = (ST_INT16)val;
		break;
	case VALType_Int32:
		pVar->iVal = (ST_INT32)val;
		break;
    case VALType_Int64:
		pVar->lVal = (ST_INT64)val;
		break;
	case VALType_UInt16:
		pVar->usVal = (ST_UINT16)val;
		break;
	case VALType_UInt32:
		pVar->uiVal = (ST_UINT32)val;
		break;
	case VALType_UInt64:
		pVar->ulVal = (ST_UINT64)val;
		break;
	case VALType_Float:
		pVar->fVal = (ST_FLOAT)val;
		break;
	case VALType_Boolean:
		pVar->blVal = (ST_BOOLEAN)val;
		break;
    case VALType_Double:
		pVar->dtVal = (ST_DOUBLE)val;
		break;
	case VALType_Decimal:
		return SD_FAILURE;
	case VALType_DateTime:
		return SD_FAILURE;
	case VALType_String:
        return SD_FAILURE;
	case VALType_Binary:
		return SD_FAILURE;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_double(ST_VARIANT *pVar,ST_DOUBLE val)
{
    switch(pVar->vt)
	{
	case VALType_Byte:
		pVar->bVal = (ST_BYTE)val;
		break;
	case VALType_Int16:
		pVar->sVal = (ST_INT16)val;
		break;
	case VALType_Int32:
		pVar->iVal = (ST_INT32)val;
		break;
    case VALType_Int64:
		pVar->lVal = (ST_INT64)val;
		break;
	case VALType_UInt16:
		pVar->usVal = (ST_UINT16)val;
		break;
	case VALType_UInt32:
		pVar->uiVal = (ST_UINT32)val;
		break;
	case VALType_UInt64:
		pVar->ulVal = (ST_UINT64)val;
		break;
	case VALType_Float:
		pVar->fVal = (ST_FLOAT)val;
		break;
	case VALType_Boolean:
		pVar->blVal = (ST_BOOLEAN)val;
		break;
    case VALType_Double:
		pVar->dtVal = (ST_DOUBLE)val;
		break;
	case VALType_Decimal:
		return SD_FAILURE;
	case VALType_DateTime:
		return SD_FAILURE;
	case VALType_String:
        return SD_FAILURE;
	case VALType_Binary:
		return SD_FAILURE;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_String(ST_VARIANT *pVar,ST_CHAR *val)
{
	switch(pVar->vt)
	{
        case VALType_String:
        {
            int len = Strlen(val);
            if(len == 0)return SD_SUCCESS;
            if(!pVar->strVal)
            {
                pVar->wReserved1 = len + 1;
                pVar->strVal = (ST_CHAR*)Malloc(pVar->wReserved1);
            }
            else if(len >= pVar->wReserved1)
            {
                pVar->wReserved1 = len + 1;
                pVar->strVal = (ST_CHAR*)Realloc(pVar->strVal,pVar->wReserved1);
            }
            Strcpy(pVar->strVal,val);
        }
            break;
        default:
            return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_DateTime(ST_VARIANT *pVar,ST_DOUBLE val)
{
	switch(pVar->vt)
	{
	case VALType_DateTime:
		//pVar->date = val;
		break;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_Boolean(ST_VARIANT *pVar,ST_BOOLEAN val)
{
	switch(pVar->vt)
	{
    case VALType_Boolean:
		pVar->blVal = (ST_BOOLEAN)val;
		break;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}

ST_INT SetVARIANTValue_Decimal(ST_VARIANT *pVar,ST_DECIMAL val)
{
	switch(pVar->vt)
	{
	case VALType_DateTime:
		pVar->decVal.wReserved = val.wReserved;
		pVar->decVal.signscale = val.signscale;
		pVar->decVal.hi32 = val.hi32;
		pVar->decVal.lo64 = val.lo64;
		break;
	default:
		return SD_FAILURE;
	}
	return SD_SUCCESS;
}



