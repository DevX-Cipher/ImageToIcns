#include <cstdint>

static uint32_t crc_table[256];

uint32_t crc(const uint8_t* buf, size_t len);
uint32_t update_crc(uint32_t crc, const uint8_t* buf, size_t len);
void make_crc_table();