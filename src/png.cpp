#include <windows.h>
#include <gdiplus.h>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

#include "png.h"
#include <crc.h>


void write_be_uint32(uint8_t* buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}


bool write_simple_png(const char* filename, const PNGImage& img) {
    // Save using GDI+
    CLSID pngClsid;
    UINT num, size;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return false;
    std::vector<BYTE> buffer(size);
    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)buffer.data();
    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pImageCodecInfo[i].MimeType, L"image/png") == 0) {
            pngClsid = pImageCodecInfo[i].Clsid;
            break;
        }
    }

    Bitmap bmp(img.width, img.height, PixelFormat32bppARGB);
    BitmapData bmpData;
    Rect rect(0, 0, img.width, img.height);
    bmp.LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &bmpData);
    BYTE* pDst = (BYTE*)bmpData.Scan0;

    for (uint32_t y = 0; y < img.height; ++y) {
        for (uint32_t x = 0; x < img.width; ++x) {
            size_t idx = (y * img.width + x) * 4;
            // GDI+ expects BGRA ordering
            pDst[y * bmpData.Stride + x * 4 + 0] = img.pixels[idx + 2]; // B
            pDst[y * bmpData.Stride + x * 4 + 1] = img.pixels[idx + 1]; // G
            pDst[y * bmpData.Stride + x * 4 + 2] = img.pixels[idx + 0]; // R
            pDst[y * bmpData.Stride + x * 4 + 3] = img.pixels[idx + 3]; // A
        }
    }

    bmp.UnlockBits(&bmpData);
    Status status = bmp.Save(std::wstring(filename, filename + strlen(filename)).c_str(), &pngClsid, nullptr);
    return status == Ok;
}


bool write_png_chunk(FILE* f, const char* type, const uint8_t* data, uint32_t len) {
    uint8_t len_buf[4];
    write_be_uint32(len_buf, len);
    if (fwrite(len_buf, 1, 4, f) != 4) return false;
    if (fwrite(type, 1, 4, f) != 4) return false;
    if (len && fwrite(data, 1, len, f) != len) return false;

    uint32_t c = crc((const uint8_t*)type, 4);
    if (len) c = update_crc(c, data, len);
    uint8_t crc_buf[4];
    write_be_uint32(crc_buf, c);
    if (fwrite(crc_buf, 1, 4, f) != 4) return false;

    return true;
}

bool load_png_gdiplus(const wchar_t* filename, PNGImage& out) {
    Bitmap bitmap(filename);
    if (bitmap.GetLastStatus() != Ok) {
        return false;
    }

    UINT w = bitmap.GetWidth();
    UINT h = bitmap.GetHeight();

    out.width = w;
    out.height = h;
    out.pixels.resize(w * h * 4);

    for (UINT y = 0; y < h; y++) {
        for (UINT x = 0; x < w; x++) {
            Color c;
            bitmap.GetPixel(x, y, &c);
            BYTE a = c.GetA();
            BYTE r = c.GetR();
            BYTE g = c.GetG();
            BYTE b = c.GetB();

            size_t idx = (y * w + x) * 4;
            out.pixels[idx + 0] = r;
            out.pixels[idx + 1] = g;
            out.pixels[idx + 2] = b;
            out.pixels[idx + 3] = a;  // keep transparency
        }
    }


    // Debug print first 8 pixels RGBA
#ifdef _DEBUG
    wprintf(L"Original image size: %u x %u\n", w, h);
    wprintf(L"First 8 pixels RGBA (hex): ");

    for (int i = 0; i < 8 && i < (int)(w * h); i++) {
        size_t idx = i * 4;
        wprintf(L"%02X %02X %02X %02X  ", out.pixels[idx], out.pixels[idx + 1], out.pixels[idx + 2], out.pixels[idx + 3]);
    }
    wprintf(L"\n");
#endif // DEBUG
    return true;
}

void resize_nn(const PNGImage& src, PNGImage& dst, uint32_t w, uint32_t h) {
    dst.width = w;
    dst.height = h;
    dst.pixels.resize(w * h * 4);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t sx = x * src.width / w;
            uint32_t sy = y * src.height / h;
            size_t sidx = (sy * src.width + sx) * 4;
            size_t didx = (y * w + x) * 4;
            dst.pixels[didx + 0] = src.pixels[sidx + 0];
            dst.pixels[didx + 1] = src.pixels[sidx + 1];
            dst.pixels[didx + 2] = src.pixels[sidx + 2];
            dst.pixels[didx + 3] = src.pixels[sidx + 3];
        }
    }
#ifdef DEBUG

    wprintf(L"Resized to %ux%u\n", w, h);
#endif // DEBUG
}




std::string WideCharToUtf8(const wchar_t* wstr) {
    if (!wstr) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return {};
    std::string strTo(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

