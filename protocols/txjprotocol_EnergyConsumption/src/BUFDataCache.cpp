#include "BUFDataCache.h"
#include "sysmalloc.h"

BUFDataCache::BUFDataCache()
{
    //ctor
}

BUFDataCache::~BUFDataCache()
{
    //dtor
}

void    BUFDataCache::pushBUF(ST_BYTE *dest,int bSize)
{
    DataInfo pData;
    pData.pBuf = new ST_BYTE[bSize];
    Memcpy(pData.pBuf,dest,bSize);
    pData.pSize = bSize;
    push(pData);
}

void    BUFDataCache::get_front_BUF(ST_BYTE *dest,int &bSize)
{
    DataInfo pData = get_front();
    Memcpy(dest,pData.pBuf,pData.pSize);
    bSize = pData.pSize;
}

void    BUFDataCache::get_back_BUF(ST_BYTE *dest,int &bSize)
{
    DataInfo pData = get_back();
    Memcpy(dest,pData.pBuf,pData.pSize);
    bSize = pData.pSize;
}

bool    BUFDataCache::pop()
{
    DataInfo pData = get_front();
    DataCaches::pop();
    delete[] pData.pBuf;
}

