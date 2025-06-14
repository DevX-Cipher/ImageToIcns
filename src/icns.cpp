#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include "icns.h"
#include <utils.h>
#include <iostream>

bool write_icns(const char* filename, const std::vector<PNGImage>& images) {
  // Mapping of icon sizes to their corresponding ICNS type codes
  struct IconMapping {
    uint32_t size;
    char code[5]; // 4 chars for code + null terminator
  } mapping[] = {
      {16, "icp4"},  // 16x16
      {32, "icp5"},  // 32x32
      {64, "icp6"},  // 64x64
      {128, "ic07"}, // 128x128
      {256, "ic08"}, // 256x256
      {512, "ic09"}, // 512x512
      {1024, "ic10"} // 1024x1024 (retina)
  };

  std::vector<ICNSChunk> chunks;

  for (auto& m : mapping) {
    // Find the image in the input vector that matches the current size
    auto it = std::find_if(images.begin(), images.end(), [&](const PNGImage& img) {
      return img.width == m.size && img.height == m.size;
      });

    if (it == images.end()) {
      debug_log("Missing icon size %u for ICNS file.", m.size);
      std::cerr << "write_icns: Missing icon size " << m.size << "x" << m.size << "\n";
      return false;
    }

    // Create a temporary filename for the PNG
    // Using a more unique name for temp file to avoid conflicts
    std::string tmpname = std::string("tmp_icns_icon_") + std::to_string(m.size) + ".png";

    // Write the PNG image data to the temporary file
    if (!write_png(tmpname, it->pixels, it->width, it->height)) {
      debug_log("Failed to write temp PNG for size %u: %s", m.size, tmpname.c_str());
      std::cerr << "write_icns: Failed to write temp PNG for size " << m.size << "\n";
      return false;
    }

    // Read the temporary PNG file back into a vector of bytes
    std::ifstream temp_png_file(tmpname, std::ios::binary | std::ios::ate); // Open at end to get size
    if (!temp_png_file.is_open()) {
      debug_log("Failed to open temporary PNG file %s for reading.", tmpname.c_str());
      std::cerr << "write_icns: Failed to open temp PNG for reading: " << tmpname << "\n";
      return false;
    }

    std::streampos sz = temp_png_file.tellg(); // Get file size
    std::vector<uint8_t> pngdata(static_cast<size_t>(sz));
    temp_png_file.seekg(0, std::ios::beg); // Go back to beginning
    temp_png_file.read(reinterpret_cast<char*>(pngdata.data()), sz); // Read all data
    temp_png_file.close();

    // Delete the temporary PNG file
    std::filesystem::remove(tmpname);
    debug_log("Temporary PNG file %s deleted.", tmpname.c_str());

    // Create an ICNSChunk and add it to the list
    ICNSChunk c;
    std::memcpy(c.type, m.code, 4); // Copy the 4-char code
    c.data = std::move(pngdata); // Move the PNG data
    chunks.push_back(std::move(c));
    debug_log("Added ICNS chunk for size %u, type %.4s, data size %zu", m.size, m.code, chunks.back().data.size());
  }

  // Calculate total size of the .icns file
  uint32_t total_size = 8; // "icns" header (4 bytes type + 4 bytes size)
  for (auto& c : chunks) {
    total_size += 8 + (uint32_t)c.data.size(); // Each chunk has 4 bytes type + 4 bytes size + data
  }
  debug_log("Calculated total ICNS file size: %u bytes", total_size);

  // Open the output .icns file for writing
  std::ofstream out_icns_file(filename, std::ios::binary);
  if (!out_icns_file.is_open()) {
    std::cerr << "write_icns: Failed to open " << filename << " for writing.\n";
    return false;
  }

  // Write ICNS file header
  out_icns_file.write("icns", 4); // File type
  uint8_t size_buf[4];
  write_be_uint32(size_buf, total_size);
  out_icns_file.write(reinterpret_cast<char*>(size_buf), 4); // Total file size

  // Write each ICNS chunk
  for (auto& c : chunks) {
    out_icns_file.write(c.type, 4); // Chunk type
    write_be_uint32(size_buf, (uint32_t)c.data.size() + 8); // Chunk size (data size + 8 bytes for type+size)
    out_icns_file.write(reinterpret_cast<char*>(size_buf), 4);
    out_icns_file.write(reinterpret_cast<const char*>(c.data.data()), c.data.size()); // Chunk data
    debug_log("Wrote chunk type %.4s, data size %zu", c.type, c.data.size());
  }

  out_icns_file.close();
  debug_log("Successfully wrote ICNS file: %s", filename);
  return true;
}
