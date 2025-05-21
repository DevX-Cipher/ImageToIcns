#include <windows.h>
#include <gdiplus.h>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

#include "png.h"


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
    wprintf(L"Original image size: %u x %u\n", w, h);
    wprintf(L"First 8 pixels RGBA (hex): ");
    for (int i = 0; i < 8 && i < (int)(w * h); i++) {
        size_t idx = i * 4;
        wprintf(L"%02X %02X %02X %02X  ", out.pixels[idx], out.pixels[idx + 1], out.pixels[idx + 2], out.pixels[idx + 3]);
    }
    wprintf(L"\n");

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
    wprintf(L"Resized to %ux%u\n", w, h);
}


bool write_icns(const char* filename, const std::vector<PNGImage>& images) {
    struct {
        uint32_t size;
        char code[5];
    } mapping[] = {
        {16, "icp4"},
        {32, "icp5"},
        {64, "icp6"},
        {128,"ic07"},
        {256,"ic08"},
        {512,"ic09"},
        {1024,"ic10"}
    };

    std::vector<ICNSChunk> chunks;

    for (auto& m : mapping) {
        auto it = std::find_if(images.begin(), images.end(), [&](const PNGImage& img) {
            return img.width == m.size && img.height == m.size;
            });
        if (it == images.end()) {
            fprintf(stderr, "Missing icon size %u\n", m.size);
            return false;
        }

        const char* tmpname = "tmp_icns_icon.png";
        if (!write_simple_png(tmpname, *it)) {
            fprintf(stderr, "Failed to write temp PNG for size %u\n", m.size);
            return false;
        }

        FILE* f = nullptr;
        errno_t err = fopen_s(&f, tmpname, "rb");
        if (err || !f) return false;
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<uint8_t> pngdata(sz);
        fread(pngdata.data(), 1, sz, f);
        fclose(f);
        remove(tmpname);

        ICNSChunk c;
        memcpy(c.type, m.code, 4);
        c.data = std::move(pngdata);
        chunks.push_back(std::move(c));
    }

    uint32_t total_size = 8; // header
    for (auto& c : chunks) {
        total_size += 8 + (uint32_t)c.data.size();
    }

    FILE* f = nullptr;
    errno_t err = fopen_s(&f, filename, "wb");
    if (err || !f) return false;

    fwrite("icns", 1, 4, f);
    uint8_t buf[4];
    write_be_uint32(buf, total_size);
    fwrite(buf, 1, 4, f);

    for (auto& c : chunks) {
        fwrite(c.type, 1, 4, f);
        write_be_uint32(buf, (uint32_t)c.data.size() + 8);
        fwrite(buf, 1, 4, f);
        fwrite(c.data.data(), 1, c.data.size(), f);
    }

    fclose(f);
    return true;
}

std::string WideCharToUtf8(const wchar_t* wstr) {
    if (!wstr) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return {};
    std::string strTo(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

