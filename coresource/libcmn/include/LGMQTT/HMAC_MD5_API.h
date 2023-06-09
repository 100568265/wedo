#pragma once

//-------------------��������-------------
#ifdef __cplusplus
extern "C"{
#endif
#include "datatype.h"
/*
���ܣ������������ݵ�MD5��ϣֵ
��ڲ�����
    inputBuffer����������
    inputCount���������ݳ��ȣ��ֽ�����
    outputBuffer���������ݵĹ�ϣֵ
����ֵ��
    ��ϣֵ����Ч���ȣ��ֽ�����
*/
ST_INT MD5_Hash( const ST_BYTE* inputBuffer, ST_INT inputCount, ST_BYTE* outputBuffer );

/*
���ܣ������������ݵ�HMAC-MD5��ϣֵ
��ڲ�����
    inputBuffer����������
    inputCount���������ݳ��ȣ��ֽ�����
    userKey���û���Կ
    UserKeyLen���û���Կ����
    outputBuffer���������ݵĹ�ϣֵ
����ֵ��
    ��ϣֵ����Ч���ȣ��ֽ�����
*/
ST_INT HMAC_MD5_Hash( const ST_BYTE* inputBuffer, ST_INT inputCount, const ST_BYTE* userKey, ST_INT UserKeyLen, ST_BYTE* outputBuffer );

/*
���ܣ������������ݵ�MD5��ϣֵ����ת��ΪBASE64�����ַ��������
��ڲ�����
    inputBuffer����������
    inputCount���������ݳ��ȣ��ֽ�����
    outputBuffer��MD5��ϣֵ��BASE64�����ַ���
����ֵ��
    BASE64�����ַ������ȣ��ַ�����,�������ַ���������
*/
ST_INT MD5_BASE64( const ST_BYTE* inputBuffer, ST_INT inputCount, ST_BYTE* outputBuffer );

/*
���ܣ������������ݵ�HMAC-MD5��ϣֵ����ת��ΪBASE64�����ַ��������
��ڲ�����
    inputBuffer����������
    inputCount���������ݳ��ȣ��ֽ�����
    userKey���û���Կ
    UserKeyLen���û���Կ����
    outputBuffer��HMAC-MD5��ϣֵ��BASE64�����ַ���
����ֵ��
    BASE64�����ַ������ȣ��ַ�����,�������ַ���������
*/
ST_INT HMAC_MD5_BASE64( const ST_BYTE* inputBuffer, ST_INT inputCount, const ST_BYTE* userKey, ST_INT UserKeyLen, ST_CHAR* outputBuffer );

#ifdef __cplusplus
}
#endif

