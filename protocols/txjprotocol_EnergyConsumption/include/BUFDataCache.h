#ifndef BUFDATACACHE_H
#define BUFDATACACHE_H

#include "DataCaches.h"
#include "datatype.h"

class BUFDataCache : public DataCaches
{
    public:
        BUFDataCache();
        virtual ~BUFDataCache();

        void    pushBUF(ST_BYTE *dest,int size);
        void    get_front_BUF(ST_BYTE *dest,int &size);
        void    get_back_BUF(ST_BYTE *dest,int &size);
        bool    pop();
    protected:
    private:
};

#endif // BUFDATACACHE_H
