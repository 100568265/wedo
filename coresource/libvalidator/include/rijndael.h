
#ifndef H__RIJNDAEL
#define H__RIJNDAEL

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int rijndaelSetupEncrypt (unsigned long *rk, const unsigned char *key, int keybits);
int rijndaelSetupDecrypt (unsigned long *rk, const unsigned char *key, int keybits);

void rijndaelEncrypt     (const unsigned long * rk, int nrounds,
				const unsigned char plaintext[16], unsigned char ciphertext[16]);
void rijndaelDecrypt     (const unsigned long * rk, int nrounds,
				const unsigned char ciphertext[16], unsigned char plaintext[16]);

void rijndaelEncryptEx   (const unsigned long * rk, int nrounds,
                const unsigned char plaintext[32], unsigned char ciphertext[32]);
void rijndaelDecryptEx   (const unsigned long * rk, int nrounds,
                const unsigned char ciphertext[32], unsigned char plaintext[32]);

#define RIJNDAEL_KEY_LENGTH(keybits) ((keybits) / 8      )
#define RIJNDAEL_RK_LENGTH(keybits)  ((keybits) / 8  + 28)
#define RIJNDAEL_NROUNDS(keybits)    ((keybits) / 32 +  6)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
