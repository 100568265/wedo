#pragma once
#include "datatype.h"
#ifdef  __cplusplus
extern "C" {
#endif

/*
���ܣ�������������ת����BASE64�����ַ���
����˵����
    inputBuffer��Ҫ����Ķ���������
    inputCount�����ݳ���
    outputBuffer���洢ת�����BASE64�����ַ���
����ֵ��
     -1����������
    >=0����Ч���볤�ȣ��ַ�����,�������ַ�����������
��ע��
    ��Ч��openssl��EVP_EncodeBlock����
*/
ST_INT BASE64_Encode( const ST_BYTE* inputBuffer, ST_INT inputCount, ST_CHAR* outputBuffer );

/*
���ܣ���BASE64�����ַ���ת��Ϊ����������
����˵����
    inputBuffer��BASE64�����ַ���
    inputCount�����볤�ȣ��ַ�����,Ӧ��Ϊ4�ı�����
    outputBuffer���洢ת����Ķ���������
����ֵ��
     -1����������
     -2�����ݴ���
    >=0��ת������ֽ���
��ע��
    ��Ч��openssl��EVP_DecodeBlock����
*/
ST_INT  BASE64_Decode( const ST_CHAR* inputBuffer, ST_INT inputCount, ST_BYTE* outputBuffer );

#ifdef  __cplusplus
}
#endif
