#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <resize.h>

struct PNGImage {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<Pixel> pixels;
};

struct ICNSChunk {
  char type[4];
  std::vector<uint8_t> data;
};


static void write_chunk(std::ostream& out, const char* type, const std::vector<uint8_t>& data);
bool write_png(const std::string& filename, const std::vector<Pixel>& pixels, int width, int height);
bool load_simple_png(const std::string& filename, PNGImage& out);
void resize_nn(const PNGImage& src, PNGImage& dst, uint32_t w, uint32_t h);
std::string WideCharToUtf8(const wchar_t* wstr);
