#include "sysmalloc.h"

#include <string.h>
#include <stdlib.h>


ST_VOID *Alloc(ST_UINT size)
{
	return malloc(size);
}

ST_VOID *Malloc(ST_UINT size)
{
	return malloc(size);
}

ST_VOID Free(ST_VOID *ptr)
{
    if(ptr != NULL) {
        free (ptr);
        ptr = NULL;
	}
}

ST_VOID *Realloc(ST_VOID *ptr,ST_UINT newSize)
{
	return realloc(ptr,newSize);
}

ST_VOID *Memset(ST_VOID *s,ST_UINT c,ST_UINT count)
{
	return memset(s,c,count);
}

ST_VOID *Memcpy(ST_VOID *dest,const ST_VOID *src,ST_UINT count)
{
	return memcpy(dest,src,count);
}

ST_VOID *Memmove(ST_VOID *dest,const ST_VOID *src,ST_UINT count)
{
	return memmove (dest, src, count);
}
