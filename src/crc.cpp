#include "crc.h"

void make_crc_table() {
  for (uint32_t n = 0; n < 256; n++) {
    uint32_t c = n;
    for (int k = 0; k < 8; k++) {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
}

uint32_t update_crc(uint32_t crc, const uint8_t* buf, size_t len) {
  uint32_t c = crc;
  for (size_t n = 0; n < len; n++) {
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

uint32_t crc(const uint8_t* buf, size_t len) {
  return update_crc(0xffffffff, buf, len) ^ 0xffffffff;
}