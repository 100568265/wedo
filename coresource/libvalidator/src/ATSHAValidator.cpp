
#include "ATSHAValidator.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <err.h>
#include <sys/ioctl.h>

#include <sys/sysctl.h>
#include <time.h>
#include <sys/time.h>
#include "cryptodev.h"

#include "rijndael.h"
#include "tea_crypto.h"


/*char serno[32]  = {0};
char myKey[32] = {
    0x01,0x23,0x44,0xa0,0x0f,0x85,0xb5,0xc8,
	0xee,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x10,0x09,0x09,0x0c,0x14,0x00,
	0x00,0x14,0x5b,0x00,0x00,0x00,0x00,0x03
};*/

// #define IO_SN_READ 		0x1976
// #define IO_KEY_WRITE 	0x4051
#define IO_MAC 			0x9811


static const uint16_t crc16_table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t get_crc16 (const uint8_t *pdata, int nsize)
{
	uint16_t crc = 0xFFFF;
	while (nsize --> 0)
		crc = crc16_table[(crc & 0xFF) ^ (*pdata++)] ^ (crc >> 8);
	return crc;
}

inline static int read_token (uint8_t ret[32])
{
	FILE * fp = fopen("/lib/libtoken.so.1", "rb");
	if (!fp)
		return -1;

	if (fread(ret, 32, 1, fp) != 1) {
		fclose(fp);
		return -2;
	}
	fclose(fp);
	return 0;
}

inline static int devcrypto(void)
{
	static int fd = -1;

	if (fd < 0) {
		fd = open("/dev/crypto", O_RDWR, 0);
		if (fd < 0)
			return -1;
		if (fcntl(fd, F_SETFD, 1) == -1)
			return -1;
	}
	return fd;
}

inline static int crget(void)
{
	int fd;

	if (ioctl(devcrypto(), CRIOGET, &fd) == -1)
		return -1;
	if (fcntl(fd, F_SETFD, 1) == -1)
		return -1;
	return fd;
}

inline int getrandom(char *rand)
{
	int rfd = open("/dev/hw_random", O_RDONLY);
	int len = 0;
	char *data = rand;
	if(rfd < 0)
		return -1;

	while (len < 32) {
    	int result = read(rfd, data + len, 32 - len);
    	if (result < 0) {
    		close(rfd);
    		return -1;
    	}
    	len += result;
	}
	close(rfd);
	return 0;
}
/*
inline int MakeAuthMAC(char *rand, char *code)
{
		bzero(code,88);
		memcpy(code,myKey,32);
		memcpy(code+32,rand,32);
		code[64] = 0x8;
		code[65] = 0x40;
		code[79] = serno[8];
		code[80] = serno[4];
		code[81] = serno[5];
		code[82] = serno[6];
		code[83] = serno[7];
		code[84] = serno[0];
		code[85] = serno[1];
		code[86] = serno[2];
		code[87] = serno[3];

		return 0;
}
*/
/*int getchallenge(char *rand, char *mac)
{
		int cryptfd = crget();
		char text[88];
		struct session2_op sop;
		struct crypt_op cop;
		if(MakeAuthMAC(rand,text)) return -1;
		bzero(&sop, sizeof(sop));
		sop.mac = CRYPTO_SHA2_256;
		sop.crid = CRYPTO_FLAG_HARDWARE; // | CRYPTO_FLAG_SOFTWARE;//
		if (ioctl(cryptfd, CIOCGSESSION, &sop) < 0) {
				close(cryptfd);
			return -1;
		}

		cop.ses = sop.ses;
		cop.op = 0;
		cop.flags = 0;
		cop.len = sizeof(text);
		cop.src = text;
		cop.dst = 0;
		cop.mac = mac;
		cop.iv = 0;

		if (ioctl(cryptfd, CIOCCRYPT, &cop) < 0){
				close(cryptfd);
				return -1;
		}
		return 0;
}
*/

inline int MakeAuthMAC(char *code, const char *key, const char *sn, const char *rand)
{
		memcpy(code, key, 32);
		memcpy(code + 32, rand, 32);
		code[64] = 0x08;
		code[65] = 0x40;
		code[79] = sn[8];
		code[80] = sn[4];
		code[81] = sn[5];
		code[82] = sn[6];
		code[83] = sn[7];
		code[84] = sn[0];
		code[85] = sn[1];
		code[86] = sn[2];
		code[87] = sn[3];

		return 0;
}

int getchallenge(char *text, int length, char *mac)
{
		int cryptfd = crget();
		struct session2_op sop;
		struct crypt_op cop;
		bzero(&sop, sizeof(sop));
		sop.mac = CRYPTO_SHA2_256;
		sop.crid = CRYPTO_FLAG_HARDWARE; // | CRYPTO_FLAG_SOFTWARE;//
		if (ioctl(cryptfd, CIOCGSESSION, &sop) < 0) {
			close(cryptfd);
			return -1;
		}

		cop.ses = sop.ses;
		cop.op = 0;
		cop.flags = 0;
		cop.len = length;
		cop.src = text;
		cop.dst = 0;
		cop.mac = mac;
		cop.iv = 0;

		if (ioctl(cryptfd, CIOCCRYPT, &cop) < 0){
			close(cryptfd);
			return -1;
		}
		close(cryptfd);
		return 0;
}

struct auth_mac
{
		char chl[32];
		char rsp[32];
};

inline int macvalid(int fd, char *chl, char *rsp)
{
		int res;
		struct auth_mac amac;
		memcpy(amac.chl,chl,32);
		bzero(amac.rsp,32);
		res = ioctl(fd,IO_MAC,&amac);
		if(res) {
				return -1;
		}
		memcpy(rsp,amac.rsp,32);
		return 0;
}

inline void spreads (const uint8_t * src, uint8_t * dest, int srclen)
{
    while (srclen --> 0) {
        dest[2*srclen+0] = src[srclen] / 16 + 16;
        dest[2*srclen+1] = src[srclen] % 16;
    }
}

void deacidize (const uint8_t ciphertext[32], uint8_t data[16])
{
    unsigned char index_first[4] = {0};
    spreads (ciphertext, index_first, 2);

    for (int i = 0; i < sizeof(index_first); ++i)
        index_first[i] = ciphertext[index_first[i]];

    unsigned char index_next [8] = {0};
    spreads (index_first, index_next, sizeof(index_first));

    for (int i = 0; i < sizeof(index_next); ++i)
        index_next[i] = ciphertext[index_next[i]];

    unsigned char index_end [16] = {0};
    spreads (index_next , index_end , sizeof(index_next) );

    for (int i = 0; i < 16; ++i)
        data[i] = ciphertext[index_end[i]];
}

ATSHAValidator::ATSHAValidator()
{
}

ATSHAValidator::~ATSHAValidator()
{
}

std::string ATSHAValidator::Auth ()
{
/*	int shafd = open("/dev/atsha0", O_RDWR);
	if (shafd < 0) return false;

	int res = ioctl(shafd, IO_SN_READ, serno);
	if (res) {
		close (shafd);
		return false;
	}
	if (memcmp(myKey, serno, 9))
        return false;

	char random[32];
	getrandom(random);

	char mac[32];
	if(getchallenge(random, mac)) {
			close(shafd);
			return false;
	}
	char rsp[32];
	if(macvalid(shafd, random, rsp)) {
			close(shafd);
			return false;
	}
	close(shafd);
	return !memcmp(mac, rsp, 32);*/

	uint8_t token[32] = {0};
	if (read_token (token))
		return "KeyFail";

	int shafd = open("/dev/atsha0", O_RDWR);
	if (shafd < 0)
		return "OpenError";

	char random[32];
	getrandom(random);

	unsigned long rk[RIJNDAEL_RK_LENGTH(256)];

	unsigned char key[32] = {
		0x57, 0x45, 0x7C, 0x00, 0x79, 0x6F, 0x6E, 0x67,
		0x36, 0x42, 0x49, 0x47, 0x7C, 0x78, 0x69, 0x6E,
		0x66, 0x75, 0x77, 0x75, 0x45, 0x52, 0x7C, 0x00,
		0x5A, 0x48, 0x57, 0x45, 0x44, 0x4F, 0x55, 0x49
	};

	tea_decrypt_ex ((uint32_t*)key, sizeof(key) / sizeof(unsigned long));
	int nrounds = rijndaelSetupDecrypt (rk, key, 256);

	unsigned char plaintext[32] = {0};
	rijndaelDecryptEx   (rk, nrounds, token, plaintext);
	unsigned char text_bak [32] = {0};
	memcpy(text_bak, plaintext, sizeof(text_bak));

	tea_decrypt_ex ((uint32_t*)plaintext, sizeof(plaintext) / sizeof(unsigned long));

	unsigned char sn[16] = {0};
	deacidize (plaintext, sn);

	char code[88] = {0};
//	tea_decrypt_ex ((uint32_t*)sn, sizeof(sn) / sizeof(unsigned long));
	uint16_t check = get_crc16 (plaintext, 30);
	if (memcmp(plaintext + 30, &check, sizeof(check)))
		return "CheckError";

	MakeAuthMAC (code, (char*)text_bak, (char*)sn, random);

	char mac [32] = {0};
	if(getchallenge(code, sizeof(code), mac))
	{
			close(shafd);
			return "GetFail";
	}
	char rsp [32] = {0};
	if(macvalid(shafd, random, rsp)) {
			close(shafd);
			return "GetError";
	}
	close(shafd);

	return (memcmp(mac, rsp, 32) ? "Fail": "Success");
}
