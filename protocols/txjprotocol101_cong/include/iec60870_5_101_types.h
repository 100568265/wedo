
#ifndef __IEC_60870_5_101_TYPES_H
#define __IEC_60870_5_101_TYPES_H

#if __cplusplus > 201100L
	#include <stdint.h>
#else
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
#endif

#define NAMESPACE_BEGIN_IEC101// namespace IEC101 {
#define NAMESPACE_ENDED_IEC101// };

NAMESPACE_BEGIN_IEC101

#pragma pack(push,1)	// 紧凑对齐

// 32-bit string state and change data unit
struct iec_stcd {
    union {
        unsigned short  st;
        struct {
            unsigned char st1  :1;
            unsigned char st2  :1;
            unsigned char st3  :1;
            unsigned char st4  :1;
            unsigned char st5  :1;
            unsigned char st6  :1;
            unsigned char st7  :1;
            unsigned char st8  :1;
            unsigned char st9  :1;
            unsigned char st10 :1;
            unsigned char st11 :1;
            unsigned char st12 :1;
            unsigned char st13 :1;
            unsigned char st14 :1;
            unsigned char st15 :1;
            unsigned char st16 :1;
        };
    };
    union {
        unsigned short  cd;
        struct {
            unsigned char cd1  :1;
            unsigned char cd2  :1;
            unsigned char cd3  :1;
            unsigned char cd4  :1;
            unsigned char cd5  :1;
            unsigned char cd6  :1;
            unsigned char cd7  :1;
            unsigned char cd8  :1;
            unsigned char cd9  :1;
            unsigned char cd10 :1;
            unsigned char cd11 :1;
            unsigned char cd12 :1;
            unsigned char cd13 :1;
            unsigned char cd14 :1;
            unsigned char cd15 :1;
            unsigned char cd16 :1;
        };
    };
};

// CP56Time2a //timestamp
// Defined in IEC60870-5-101 7.2.6.18
struct CP56Time2a {
    unsigned short msec;
    unsigned char min   :6;
    unsigned char res1  :1;
    unsigned char iv    :1;
    unsigned char hour  :5;
    unsigned char res2  :2;
    unsigned char su    :1;
    unsigned char mday  :5;	// day for month
    unsigned char wday  :3; // day for week
    unsigned char month :4;
    unsigned char res3  :4;
    unsigned char year  :7;
    unsigned char res4  :1;
};

struct CP24Time2a {
    unsigned short msec;
    unsigned char min   :6;
    unsigned char res1  :1;
    unsigned char iv    :1;
};

struct CP16Time2a {
    unsigned short msec;
};

struct qualif
{
    unsigned char var :2;
    unsigned char res :2;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// Undefined
template<int TYP> struct Type {};

// M_SP_NA_1 - (single point information with quality description)
template<>
struct Type<1> {
    unsigned char spi :1; // single point information
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_SP_TA_1
template<>
struct Type<2> {
    unsigned char spi :1; // single point information
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP24Time2a ts;
};

// M_DP_NA_1 - (double point information with quality description)
template<>
struct Type<3> {
    unsigned char dpi :2; // double point information
    unsigned char res :2;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_DP_TA_1
template<>
struct Type<4> {
    unsigned char dpi :2; // double point information
    unsigned char res :2;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP24Time2a ts;
};

// M_ST_NA_1 - (step position)
template<>
struct Type<5> {
    unsigned char mv  :7; // value
    unsigned char t   :1; // transient flag
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_BO_NA_1 - (state and change information bit string)
template<>
struct Type<7> {
    struct iec_stcd stcd;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_ME_NA_1 - (normalized measured value)
//归一化值
//　　NVA:F16[1...16]
//　　值得范围-1~1,将大于1的数映射到1以内的空间，通常就是用实际值除以额定值，即得到归一化的小数。具体表示法可以有F13或F16位的。２字节
template<>
struct Type<9> {
    unsigned short mv;    // normalized value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_ME_NB_1 - (scaled measured value)
template<>
struct Type<11> {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_ME_NC_1 - (short floating point measured value)
template<>
struct Type<13> {
    float mv;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

template<>
struct Type<15> {
    int32_t mv;
    unsigned char sq :5;
    unsigned char cy :1; // carry/no carry
    unsigned char ca :1; // Counter was adjusted/Counter was not adjusted
    unsigned char iv :1;
};

// M_SP_TB_1 - (single point information with quality description and time tag)
template<>
struct Type<30> {
    unsigned char spi :1; // single point information
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_DP_TB_1 - (double point information with quality description and time tag)
template<>
struct Type<31> {
    unsigned char dp  :2; // double point information
    unsigned char res :2;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_ST_TB_1 - (step position with time tag)
template<>
struct Type<32> {
    unsigned char mv  :7; // value
    unsigned char t   :1; // transient flag
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_BO_TB_1 - (state and change information bit string and time tag)
template<>
struct Type<33> {
    struct iec_stcd stcd;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_ME_TD_1 - (scaled measured value with time tag)
template<>
struct Type<34> {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_ME_TE_1 - (scaled measured value with time tag)
template<>
struct Type<35> {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_ME_TF_1 - (short floating point measurement value and time tag)
template<>
struct Type<36> {
    float  mv;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_IT_TB_1
template<>
struct Type<37> {
    uint32_t bcr;
    unsigned char sq :5;
    unsigned char cy :1;
    unsigned char ca :1;
    unsigned char iv :1;
    CP56Time2a ts;
};

// C_SC_NA_1
template<>
struct Type<45> {
    unsigned char scs :1; // single command state
    unsigned char res :1; // must be zero
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
};

// C_DC_NA_1
template<>
struct Type<46> {
    unsigned char dcs :2; // double command state
    unsigned char qu :5;
    unsigned char se :1; // select=1 / execute=0
};

// C_RC_NA_1
template<>
struct Type<47> {
    unsigned char rcs :2; // regulating step command
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
};

// C_SC_TA_1
template<>
struct Type<58> {
    unsigned char scs :1; // single command state
    unsigned char res :1; // must be zero
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
    CP56Time2a ts;
};

// C_DC_TA_1
template<>
struct Type<59> {
    unsigned char dcs :2; // double command state
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
    CP56Time2a ts;
};

// C_RC_NA_1
template<>
struct Type<60> {
    unsigned char rcs :2; // regulating step command
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
    CP56Time2a ts;
};

// M_EI_NA_1
template<>
struct Type<70> {
    unsigned char coi;
};

// C_IC_NA_1
template<>
struct Type<100> {
    unsigned char qoi; // pointer of interrogation
};

// C_CI_NA_1
template<>
struct Type<101> {
    unsigned char rqt :6; // request
    unsigned char frz :2; // freeze
};

// C_CS_NA_1
template<>
struct Type<103> {
    CP56Time2a ts;
};

// C_TS_TA_1
template<>
struct Type<107> {
    unsigned short tsc; // TSC test sequence counter
    CP56Time2a ts;
};


struct CtrlField {
    uint8_t fc  :4;
    uint8_t fd  :1; // FCV/DFC
    uint8_t fa  :1; // FCB/ACD
    uint8_t prm :1;
    uint8_t dir :1;
};

struct Fixedframe {
    uint8_t start;
    CtrlField cf;
    uint8_t laddr;
    uint8_t laddr1;//linweiming
    uint8_t cs;
    uint8_t stop;
};

struct COT_OA {
    uint8_t cause :6; // 传送原因
    uint8_t pn :1;    // 否定确认：1，肯定确认：0
    uint8_t t :1;     // 试验：1，未试验：0
    uint8_t cause1;   //linweiming
    uint8_t oaddr;    // 源地址，未使用
};
struct COT_NOA {
    uint8_t cause :6; // 传送原因
    uint8_t pn :1;    // 否定确认：1，肯定确认：0
    uint8_t t :1;     // 试验：1，未试验：0
    uint8_t cause1;   //linweiming
};
struct COT_NOB {
    uint8_t cause :6; // 传送原因
    uint8_t pn :1;    // 否定确认：1，肯定确认：0
    uint8_t t :1;     // 试验：1，未试验：0
};
template<int N> struct Cot { typedef COT_NOA _Type; };
template<> struct Cot<2>   { typedef COT_OA  _Type; };
template<> struct Cot<3>   { typedef COT_NOB  _Type; };

template<int N> struct Paddr { typedef uint8_t  _Type; };
template<> struct Paddr<2>   { typedef uint16_t _Type; };

struct Ioaddr1 {
    inline void set (uint32_t addr) { l8 = (uint8_t)addr; }
    inline uint32_t get () const { return l8; }
    uint8_t l8;
};
struct Ioaddr2 {
    inline void set (uint32_t addr) { l16 = (uint16_t)addr; }
    inline uint32_t get () const { return l16; }
    uint16_t l16;
};
struct Ioaddr3 {
    inline void set (uint32_t addr) { l16 = (uint16_t)addr; h8 = (uint8_t)(addr >> 8); }
    inline uint32_t get () const { return l16 + (h8 << 16); }
    uint16_t l16; uint8_t  h8;
};
template<int N> struct Ioaddr { typedef struct Ioaddr2 _Type; };
template<> struct Ioaddr<1>   { typedef struct Ioaddr1 _Type; };
template<> struct Ioaddr<3>   { typedef struct Ioaddr3 _Type; };

// ASDU header - data unit id
template<int COTLEN = 2, int PALEN = 2>
struct DataUnitId {
    unsigned char type;  	            // 类型标识
    unsigned char num :7;	            // 信息对象数目
    unsigned char sq :1; 	            // 顺序：1，非顺序：0
    typename Cot<COTLEN>::_Type cot;    // 传送原因域
    typename Paddr<PALEN>::_Type paddr; // 公共地址域
};

struct APCI {
    enum {
        STARTCHR = 0x68,
        STOPCHR  = 0x16,
        MAX_FRAME_SIZE = 256
    };

    uint8_t start1;
    uint8_t len1;
    uint8_t len2;
    uint8_t start2;
    CtrlField cf;
    uint8_t laddr;
    uint8_t laddr1;//linweiming
};

template<int TYP, int IOALEN = 2,
    int COTLEN = 1, int PALEN = 2>
struct APDU {
    APCI apci;
    DataUnitId<COTLEN, PALEN> asduh;
    union {
        struct {
            typename Ioaddr<IOALEN>::_Type ioa;
            Type<TYP> obj;
        } nsq[1];
        struct {
            typename Ioaddr<IOALEN>::_Type ioa;
            Type<TYP> obj[1];
        } sq;
        uint8_t dados [245];
    };
};

#pragma pack(pop)		// 恢复默认内存对齐

struct FC {
    enum MAIN {
        RST_LINK        = 0,    // 复位远方链路
        RST_USER_PRO    = 1,    // 复位用户进程
        TEST_LINK       = 2,
        SEND_DATA       = 3,    // 发送
        BROADCAST_DATA  = 4,
        VISIT_ASK       = 8,
        ASK_LINK_STATE  = 9,    // 请求链路状态
        ASK_TIER1_DATA  = 10,   // 请求1级数据
        ASK_TIER2_DATA  = 11    // 请求2级数据
    };
    enum CLIENT {
        CONFIRM         = 0,    // 确认
        LINK_BUSY       = 1,    // 链路忙
        ANSWER_DATA     = 8,    // 响应用户数据
        NO_ANSWER_DATA  = 9,    // 无所响应用户数据
        LINK_PERFECT    = 11,   // 链路完好
        LINK_NO_WORK    = 14,   // 链路未工作
        LINK_NO_FINISH  = 15
    };
};
//---------------类型标识------------------//
struct TYP {
    enum type {
        // 在监视方向的过程信息
        Undefined = 0,      // 未定义
        M_SP_NA_1 = 1,      // 单点信息
        M_SP_TA_1 = 2,      //带时标的单点信息
        M_DP_NA_1 = 3,      // 双点信息
        M_DP_TA_1 = 4,      //带时标的双点信息
        M_ST_NA_1 = 5,      // 步位置信息
        M_ST_TA_1 = 6,      // 带时标的步位置信息
        M_BO_NA_1 = 7,      // 32比特串
        M_BO_TA_1 = 8,      // 带时标的32比特串
        M_ME_NA_1 = 9,      // 测量值，规一化值
        M_ME_TA_1 = 10,     // 测量值, 带时标的规一化值
        M_ME_NB_1 = 11,     // 测量值，标度化值
        M_ME_TB_1 = 12,     // 测量值, 带时标的标度化值
        M_ME_NC_1 = 13,     // 测量值，短浮点数
        M_ME_TC_1 = 14,     // 测量值, 带时标的短浮点数
        M_IT_NA_1 = 15,     // 累计量
        M_IT_TA_1 = 16,     // 带时标的累积量
        M_EP_TA_1 = 17,     // 带时标的继电保护设备事件
        M_EP_TB_1 = 18,     // 带时标的继电保护设备成组启动事件
        M_EP_TC_1 = 19,     // 带时标的继电保护设备成组输出电路信息
        M_PS_NA_1 = 20,     // 带状态检出的成组单点信息
        M_ME_ND_1 = 21,     // 不带品质描述的规一化测量值

        M_SP_TB_1 = 30,     // 带时标CP56Time2a的单点信息
        M_DP_TB_1 = 31,     // 带时标CP56Time2a的双点信息
        M_ST_TB_1 = 32,     // 带时标CP56Time2a的步位置信息
        M_BO_TB_1 = 33,     // 带时标CP56Time2a的32比特串
        M_ME_TD_1 = 34,     // 带时标CP56Time2a的测量值，规一化值
        M_ME_TE_1 = 35,     // 带时标CP56Time2a的测量值，标度化值
        M_ME_TF_1 = 36,     // 带时标CP56Time2a的测量值，短浮点数
        M_IT_TB_1 = 37,     // 带时标CP56Time2a的累计量
        M_EP_TD_1 = 38,     // 带时标CP56Time2a的继电保护装置事件
        M_EP_TE_1 = 39,     // 带时标CP56Time2a的继电保护装置成组启动事件
        M_EP_TF_1 = 40,     // 带时标CP56Time2a的继电保护装置成组出口信息

        // 在控制方向的过程信息
        C_SC_NA_1 = 45,     // 单命令
        C_DC_NA_1 = 46,     // 双命令
        C_RC_NA_1 = 47,     // 升降命令
        C_SE_NA_1 = 48,     // 设点命令，规一化值
        C_SE_NB_1 = 49,     // 设点命令，标度化值
        C_SE_NC_1 = 50,     // 设点命令，短浮点数
        C_BO_NA_1 = 51,     // 32比特串

        C_SC_TA_1 = 58,     // 带时标CP56Time2a的单命令
        C_DC_TA_1 = 59,     // 带时标CP56Time2a的双命令
        C_RC_TA_1 = 60,     // 带时标CP56Time2a的升降命令
        C_SE_TA_1 = 61,     // 带时标CP56Time2a的设点命令，规一化值
        C_SE_TB_1 = 62,     // 带时标CP56Time2a的设点命令，标度化值
        C_SE_TC_1 = 63,     // 带时标CP56Time2a的设点命令，短浮点数
        C_BO_TA_1 = 64,     // 带时标CP56Time2a的32比特串

        // 在监视方向的系统信息
        M_EI_NA_1 = 70,     // 初始化结束

        // 在控制方向的系统信息
        C_IC_NA_1 = 100,    // 总召唤命令
        C_CI_NA_1 = 101,    // 电能脉冲召唤命令
        C_RD_NA_1 = 102,    // 读命令
        C_CS_NA_1 = 103,    // 时钟同步命令
        C_TS_NA_1 = 104,    // 测试命令
        C_RP_NA_1 = 105,    // 复位进程命令
        C_TS_TA_1 = 107,    // 带时标CP56Time2a的测试命令

        // 在控制方向的参数
        P_ME_NA_1 = 110,    // 测量值参数，规一化值
        P_ME_NB_1 = 111,    // 测量值参数，标度化值
        P_ME_NC_1 = 112,    // 测量值参数，短浮点数
        P_AC_NA_1 = 113,    // 参数激活

        // 文件传输
        F_FR_NA_1 = 120,    // 文件已准备好
        F_SR_NA_1 = 121,    // 节已准备好
        F_SC_NA_1 = 122,    // 召唤目录，选择文件，召唤文件，召唤节
        F_LS_NA_1 = 123,    // 最后的节，最后的段
        F_AF_NA_1 = 124,    // 确认文件，确认节
        F_SG_NA_1 = 125,    // 段
        F_DR_NA_1 = 126,    // 目录

        // 华北电网101实施准则
        C_SE_ND_1 = 136,    // CON（镜像返回）设定值命令
        Reserve
    };
}; // TYPE ID

//---------------传送原因值------------------//
struct Cause {
    enum type {
	    CYCLIC 			= 1,	// 周期、循环
        BGSCAN 			= 2,	// 背景扫描
        SPONTANEOUS 	= 3,	// 自发、突发
	    INITIALIZE		= 4,	// 初始化
        REQUEST 		= 5,	// 请求或被请求
        ACT 			= 6,	// 激活
        ACTCON 			= 7,	// 激活确认
        DEACT 		    = 8,	// 停止激活
	    DEACTCON		= 9,	// 停止激活确认
        ACTTERM 		= 10,	// 激活终止、激活结束
	    RETREM         	= 11,	// 远方命令引起的返送信息
	    RETLOC			= 12,	// 当地命令引起的返送信息
	    INROGEN         = 20,	// 响应总召唤
        INRO1           = 21,   // 响应第01组召唤
        INRO2           = 22,   // 响应第02组召唤
        INRO3           = 23,   // 响应第03组召唤
        INRO4           = 24,   // 响应第04组召唤
        INRO5           = 25,   // 响应第05组召唤
        INRO6           = 26,   // 响应第06组召唤
        INRO7           = 27,   // 响应第07组召唤
        INRO8           = 28,   // 响应第08组召唤
        INRO9           = 29,   // 响应第09组召唤
        INRO10          = 30,   // 响应第10组召唤
        INRO11          = 31,   // 响应第11组召唤
        INRO12          = 32,   // 响应第12组召唤
        INRO13          = 33,   // 响应第13组召唤
        INRO14          = 34,   // 响应第14组召唤
        INRO15          = 35,   // 响应第15组召唤
        INRO16          = 36,   // 响应第16组召唤
	    REQCOGEN        = 37,	// 响应电度量总召唤
	    REQCO1         	= 38,	// 响应第1组电度量召唤
	    REQCO2         	= 39,	// 响应第2组电度量召唤
	    REQCO3         	= 40,	// 响应第3组电度量召唤
	    REQCO4         	= 41,	// 响应第4组电度量召唤
	    UNKNOWNTYPEID	= 44,	// 未知的类型标识
	    UNKNOWNCAUSE	= 45,	// 未知的传送原因
	    UNKNOWNPADDR	= 46,	// 未知的ASDU公共地址
	    UNKNOWNINFOADDR	= 47	// 未知的信息对象地址
    };
};

struct COI {
    enum type {
        LocalPwOn   = 0,
        LocalReset  = 1,
        RemoteReset = 2
    };
};

// QOI := 召唤限定词，UI[1~8]{0~255}----------------//
struct QOI {
    enum type {
	    Unused      = 1,	// 未使用
	    // <1~19>:= 为配套标准保留（兼容范围）
	    GroupAll	= 20,	// 站总召唤（14H）， 所有遥信、遥测、档位信息和远东终端状态
	    Group1		= 21,	// 召唤第1组、遥信信息（15H）
	    Group2		= 22,	// 召唤第2组、遥信信息（16H）
	    Group3		= 23,	// 召唤第3组、遥信信息（17H）
	    Group4		= 24,	// 召唤第4组、遥信信息（18H）
	    Group5		= 25,	// 召唤第5组、遥信信息（19H）
	    Group6		= 26,	// 召唤第6组、遥信信息（1AH）
	    Group7		= 27,	// 召唤第7组、遥信信息（1BH）
	    Group8		= 28,	// 召唤第8组、遥信信息（1CH）

	    Group9		= 29,	// 召唤第9组、遥测信息（1DH）
	    Group10		= 30,	// 召唤第10组、遥测信息（1EH）
	    Group11		= 31,	// 召唤第11组、遥测信息（1FH）
	    Group12		= 32,	// 召唤第12组、遥测信息（20H）
	    Group13		= 33,	// 召唤第13组、遥测信息（21H）
	    Group14		= 34,	// 召唤第14组、遥测信息（22H）
	    Group15		= 35,	// 召唤第15组、档位信息（23H）
	    Group16		= 36	// 召唤第16组、远动终端状态信息
	// <37~63>:= 为配套标准保留（兼容范围）
	// <64~255>:= 为特殊用途保留（专用范围）
     };
};
//---------------累计量召唤限定词------------------//
struct QCC {
	enum FRZ {
		Read		= 0,	// 读 (计数量)
		Freeze		= 1,	// 冻结不复位 (累计量)
		FrzAndRst	= 2,	// 冻结复位 (增量信息)
		Reset		= 3		// 复位
	};
	enum RQT {
		Group1		= 1,	// 召唤第1组
		Group2		= 2,	// 召唤第2组
		Group3		= 3,	// 召唤第3组
		Group4		= 4,	// 召唤第4组
		GroupAll	= 5		// 总召唤
	};
};

struct QPM {
    enum KPA {
        UNUSED     = 0,
        THRESHOLD  = 1,
        SMOOTH     = 2,
        LOWERLIMIT = 3,
        UPPERLIMIT = 4
    };
    enum LPC {
        NO_CHANGE  = 0,
        CHANGE     = 1
    };
    enum POP {
        RUN        = 0,
        NO_RUN     = 1
    };
};

struct QOC {
    enum QU {
        Undefined   = 0,
        SHORT_PULSE = 1,
        LONG_PULSE  = 2,
        HOLD_OUT    = 3
    };
    enum {
        EXEC        = 0,
        SELECT      = 1
    };
};

//        	1997版			2002版
//遥信	  	1H~400H			1H~4000H
//遥测		701H~900H		4001H~5000H
//遥控		B01H~B80H		6001H~6100H
//遥调		B81H~C00H		6201H~6400H
//遥脉		C01H~C80H		6401H~6600H
NAMESPACE_ENDED_IEC101

#endif // __IEC_60870_5_101_TYPES_H
