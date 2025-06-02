#include "jpg.h"

bool load_jpeg_gdiplus(const wchar_t* filename, PNGImage& out) {
    Gdiplus::Bitmap bitmap(filename);
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        return false;
    }

    UINT w = bitmap.GetWidth();
    UINT h = bitmap.GetHeight();

    out.width = w;
    out.height = h;
    out.pixels.resize(w * h * 4);

    for (UINT y = 0; y < h; y++) {
        for (UINT x = 0; x < w; x++) {
            Gdiplus::Color c;
            bitmap.GetPixel(x, y, &c);
            BYTE r = c.GetR();
            BYTE g = c.GetG();
            BYTE b = c.GetB();

            size_t idx = (y * w + x) * 4;
            out.pixels[idx + 0] = r;
            out.pixels[idx + 1] = g;
            out.pixels[idx + 2] = b;
            out.pixels[idx + 3] = 255;  // JPEG has no alpha, opaque
        }
    }

    return true;
}
