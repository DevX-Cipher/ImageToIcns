#include <vector>
#include <cstdint>
#include "resize.h"

// Resize image using nearest neighbor interpolation
std::vector<Pixel> resize_image(const std::vector<Pixel>& src, int src_w, int src_h, int dst_w, int dst_h) {
  std::vector<Pixel> dst(dst_w * dst_h);

  float scale_x = static_cast<float>(src_w) / dst_w;
  float scale_y = static_cast<float>(src_h) / dst_h;

  for (int y = 0; y < dst_h; ++y) {
    for (int x = 0; x < dst_w; ++x) {
      int src_x = static_cast<int>(x * scale_x);
      int src_y = static_cast<int>(y * scale_y);
      Pixel p = src[src_y * src_w + src_x];

      // Optional: fix fully transparent pixels alpha
      if (p.a == 0) p.a = 255;

      dst[y * dst_w + x] = p;
    }
  }

  return dst;
}

// Flatten transparency on white background
void flatten_to_white(std::vector<Pixel>& pixels) {
  for (auto& p : pixels) {
    float alpha = p.a / 255.0f;
    p.r = static_cast<uint8_t>(p.r * alpha + 255 * (1.0f - alpha));
    p.g = static_cast<uint8_t>(p.g * alpha + 255 * (1.0f - alpha));
    p.b = static_cast<uint8_t>(p.b * alpha + 255 * (1.0f - alpha));
    p.a = 255;
  }
}

// Helper: convert vector of Pixels to raw RGBA bytes
std::vector<uint8_t> flatten_pixels(const std::vector<Pixel>& pixels) {
  std::vector<uint8_t> raw;
  raw.reserve(pixels.size() * 4); // 4 bytes per pixel: RGBA

  for (const auto& p : pixels) {
    raw.push_back(p.r);
    raw.push_back(p.g);
    raw.push_back(p.b);
    raw.push_back(p.a);
  }

  return raw;
}
