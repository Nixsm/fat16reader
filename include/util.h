#ifndef FAT16_UTIL_H__
#define FAT16_UTIL_H__

typedef unsigned char uchar;

/*
 * Expects little endian bytes
 */
namespace util {
    unsigned short toShort(uchar* bytes);
    unsigned short toShort(uchar b1, uchar b2);
    void toByte(unsigned short v, uchar* bytes);
    unsigned int toInt(uchar* bytes);
    void toByte(unsigned int v, uchar* bytes);
}

#endif//FAT16_UTIL_H__
