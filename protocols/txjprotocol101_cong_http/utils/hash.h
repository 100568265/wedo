#ifndef __HASH_H__
#define __HASH_H__

unsigned long simple_hash11(const char* str, unsigned int len);
unsigned long simple_hash31(const char* str, unsigned int len);
unsigned long djb_hash(const char* str, unsigned int len);
unsigned long sdbm_hash(const char* str, unsigned int len);
unsigned long dek_hash(const char* str, unsigned int len);
unsigned long bp_hash(const char* str, unsigned int len);
unsigned long pjw_hash(const char* str, unsigned int len);

#endif
