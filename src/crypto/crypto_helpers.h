#ifndef _CRYPTO_HELPERS_H__
#define _CRYPTO_HELPERS_H__

#include <cstddef>

bool readKeyFromFile(const char* filename, char newkey[16]);

void getRandomByte(void *dest, size_t num);


#endif //CRYPTO_HELPERS_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
