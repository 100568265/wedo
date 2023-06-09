#pragma once
#include "datatype.h"
#ifdef  __cplusplus
extern "C" {
#endif

/*
功能：将二进制数据转换成BASE64编码字符串
参数说明：
    inputBuffer：要编码的二进制数据
    inputCount：数据长度
    outputBuffer：存储转换后的BASE64编码字符串
返回值：
     -1：参数错误
    >=0：有效编码长度（字符数）,不包括字符串结束符。
备注：
    等效于openssl中EVP_EncodeBlock函数
*/
ST_INT BASE64_Encode( const ST_BYTE* inputBuffer, ST_INT inputCount, ST_CHAR* outputBuffer );

/*
功能：将BASE64编码字符串转换为二进制数据
参数说明：
    inputBuffer：BASE64编码字符串
    inputCount：编码长度（字符数）,应该为4的倍数。
    outputBuffer：存储转换后的二进制数据
返回值：
     -1：参数错误
     -2：数据错误
    >=0：转换后的字节数
备注：
    等效于openssl中EVP_DecodeBlock函数
*/
ST_INT  BASE64_Decode( const ST_CHAR* inputBuffer, ST_INT inputCount, ST_BYTE* outputBuffer );

#ifdef  __cplusplus
}
#endif
