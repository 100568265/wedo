# ifndef _Dataitem645_H
# define _Dataitem645_H

//
struct TYPE
{
    enum
    {
        XXXX        = 1,
        XXXXXX      = 2,
        XXXXXXXX    = 3
    };
};

#pragma pack(push,1)
template<int type> struct DataItem {};

// XXXX
template<>
struct DataItem<1>
{
    union
    {
        struct
        {
            ST_BYTE ge      :4;
            ST_BYTE shi     :4;
            ST_BYTE bai     :4;
            ST_BYTE qian    :4;
        };
        //ST_BYTE buf[2];
    };
    DataItem<1> operator - (ST_BYTE dat)
    {
        ST_BYTE *p = (ST_BYTE*)this;
        for(int i=0; i<(sizeof(DataItem<1>)); i++)
        {
            i[p] -= dat;
        }
        return *this;
    }
};

// XXXXXX
template<>
struct DataItem<2>
{
    union
    {
        struct
        {
            ST_BYTE ge      :4;
            ST_BYTE shi     :4;
            ST_BYTE bai     :4;
            ST_BYTE qian    :4;
            ST_BYTE wan     :4;
            ST_BYTE sW      :4;
        };
        ST_BYTE buf[3];
    };
};


// XXXXXXXX
template<>
struct DataItem<3>
{
    union
    {
        struct
        {
            ST_BYTE ge      :4;
            ST_BYTE shi     :4;
            ST_BYTE bai     :4;
            ST_BYTE qian    :4;
            ST_BYTE wan     :4;
            ST_BYTE sW      :4;
            ST_BYTE bW      :4;
            ST_BYTE qW      :4;
        };
        ST_BYTE buf[4];
    };
};

/*
case 0x0201FF00:
            case 0x35343233: { // 电压
                float fvalue = 0;
                DataItem<TYPE::XXXX> data;
                for(int i = 0; i < 3; ++i)
                {
                    data =  *((DataItem<TYPE::XXXX> *)(&pbuf[14+i*2]));
                    data = data - 0x33;
                    fvalue = (float)(data.ge + data.shi *10 + data.bai *100 + data.qian *1000)*0.1;
                    this->UpdateValue(i, fvalue);
                }
            } break;
            */
#pragma pack(pop)

# endif
