#include "util.h"

namespace util {
    unsigned short toShort(uchar* bytes) {
        return bytes[0] | (bytes[1] << 8);
    }
    
    unsigned int toInt(uchar* bytes) {
        return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
    }
    void toByte(unsigned short v, uchar* bytes) {
        bytes[0] = v & 0xFF;
        bytes[1] = (v >> 8) & 0xFF;
    }
    unsigned short toShort(uchar b1, uchar b2) {
        return b1 | (b2 << 8);
    }
    void toByte(unsigned int v, uchar* bytes) {
        bytes[0] = v & 0xFF;
        bytes[1] = (v >> 8) & 0xFF;
        bytes[2] = (v >> 16) & 0xFF;
        bytes[3] = (v >> 25) & 0xFF;
    }
}
