#include <windows.h>
#include <gdiplus.h>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

#ifdef _DEBUG
#include <filesystem>
#endif

#include "png.h"
#include "jpg.h"
#include <crc.h>

#pragma comment(lib, "gdiplus.lib")

bool load_image(const wchar_t* filename, PNGImage& out) {
    std::wstring file_str(filename);
    std::transform(file_str.begin(), file_str.end(), file_str.begin(), ::towlower);

    if (file_str.size() >= 4) {
        std::wstring ext = file_str.substr(file_str.size() - 4);
        if (ext == L".png") {
            return load_png_gdiplus(filename, out);
        }
        else if (ext == L".jpg" || ext == L".jpeg") {
            return load_jpeg_gdiplus(filename, out);
        }
    }
    return false;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        wprintf(L"Usage: %s input.png|input.jpg output.icns\n", argv[0]);
        return 1;
    }

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) {
        wprintf(L"Failed to initialize GDI+\n");
        return 1;
    }

    PNGImage original;
    if (!load_image(argv[1], original)) {
        wprintf(L"Failed to load image: %s\n", argv[1]);
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    uint32_t sizes[] = { 16, 32, 64, 128, 256, 512, 1024 };
    std::vector<PNGImage> icons;
    for (uint32_t sz : sizes) {
        PNGImage resized;
        resize_nn(original, resized, sz, sz);
        icons.push_back(std::move(resized));
    }

#ifdef _DEBUG
    namespace fs = std::filesystem;
    fs::path debug_folder = "debug_images";

    // Create the folder if it doesn't exist
    if (!fs::exists(debug_folder)) {
        fs::create_directory(debug_folder);
    }

    // Save debug PNGs inside the debug_images folder
    char buf[128];
    for (size_t i = 0; i < icons.size(); ++i) {
        sprintf_s(buf, sizeof(buf), "debug_%u.png", sizes[i]);
        fs::path debug_path = debug_folder / buf;
        write_simple_png(debug_path.string().c_str(), icons[i]);
    }
#endif

    make_crc_table();

    std::string output_filename = WideCharToUtf8(argv[2]);

    if (!write_icns(output_filename.c_str(), icons)) {
        wprintf(L"Failed to write ICNS: %s\n", argv[2]);
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    wprintf(L"ICNS file created successfully: %s\n", argv[2]);

    GdiplusShutdown(gdiplusToken);
    return 0;
}