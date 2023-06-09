#ifndef DATACENTER_H
#define DATACENTER_H
#include "Protocol.h"     //¹æÔ¼¸¸Àà
#include "DataCache.h"    //Êý¾Ý»º³åÀà
#include "Device.h"       //Í¨Ñ¸Éè±¸Àà
#include "Devices.h"      //Í¨Ñ¸Éè±¸¹ÜÀíÀà
#include "sysinifile.h"   //INIÎÄ¼þ¶ÁÈ¡Àà
#include "datatype.h" //
#include "rtobjecttree.h" //
#include <list>
#include <map>
using namespace std;
class NodeTree;
/*
struct xmlDateType{
            int areaId;
            int id;
            float fvalue;
            int dHour;
            int dminute;
        };
struct cpTimer{
        int c_mon;
        int c_day;
        int c_min;
};
struct curTime{
        int dHour;
        int dminute;
};
struct itemType{
    int id;
    char itemName[64];
    int fbool;
    float fvalue;
    double dvalue;
};
typedef list<xmlDateType> LISTXmlData;
typedef list<curTime> LISTXmlTime;
typedef list<itemType> LISTItemData;*/
struct cpTimer{
        int c_mon;
        int c_day;
        int c_min;
};

struct curTime{
        int dHour;
        int dminute;
};

class DataCenter
{
    public:
        DataCenter();
        virtual ~DataCenter();

        int         wdTime;
        void        getbefordayString(int bTime,char *dest);
        void        getlasttimeinsert(curTime *lastTime);
        void        inserAllDataInXml();
        void        writeAllInXml(struct curTime tData);


        void        writeInitXml();
        void        checkFileExist();
        void        checkAreaEixst();
        void        delBeforFile();

        int         getHistoryData(int befday,int dtime,ST_BYTE *pbuf);
        void        assemblydata(ST_BYTE *dbuf);
        void        calculationValue(double d1,double d2,double d3,double d4,ST_BYTE *dpbuf);
        void        fv2UIntt16(float fvalue,ST_BYTE *pbuf);
        void        dv2UIntt32(double dvalue,ST_BYTE *pbuf);
        void        waterStatus(int pos,ST_BYTE *pbuf);

        double      getdData2tableList(int i,int j);
        float       getfData2tableList(int i,int j);

        double      m_hisdlist[10][100];
    protected:
    private:
};

#endif // DATACENTER_H
