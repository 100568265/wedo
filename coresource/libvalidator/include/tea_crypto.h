
#ifndef _TEA_CRYPTO_H_
#define _TEA_CRYPTO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void tea_encrypt    (uint32_t*v, const uint32_t*k);
void tea_decrypt    (uint32_t*v, const uint32_t*k);

void tea_encrypt_ex (uint32_t*v, int len);
void tea_decrypt_ex (uint32_t*v, int len);

#ifdef __cplusplus
}
#endif

#endif // _TEA_CRYPTO_H_