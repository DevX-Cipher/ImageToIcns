#ifndef DEBUG_H
#define DEBUG_H

#include <string>

void write_be_uint32(uint8_t* buf, uint32_t val);

#ifdef _DEBUG
void debug_log(const char* format, ...);
#else
inline void debug_log(const char* format, ...) {}
#endif

#endif // DEBUG_H