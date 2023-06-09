#ifndef SYSMALLOC_H_INCLUDED
#define SYSMALLOC_H_INCLUDED

#include "datatype.h"

class st_nullptr_t {
public:
template<class T>
    operator T* () const { return 0; }

template<class C, class T>
    operator T C::* () const { return 0; }

private:
    void operator& () const;
};

extern const st_nullptr_t st_nullptr;

#ifdef __cplusplus
extern "C" {
#endif

extern ST_VOID 	*Malloc  (ST_UINT size);
extern ST_VOID 	Free     (ST_VOID *ptr);
extern ST_VOID 	*Realloc (ST_VOID *ptr,ST_UINT newSize);
extern ST_VOID 	*Alloc   (ST_UINT size);
extern ST_VOID 	*Memset  (ST_VOID *s,ST_UINT c,ST_UINT count);
extern ST_VOID 	*Memcpy  (ST_VOID *dest,const ST_VOID *src,ST_UINT count);
extern ST_VOID  *Memmove (ST_VOID *dest,const ST_VOID *src,ST_UINT count);

#ifdef __cplusplus
}
#endif
#endif // SYSMALLOC_H_INCLUDED
