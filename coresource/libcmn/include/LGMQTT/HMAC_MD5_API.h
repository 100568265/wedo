#pragma once

//-------------------导出函数-------------
#ifdef __cplusplus
extern "C"{
#endif
#include "datatype.h"
/*
功能：计算输入数据的MD5哈希值
入口参数：
    inputBuffer：输入数据
    inputCount：输入数据长度（字节数）
    outputBuffer：输入数据的哈希值
返回值：
    哈希值的有效长度（字节数）
*/
ST_INT MD5_Hash( const ST_BYTE* inputBuffer, ST_INT inputCount, ST_BYTE* outputBuffer );

/*
功能：计算输入数据的HMAC-MD5哈希值
入口参数：
    inputBuffer：输入数据
    inputCount：输入数据长度（字节数）
    userKey：用户密钥
    UserKeyLen：用户密钥长度
    outputBuffer：输入数据的哈希值
返回值：
    哈希值的有效长度（字节数）
*/
ST_INT HMAC_MD5_Hash( const ST_BYTE* inputBuffer, ST_INT inputCount, const ST_BYTE* userKey, ST_INT UserKeyLen, ST_BYTE* outputBuffer );

/*
功能：计算输入数据的MD5哈希值，并转换为BASE64编码字符串输出。
入口参数：
    inputBuffer：输入数据
    inputCount：输入数据长度（字节数）
    outputBuffer：MD5哈希值的BASE64编码字符串
返回值：
    BASE64编码字符串长度（字符数）,不包括字符串结束符
*/
ST_INT MD5_BASE64( const ST_BYTE* inputBuffer, ST_INT inputCount, ST_BYTE* outputBuffer );

/*
功能：计算输入数据的HMAC-MD5哈希值，并转换为BASE64编码字符串输出。
入口参数：
    inputBuffer：输入数据
    inputCount：输入数据长度（字节数）
    userKey：用户密钥
    UserKeyLen：用户密钥长度
    outputBuffer：HMAC-MD5哈希值的BASE64编码字符串
返回值：
    BASE64编码字符串长度（字符数）,不包括字符串结束符
*/
ST_INT HMAC_MD5_BASE64( const ST_BYTE* inputBuffer, ST_INT inputCount, const ST_BYTE* userKey, ST_INT UserKeyLen, ST_CHAR* outputBuffer );

#ifdef __cplusplus
}
#endif

