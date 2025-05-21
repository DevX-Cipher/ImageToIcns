#pragma once
#include <vector>
#include <windows.h>
#include <gdiplus.h>
#include "png.h"


bool load_jpeg_gdiplus(const wchar_t* filename, PNGImage& out);
