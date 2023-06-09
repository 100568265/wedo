//
//  AES_.h
//  Encryption
//
//  Created by Jason.xu on 13-10-22.
//  Copyright (c) 2013年 zhoumin. All rights reserved.
//

#ifndef Encryption_AES__h
#define Encryption_AES__h


typedef struct
{
    unsigned long erk[64];     /*!< encryption round keys */
    unsigned long drk[64];     /*!< decryption round keys */
    int nr;                    /*!< number of rounds      */
}
aes_context;
typedef struct
{
	char* content;
	int len;
}
aes_text;
/**
 * \brief          AES key schedule
 *
 * \param ctx      AES context to be initialized
 * \param key      the secret key
 * \param keysize  must be 128, 192 or 256
 */
void aes_set_key( aes_context *ctx, unsigned char *key, int keysize );

/**
 * \brief          AES block encryption (ECB mode)
 *
 * \param ctx      AES context
 * \param input    plaintext  block
 * \param output   ciphertext block
 */
void aes_encrypt( aes_context *ctx,
                 unsigned char input[16],
                 unsigned char output[16] );

/**
 * \brief          AES block decryption (ECB mode)
 *
 * \param ctx      AES context
 * \param input    ciphertext block
 * \param output   plaintext  block
 */
void aes_decrypt( aes_context *ctx,
                 unsigned char input[16],
                 unsigned char output[16] );

/**
 * \brief          AES-CBC buffer encryption
 *
 * \param ctx      AES context
 * \param iv       initialization vector (modified after use)
 * \param input    buffer holding the plaintext
 * \param output   buffer holding the ciphertext
 * \param len      length of the data to be encrypted
 */
void aes_cbc_encrypt( aes_context *ctx,
                     unsigned char iv[16],
                     unsigned char *input,
                     unsigned char *output,
                     int len );

/**
 * \brief          AES-CBC buffer decryption
 *
 * \param ctx      AES context
 * \param iv       initialization vector (modified after use)
 * \param input    buffer holding the ciphertext
 * \param output   buffer holding the plaintext
 * \param len      length of the data to be decrypted
 */
void aes_cbc_decrypt( aes_context *ctx,
                     unsigned char iv[16],
                     unsigned char *input,
                     unsigned char *output,
                     int len );
int aes_encrypt_Char(char *key,aes_text* content,aes_text* out_content);
int aes_encrypt_Hex(char *key,aes_text* content,aes_text* out_content);
int aes_decrypt_Char(char *key,aes_text* content,aes_text* out_content);
int aes_decrypt_Hex(char *key,aes_text* content,aes_text* out_content);

int hexToUCharLen(const char *hex, int len,unsigned char *uch);
int hexToUChar(const char *hex, unsigned char *uch);
int ucharToHexLen(const unsigned char *uch,int len, char *hex);
int ucharToHex(const unsigned char *uch, char *hex);
char valueToHexCh(const int value);
int ascillToValue(const char ch);
int free_aes_text(aes_text* content);
/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int aes_self_test( void );




#endif
