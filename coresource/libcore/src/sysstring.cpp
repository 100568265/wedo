#include "sysstring.h"

#include <stdio.h>
#include <string.h>

ST_CHAR *Strcpy (ST_CHAR *dest,const ST_CHAR *src)
{
	ST_CHAR *tmp = dest;
	while ((*dest++ = *src++) != '\0')
		continue;
	return tmp;
}

ST_CHAR *Strncpy (ST_CHAR *dest, const ST_CHAR *src, ST_INT32 num)
{
	ST_CHAR *tmp = dest;
	while (--num > 0 && (*dest++ = *src++) != '\0')
		continue;

    *dest = '\0';
	return tmp;
}

ST_CHAR *Strcat (ST_CHAR *dest, const ST_CHAR *src)
{
	ST_CHAR *tmp = dest;
	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		continue;
	return tmp;
}

/**
 * [Strncat  将src拷贝到dest字符串结尾]
 * @Author    Wedo
 * @NotesTime 2016-08-07T12:45:56+0800
 * @param     dest                     [目标字符串]
 * @param     src                      [源字符串]
 * @param     num                      [目标字符串长度]
 * @return                             [返回目标字符串]
 */
ST_CHAR *Strncat (ST_CHAR *dest, const ST_CHAR *src, ST_INT32 num)
{
	ST_CHAR *tmp = dest;
	while (*dest && --num > 0)
		++dest;

    while (--num > 0 && (*dest++ = *src++) != '\0')
		continue;
    *dest = '\0';
    return tmp;
}

int Strcmp (const ST_CHAR *cs, const ST_CHAR *ct)
{
	register ST_CHAR __res;
	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
	}
	return __res;
}

int Strlen (const ST_CHAR *s)
{
	const ST_CHAR *sc;
	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return (int)(sc - s);
}

/**
 * [AlignStrlen  在字符串长度(包含空字符)的基础上计算内存对齐长度]
 * @Author    Wedo
 * @NotesTime 2016-08-06T13:16:31+0800
 * @param     s                        [源字符串]
 * @return                             [返回长度]
 */
ST_UINT AlignStrlen (const ST_CHAR *s)
{
	if (!s) return 0;
	ST_UINT len = 01;
	while (*s++ != '\0')
		++len;
	return len + (sizeof(int) - (len % sizeof(int)));
}

ST_CHAR *Strtrim (ST_CHAR *s,const ST_CHAR c)
{
    ST_CHAR *f=s;
    ST_CHAR *l=s;
	while(*f==c && *f!='\0' ){
		f++;
	}
    while(*l!='\0'){
		l++;
	}
	while(l>f && (*l==c || *l=='\0')){
		l--;
	}
	while(l>=f){
		*s++=*f++;
	}
    *(s)='\0';
    return s;
}

int StrLastIndexOf(const ST_CHAR *s,const ST_CHAR c)
{
    const ST_CHAR *l=s;
    while(*l!='\0')l++;
    while(l>=s && *l!=c)
     l--;
    return l-s;
}

int StrIndexOf(const ST_CHAR *s,const ST_CHAR c)
{
    const ST_CHAR *l=s;
    while(*l!=c && *l!='\0')l++;
    return l-s;
}

int Int2Str (ST_INT iVal, char *str)
{
    sprintf (str,"%d",iVal);
    return 1;
}
