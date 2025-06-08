#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

using namespace Gdiplus;

struct PNGImage {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixels; // RGBA8 format
};


struct ICNSChunk {
    char type[4];
    std::vector<uint8_t> data;
};



void resize_nn(const PNGImage& src, PNGImage& dst, uint32_t w, uint32_t h);
bool load_png_gdiplus(const wchar_t* filename, PNGImage& out);
bool write_png_chunk(FILE* f, const char* type, const uint8_t* data, uint32_t len);
bool write_simple_png(const char* filename, const PNGImage& img);
void write_be_uint32(uint8_t* buf, uint32_t val);
std::string WideCharToUtf8(const wchar_t* wstr);
bool write_icns(const char* filename, const std::vector<PNGImage>& images);