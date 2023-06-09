
#ifndef __IEC_60870_5_101OR104_TYPES_H
#define __IEC_60870_5_101OR104_TYPES_H

/*
//----------------带品质描述的单点信息SIQ----------------//
SIQ := CP8 {SPI, RES, BL, SB, NT, IV}
SPI := 单点遥信信息状态，bit1；
	<0> := OFF 开
	<1> := ON 合

RES := 保留位置bit2~4，置零

BL := 封锁标志位，bit5；
	<0> := 未被封锁
	<1> := 被封锁
  信息体的值被闭锁后，为了传输需要，传输被封锁前的值，
  封锁和解锁可以由当地联锁机构或当地其他原因来启动。

SB := 取代标志位，bit6；
	<0> := 未被取代
	<1> := 被取代
  信息体的值被值班人员的输入值或由一个自动装置的输入所取代。

NT := 刷新标志位，bit7；
	<0> := 当前值
	<1> := 非当前值，表示本次采样刷新未成功
  若最近的刷新成功，则值就称为当前值。若在一个指定的时间间隔内刷新不成功或者值不可用，就称为非当前值。

IV := 有效标志位，bit8；
	<0> := 状态有效
	<1> := 状态无效
  若值被正确采集就是有效，在采集功能确认信息源的反常状态（丧失或非工作刷新）
  则值就是无效，信息体值在这些条件下没有被定义。标上无效用以提醒使用者，此值不正确而不能被使用。
*/


/*
//----------------带品质描述的双点信息DIQ----------------//
DIQ := CP8 {DPI, RES, BL, SB, NT, IV}
DPI := 双点信息信息状态，bit1~2；
	<0> := 中间状态或不确定
	<1> := 确定状态开（OFF）
	<2> := 确定状态合（ON）
	<3> := 中间状态或不确定
其它标志位与单点信息相同
*/

/*
//-------------------品质描述、遥测信息QDS---------------------//
QDS := CP8 {OV, RES, BL, SB, NT, IV}
OV := 溢出标志位，表示遥测值是否发生溢出，bit1；
	<> := 未溢出
	<> := 溢出
其它标志位与单点信息相同
*/

/*
//-------------------品质描述、双点遥控DCO---------------------//
DCO := CP8 {DCS, QOC}
DCS := 双点遥控状态，bit1~2；
	<0> := 不允许
	<1> := OFF，开
	<2> := ON，合
	<3> := 不允许

QOC := bit3~8 (QU, S/E)
QU := 遥控输出方式，bit3~7<0~31>
	<0> := 无定义， 可以用于被寻址的控制功能的属性（如脉冲持续时间等），
	这些属性在被控站事先定义而不由控制中心来选择
	<1> := 短脉冲持续时间（断路器），持续时间由远方终端系统参数决定
	<2> := 长脉冲持续时间，持续时间由远方终端系统参数决定
	<3> := 持续输出
	<4~8> := 为配套标准保留（兼容范围）
	<9~15> := 为其他预先定义功能保留，用于有固定属性的控制功能，这些属性在被控站事先定义
	<16~31> := 为特殊用途保留（专用范围）

S/E := bit8 <0~1>
	<0> := 执行
	<1> := 选择
*/

/*
//-------------------关于超时时间的理解------------------------------//
   t0规定了主站端和子站RTU端建立一次TCP连接的最大允许时间，主站端和子站
RTU端之间的TCP连接在实际运行中可能经常进行关闭和重建，这发生在4种情况下：
① 主站端和子站RTU端之间的I格式报文传送出现丢失、错序或者发送U格式报文得
不到应答时，双方均可主动关闭TCP连接，然后进行重建；
② 主站系统重新启动后将与各个子站重新建立TCP连接；
③ 子站RTU合上电源或由于自恢复而重新启动后，将重建连接；
④ 子站RTU收到主站端的RESET_PROCESS（复位远方终端）信号
后，将关闭连接并重新初始化，然后重建连接。每次建立连接时，RTU都调用
socket的listen( )函数进行侦听，主站端调用socket的connect( )函数进行连
接，如果在t0时间内未能成功建立连接，可能网络发生了故障，主站端应该向运
行人员给出警告信息。t1规定发送方发送一个I格式报文或U格式报文后，必须在
t1的时间内得到接收方的认可，否则发送方认为TCP连接出现问题并应重新建立连
接。t2规定接收方在接收到I格式报文后，若经过t2时间未再收到新的I格式报
文，则必须向发送方发送S格式帧对已经接收到的I格式报文进行认可，显然t2必
须小于t1。t3规定调度端或子站RTU端每接收一帧 I帧、S帧或者U帧将重新触发计
时器t3，若在t3内未接收到任何报文，将向对方发送测试链路帧TESTFR
*/

#include <stdint.h>

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

// CP56Time2a 时标
// 在IEC60870-5-101 7.2.6.18中定义
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

struct CP16Time2a{
    unsigned short msec;
};

struct iec_qualif
{
    unsigned char var :2;
    unsigned char res :2;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_SP_NA_1 - 单点信息(single point information with quality description)
struct iec_type1 {
    unsigned char spi :1; // single point information
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_DP_NA_1 - 双点信息(double point information with quality description)
struct iec_type3 {
    unsigned char dpi :2; // double point information
    unsigned char res :2;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_ST_NA_1 - 步位置信息(step position)
struct iec_type5 {
    unsigned char mv  :7; // value
    unsigned char t   :1; // transient flag
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_BO_NA_1 - 32比特串(state and change information bit string)
struct iec_type7 {
    struct iec_stcd stcd;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_ME_NA_1 - 测量值，规一化值(normalized measured value)
//归一化值
//　　NVA:F16[1...16]
//　　值得范围-1~1,将大于1的数映射到1以内的空间，通常就是用实际值除以额定值，即得到归一化的小数。具体表示法可以有F13或F16位的。２字节
struct iec_type9 {
    unsigned short mv;    // normalized value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_ME_NB_1 - 测量值，标度化值(scaled measured value)
struct iec_type11 {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// M_ME_NC_1 - 测量值，短浮点数(short floating point measured value)
struct iec_type13 {
    float mv;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

struct iec_type15 {
    int32_t mv;
    unsigned char sq :5;
    unsigned char cy :1;
    unsigned char ca :1;
    unsigned char iv :1;
};

// M_SP_TB_1 - 带时标CP56Time2a的单点信息(single point information with quality description and time tag)
struct iec_type30 {
    unsigned char spi :1; // single point information
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_DP_TB_1 - 带时标CP56Time2a的双点信息(double point information with quality description and time tag)
struct iec_type31 {
    unsigned char dp  :2; // double point information
    unsigned char res :2;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_ST_TB_1 - 带时标CP56Time2a的步位置信息(step position with time tag)
struct iec_type32 {
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

// M_BO_TB_1 - 带时标CP56Time2a的32比特串(state and change information bit string and time tag)
struct iec_type33 {
    struct iec_stcd stcd;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_ME_TD_1 - 带时标CP56Time2a的测量值，规一化值(scaled measured value with time tag)
struct iec_type34 {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_ME_TE_1 - 带时标CP56Time2a的测量值，标度化值(scaled measured value with time tag)
struct iec_type35 {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_ME_TF_1 - 带时标CP56Time2a的测量值，短浮点数(short floating point measurement value and time tag)
struct iec_type36 {
    float  mv;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// M_IT_TB_1 - 带时标CP56Time2a的累计量
struct iec_type37 {
    uint32_t bcr;
    unsigned char sq :5;
    unsigned char cy :1;
    unsigned char ca :1;
    unsigned char iv :1;
    CP56Time2a ts;
};

// C_SC_NA_1 - 单命令
struct iec_type45 {
    unsigned char scs :1; // single command state
    unsigned char res :1; // must be zero
    unsigned char qu :5;
    unsigned char se :1; // select=1 / execute=0
};

// C_DC_NA_1 - 双命令
struct iec_type46 {
    unsigned char dcs :2; // double command state
    unsigned char qu :5;
    unsigned char se :1; // select=1 / execute=0
};

// C_RC_NA_1 - 升降命令
struct iec_type47 {
    unsigned char rcs :2; // regulating step command
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
};

// C_SE_NA_1 - 设定值命令, 规一化值
struct iec_type48 {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// C_SE_NB_1 - 设定值命令, 标度化值
struct iec_type49 {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// C_SE_NC_1 - 设定值命令, 短浮点数
struct iec_type50 {
    float  mv;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
};

// C_SC_TA_1 - 带时标CP56Time2a的单命令
struct iec_type58 {
    unsigned char scs :1; // single command state
    unsigned char res :1; // must be zero
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
    CP56Time2a ts;
};

// C_DC_TA_1 - 带时标CP56Time2a的双命令
struct iec_type59 {
    unsigned char dcs :2; // double command state
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
    CP56Time2a ts;
};

// C_RC_NA_1 - 带时标CP56Time2a的升降命令
struct iec_type60 {
    unsigned char rcs :2; // regulating step command
    unsigned char qu  :5;
    unsigned char se  :1; // select=1 / execute=0
    CP56Time2a ts;
};

// C_SE_TA_1 - 带CP56Time2a时标的设定值命令, 规一化值
struct iec_type61 {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// C_SE_TB_1 - 带CP56Time2a时标的设定值命令, 标度化值
struct iec_type62 {
    unsigned short mv;    // scaled value
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// C_SE_TC_1 - 带CP56Time2a时标的设定值命令, 短浮点数
struct iec_type63 {
    float  mv;
    unsigned char ov  :1; // overflow/no overflow
    unsigned char res :3;
    unsigned char bl  :1; // blocked/not blocked
    unsigned char sb  :1; // substituted/not substituted
    unsigned char nt  :1; // not topical/topical
    unsigned char iv  :1; // valid/invalid
    CP56Time2a ts;
};

// C_IC_NA_1 - 总召唤命令
struct iec_type100 {
    unsigned char qoi; // pointer of interrogation
};

// C_CI_NA_1 - 电能脉冲召唤命令
struct iec_type101 {
    unsigned char rqt :6; // request
    unsigned char frz :2; // freeze
};

// C_CS_NA_1 - 时钟同步命令
struct iec_type103 {
    CP56Time2a ts;
};

// C_TS_NA_1 - 带时标CP56Time2a的测试命令
struct iec_type107 {
    unsigned short tsc; // TSC test sequence counter
    CP56Time2a ts;
};
struct TAGTYPE{
    enum {
        BOOLTAG = 1,
        UINTTAG = 37,
        FLOATTAG = 38
    };
};

struct iec_type200{
    unsigned short curArea;
    unsigned short minArea;
    unsigned short maxArea;
};

struct iec_type202 {
    unsigned short ioaddr16l;
    unsigned char ioaddr8h;
    unsigned char tag;
    unsigned char datalen;
    union{
        bool            bvalue;
        unsigned int    ui32value;
        float           fvalue;
    };
};

// 数据单元标识符 - ASDU header
struct iec104_data_unit_id {
    unsigned char type;  	// 类型标识
    unsigned char num :7;	// 信息对象数目
    unsigned char sq :1; 	// 顺序：1，非顺序：0
    unsigned char cause :6; // 传送原因
    unsigned char pn :1; 	// 否定确认：1，肯定确认：0
    unsigned char t :1; 	// 试验：1，未试验：0
    unsigned char oaddr; 	// 源地址，未使用
    unsigned short paddr;   // ASDU 公共地址
};

struct iec104_apci {
	unsigned char startChr;		// 启动字符
	unsigned char length;		// APDU长度; ASDU的最大长度限制在249以内，因为APDU域的最大长度是253（APDU最大值=255-启动字符-长度域）
    unsigned short NS;
	unsigned short NR;
};

// APDU 应用规约数据单元
struct iec104_apdu {
	struct iec104_apci apci;	// APCI 应用规约控制信息
    struct iec104_data_unit_id asduh;
    union {
        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type1 obj[1];
        } sq1;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type1 obj;
        } nsq1[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type3 obj[1];
        } sq3;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type3 obj;
        } nsq3[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type5 obj[1];
        } sq5;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type5 obj;
        } nsq5[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type9 obj[1];
        } sq9;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type9 obj;
        } nsq9[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type11 obj[1];
        } sq11;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type11 obj;
        } nsq11[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type13 obj[1];
        } sq13;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type13 obj;
        } nsq13[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type15 obj[1];
        } sq15;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type15 obj;
        } nsq15[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type30 obj[1];
        } sq30;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type30 obj;
        } nsq30[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type31 obj[1];
        } sq31;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type31 obj;
        } nsq31[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type32 obj[1];
        } sq32;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type32 obj;
        } nsq32[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type34 obj[1];
        } sq34;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type34 obj;
        } nsq34[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type35 obj[1];
        } sq35;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type35 obj;
        } nsq35[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type36 obj[1];
        } sq36;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type36 obj;
        } nsq36[1];

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type45 obj;
        } nsq45;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type46 obj;
        } nsq46;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type47 obj;
        } nsq47;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type48 obj;
        } nsq48;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type49 obj;
        } nsq49;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type50 obj;
        } nsq50;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type58 obj;
        } nsq58;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type59 obj;
        } nsq59;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type60 obj;
        } nsq60;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type61 obj;
        } nsq61;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type62 obj;
        } nsq62;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type63 obj;
        } nsq63;

		struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type100 obj;
        } nsq100;

		struct {
			unsigned short ioaddr16l;
			unsigned char ioaddr8h;
			iec_type101 obj;
		} nsq101;

        struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type103 obj;
        } nsq103;

		struct {
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type107 obj;
        } nsq107;

        struct{
            unsigned short ioaddr16l;
            unsigned char ioaddr8h;
            iec_type200 obj;
        }nsq200;

        struct {
            unsigned short area;
            unsigned char CONT:1;
            unsigned char RES:5;
            unsigned char CR:1;
            unsigned char SE:1;
            iec_type202 obj[1];
        }nsq202;

        unsigned char dados[243];
    };
};

#pragma pack(pop)		// 恢复默认内存对齐


//---------------类型标识------------------//
typedef enum _tag_iec_type_id {

	// 在监视方向的过程信息
	Undefined,			// 未定义
	M_SP_NA_1 = 1,  	// 单点信息
	M_SP_TA_1 = 2,		//带时标的单点信息
	M_DP_NA_1 = 3,		// 双点信息
	M_DP_TA_1 = 4,		//带时标的双点信息
	M_ST_NA_1 = 5,		// 步位置信息
    M_ST_TA_1 = 6,      // 带时标的步位置信息
	M_BO_NA_1 = 7,		// 32比特串
    M_BO_TA_1 = 8,      // 带时标的32比特串
	M_ME_NA_1 = 9,		// 测量值，规一化值
    M_ME_TA_1 = 10,     // 带时标的测量值, 规一化值
	M_ME_NB_1 = 11,		// 测量值，标度化值
    M_ME_TB_1 = 12,     // 测量值, 带时标的标度化值
	M_ME_NC_1 = 13,		// 测量值，短浮点数
    M_ME_TC_1 = 14,     // 测量值, 带时标的短浮点数
	M_IT_NA_1 = 15,		// 累计量
    M_IT_TA_1 = 16,     // 带时标的累积量
    M_EP_TA_1 = 17,     // 带时标的继电保护设备事件
    M_EP_TB_1 = 18,     // 带时标的继电保护设备成组启动事件
    M_EP_TC_1 = 19,     // 带时标的继电保护设备成组输出电路信息
	M_PS_NA_1 = 20,		// 带状态检出的成组单点信息
	M_ME_ND_1 = 21,		// 不带品质描述的规一化测量值

	M_SP_TB_1 = 30,		// 带时标CP56Time2a的单点信息
	M_DP_TB_1 = 31,		// 带时标CP56Time2a的双点信息
	M_ST_TB_1 = 32,		// 带时标CP56Time2a的步位置信息
	M_BO_TB_1 = 33,		// 带时标CP56Time2a的32比特串
	M_ME_TD_1 = 34,		// 带时标CP56Time2a的测量值，规一化值
	M_ME_TE_1 = 35,		// 带时标CP56Time2a的测量值，标度化值
	M_ME_TF_1 = 36,		// 带时标CP56Time2a的测量值，短浮点数
	M_IT_TB_1 = 37,		// 带时标CP56Time2a的累计量
	M_EP_TD_1 = 38,		// 带时标CP56Time2a的继电保护装置事件
	M_EP_TE_1 = 39,		// 带时标CP56Time2a的继电保护装置成组启动事件
	M_EP_TF_1 = 40,		// 带时标CP56Time2a的继电保护装置成组出口信息

	// 在控制方向的过程信息
	C_SC_NA_1 = 45,		// 单命令
	C_DC_NA_1 = 46,		// 双命令
	C_RC_NA_1 = 47,		// 升降命令
	C_SE_NA_1 = 48,		// 设点命令，规一化值
	C_SE_NB_1 = 49,		// 设点命令，标度化值
	C_SE_NC_1 = 50,		// 设点命令，短浮点数
	C_BO_NA_1 = 51,		// 32比特串

	C_SC_TA_1 = 58,		// 带时标CP56Time2a的单命令
	C_DC_TA_1 = 59,		// 带时标CP56Time2a的双命令
	C_RC_TA_1 = 60,		// 带时标CP56Time2a的升降命令
	C_SE_TA_1 = 61,		// 带时标CP56Time2a的设点命令，规一化值
	C_SE_TB_1 = 62,		// 带时标CP56Time2a的设点命令，标度化值
	C_SE_TC_1 = 63,		// 带时标CP56Time2a的设点命令，短浮点数
	C_BO_TA_1 = 64,		// 带时标CP56Time2a的32比特串

	// 在监视方向的系统信息
	M_EI_NA_1 = 70,		// 初始化结束

	// 在控制方向的系统信息
	C_IC_NA_1 = 100,	// 总召唤命令
	C_CI_NA_1 = 101,	// 电能脉冲召唤命令
	C_RD_NA_1 = 102,	// 读命令
	C_CS_NA_1 = 103,	// 时钟同步命令
	C_RP_NA_1 = 105,	// 复位进程命令
	C_TS_TA_1 = 107,	// 带时标CP56Time2a的测试命令

	// 在控制方向的参数
	P_ME_NA_1 = 110,	// 测量值参数，规一化值
	P_ME_NB_1 = 111,	// 测量值参数，标度化值
	P_ME_NC_1 = 112,	// 测量值参数，短浮点数
	P_AC_NA_1 = 113,	// 参数激活

	// 文件传输
	F_FR_NA_1 = 120,	// 文件已准备好
	F_SR_NA_1 = 121,	// 节已准备好
	F_SC_NA_1 = 122,	// 召唤目录，选择文件，召唤文件，召唤节
	F_LS_NA_1 = 123,	// 最后的节，最后的段
	F_AF_NA_1 = 124,	// 确认文件，确认节
	F_SG_NA_1 = 125,	// 段
	F_DR_NA_1 = 126,	// 目录

    C_SR_NA_1 = 200,    //切换定值区
    C_RR_NA_1 = 201,    //读定值区号
	C_RS_NA_1 = 202,    //读参数和定值
	C_WS_NA_1 = 203,    //写参数和定值
	Reserve
} iec_type_id;

//---------------传送原因------------------//
struct IecCause {
    enum {
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

// QOI := 召唤限定词，UI[1~8]{0~255}----------------//
struct IEC_QOI {
    enum {
	    Unused = 1,			// 未使用
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
struct IEC_QCC {
	enum FRZ {
		Read		= 0,	// 读
		Freeze		= 1,	// 冻结不复位
		FrzAndRst	= 2,	// 冻结复位
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

//        	1997版			2002版
//遥信	  	1H~400H			1H~4000H
//遥测		701H~900H		4001H~5000H
//遥控		B01H~B80H		6001H~6100H
//遥调		B81H~C00H		6201H~6400H
//遥脉		C01H~C80H		6401H~6600H

#endif // __IEC_60870_5_101OR104_TYPES_H
