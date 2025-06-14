#pragma once
#include <cstdint>
#include <vector>

struct Pixel {
  uint8_t r, g, b, a;
};

std::vector<Pixel> resize_image(const std::vector<Pixel>& src, int src_w, int src_h, int dst_w, int dst_h);
void flatten_to_white(std::vector<Pixel>& pixels);
