#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <fstream>

#ifdef _DEBUG
#include <filesystem>
#endif

#include "png.h"
#include "jpg.h"
#include "icns.h"
#include "crc.h"

bool load_image(const char* filename, PNGImage& out) {
  // Check if file exists and is readable
  std::ifstream file_check(filename, std::ios::binary);
  if (!file_check.is_open()) {
    std::fprintf(stderr, "Error: Cannot open file %s (file may not exist or is inaccessible)\n", filename);
    return false;
  }
  file_check.close();

  // Get lowercase extension
  std::string file_str(filename);
  std::transform(file_str.begin(), file_str.end(), file_str.begin(), ::tolower);
  std::string ext = file_str.size() >= 4 ? file_str.substr(file_str.size() - 4) : "";

  // Handle supported extensions
  if (ext == ".png") {
    if (!load_simple_png(filename, out)) {
      std::fprintf(stderr, "Error: Failed to load PNG file %s (possible corruption or unsupported format)\n", filename);
      return false;
    }
    return true;
  }
  if (ext == ".jpg" || (file_str.size() >= 5 && file_str.substr(file_str.size() - 5) == ".jpeg")) {
    if (!load_jpeg(filename, out)) {
      std::fprintf(stderr, "Error: Failed to load JPEG file %s\n", filename);
      return false;
    }
    return true;
  }

  std::fprintf(stderr, "Error: Unsupported file extension for %s (must be .png, .jpg, or .jpeg)\n", filename);
  return false;
}


int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::printf("Usage: %s input.png|input.jpg output.icns\n", argv[0]);
    return 1;
  }

  PNGImage original;
  if (!load_image(argv[1], original)) {
    std::printf("Failed to load image: %s\n", argv[1]);
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
  if (!fs::exists(debug_folder)) fs::create_directory(debug_folder);

  char buf[128];
  for (size_t i = 0; i < icons.size(); ++i) {
    std::snprintf(buf, sizeof(buf), "debug_%u.png", sizes[i]);
    fs::path debug_path = debug_folder / buf;

    if (!write_png(debug_path.string().c_str(), icons[i].pixels, icons[i].width, icons[i].height)) {
      std::fprintf(stderr, "Failed to write debug PNG: %s\n", debug_path.string().c_str());
    }
  }
#endif

  make_crc_table();

  if (!write_icns(argv[2], icons)) {
    std::printf("Failed to write ICNS: %s\n", argv[2]);
    return 1;
  }

  std::printf("ICNS file created successfully: %s\n", argv[2]);
  return 0;
}