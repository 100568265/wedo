#ifndef SYSSTRING_H_INCLUDED
#define SYSSTRING_H_INCLUDED

#include "datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ST_CHAR		*Strcpy (ST_CHAR *, const ST_CHAR *);
extern ST_CHAR		*Strcat (ST_CHAR *, const ST_CHAR *);
extern ST_INT		Strcmp(const ST_CHAR*,const ST_CHAR*);
extern ST_CHAR      *Strncpy(ST_CHAR *, const ST_CHAR *, ST_INT32);
extern ST_CHAR      *Strncat(ST_CHAR *, const ST_CHAR *, ST_INT32);
extern ST_INT		Strlen(const ST_CHAR*);
extern ST_UINT		AlignStrlen (const ST_CHAR *);
extern ST_CHAR		*Strtrim(ST_CHAR *,const ST_CHAR);
extern ST_INT		StrIndexOf(const ST_CHAR*,ST_CHAR);
extern ST_INT		StrLastIndexOf(const ST_CHAR*,ST_CHAR);
extern ST_INT		Int2Str(ST_INT iVal, char *str);

#ifdef __cplusplus
}
#endif

#endif // SYSSTRING_H_INCLUDE


