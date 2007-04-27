#ifndef _TOARRAY_H__
#define _TOARRAY_H__

#include <cstddef> // for size_t




/* 
 * converts an integer into a byte array of the size of the basic type
 * The output format is in big endian(network byte order )
 */
template <typename T>  
void toArray(T d, char *array) {
    
    for (size_t i = 0; i < sizeof(T); ++i) {
	array[sizeof(T) - 1 - i] = (char)(unsigned char)d;
	d >>= 8;
    }
}

template <typename T> 
T fromArray( const char *array) {
    T d = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
	d <<= 8;
	d |= (T)(unsigned char)array[i];
    }
    return d;
}

/* define optimized version for i386 platform */
#if defined(__GNUC__) && defined(__i386)

#include <byteswap.h>

template <>
static unsigned short fromArray<unsigned short>(const char *array) {
    return bswap_16( *(unsigned short *)array );
}

template <>
static unsigned long fromArray<unsigned long>(const char *array) {
    return bswap_32( *(unsigned long *)array );
}


#endif








#endif //TOARRAY_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
