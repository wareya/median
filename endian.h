#include <stdint.h>

inline uint32_t swap32(uint32_t swapme)
{
    return (((swapme & 0xFF000000) >> 24)
	   |((swapme & 0x00FF0000) >>  8)
	   |((swapme & 0x0000FF00) <<  8)
	   |((swapme & 0x000000FF) << 24));
}
inline uint16_t swap16(uint16_t swapme)
{
    return (((swapme & 0xFF00) >>  8)
	   |((swapme & 0x00FF) <<  8));
}
