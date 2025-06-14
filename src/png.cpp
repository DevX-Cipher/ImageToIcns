#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <zlib.h>
#include <array>

#include "crc.h"
#include "utils.h"
#include <resize.h>
#include <png.h>

static void write_be32(std::ostream& out, uint32_t val) {
  out.put((val >> 24) & 0xFF);
  out.put((val >> 16) & 0xFF);
  out.put((val >> 8) & 0xFF);
  out.put(val & 0xFF);
}

static void write_chunk(std::ostream& out, const char* type, const std::vector<uint8_t>& data) {
  write_be32(out, static_cast<uint32_t>(data.size()));

  std::array<uint8_t, 4> chunk_type;
  std::memcpy(chunk_type.data(), type, 4);
  out.write(reinterpret_cast<const char*>(chunk_type.data()), 4);

  if (!data.empty())
    out.write(reinterpret_cast<const char*>(data.data()), data.size());

  uint32_t crc = crc32(0, nullptr, 0);
  crc = crc32(crc, chunk_type.data(), 4);
  if (!data.empty())
    crc = crc32(crc, data.data(), data.size());

  write_be32(out, crc);
}

bool write_png(const std::string& filename, const std::vector<Pixel>& pixels, int width, int height) {
  std::ofstream out(filename, std::ios::binary);
  if (!out) {
    std::cerr << "write_png: Failed to open " << filename << " for writing\n";
    return false;
  }

  // PNG signature
  const uint8_t png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
  out.write(reinterpret_cast<const char*>(png_sig), 8);

  // IHDR chunk (13 bytes)
  std::vector<uint8_t> ihdr(13);
  ihdr[0] = (width >> 24) & 0xFF;
  ihdr[1] = (width >> 16) & 0xFF;
  ihdr[2] = (width >> 8) & 0xFF;
  ihdr[3] = width & 0xFF;

  ihdr[4] = (height >> 24) & 0xFF;
  ihdr[5] = (height >> 16) & 0xFF;
  ihdr[6] = (height >> 8) & 0xFF;
  ihdr[7] = height & 0xFF;

  ihdr[8] = 8;    // Bit depth (8 bits per channel)
  ihdr[9] = 6;    // Color type RGBA (6 = Truecolor with alpha)
  ihdr[10] = 0;   // Compression method (deflate)
  ihdr[11] = 0;   // Filter method (adaptive filtering with five basic filter types)
  ihdr[12] = 0;   // Interlace method (None)

  write_chunk(out, "IHDR", ihdr);

  // Prepare raw image data with filter bytes (filter 0: None)
  std::vector<uint8_t> raw_image_data_with_filters;
  raw_image_data_with_filters.reserve((width * 4 + 1) * height);
  for (int y = 0; y < height; ++y) {
    raw_image_data_with_filters.push_back(0); // Filter byte 0 (None) for each scanline
    for (int x = 0; x < width; ++x) {
      const Pixel& p = pixels[y * width + x];
      raw_image_data_with_filters.push_back(p.r);
      raw_image_data_with_filters.push_back(p.g);
      raw_image_data_with_filters.push_back(p.b);
      raw_image_data_with_filters.push_back(p.a);
    }
  }

  // Compress with zlib
  uLongf compressed_size = compressBound(raw_image_data_with_filters.size());
  std::vector<uint8_t> compressed_data(compressed_size);
  int ret = compress2(compressed_data.data(), &compressed_size,
    raw_image_data_with_filters.data(), raw_image_data_with_filters.size(),
    Z_BEST_COMPRESSION);
  if (ret != Z_OK) {
    std::cerr << "write_png: zlib compress failed with code " << ret << "\n";
    return false;
  }
  compressed_data.resize(compressed_size);

  write_chunk(out, "IDAT", compressed_data);
  write_chunk(out, "IEND", {});

  return true;
}

static uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c) {
  // a = left, b = above, c = upper-left
  int p = (int)a + (int)b - (int)c;
  int pa = std::abs(p - (int)a);
  int pb = std::abs(p - (int)b);
  int pc = std::abs(p - (int)c);

  if (pa <= pb && pa <= pc) return a;
  else if (pb <= pc) return b;
  else return c;
}


bool load_simple_png(const std::string& filename, PNGImage& out) {
  std::ifstream f(filename, std::ios::binary);
  if (!f.is_open()) {
    std::cerr << "load_simple_png: Failed to open " << filename << "\n";
    return false;
  }

  // Read signature
  uint8_t sig[8];
  f.read(reinterpret_cast<char*>(sig), 8);
  if (!f || std::memcmp(sig, "\x89PNG\r\n\x1a\n", 8) != 0) {
    std::cerr << "load_simple_png: Invalid PNG signature in " << filename << "\n";
    return false;
  }

  uint32_t width = 0, height = 0;
  uint8_t color_type = 0, bit_depth = 0, interlace = 0;
  bool found_ihdr = false;
  std::vector<uint8_t> idat_data;

  // Read chunks
  while (true) {
    uint8_t len_buf[4];
    if (!f.read(reinterpret_cast<char*>(len_buf), 4)) {
      if (f.eof()) {
        debug_log ("Reached EOF while reading chunk length. Exiting chunk reading loop.");
        break; // Reached end of file unexpectedly early, but can be normal for IEND
      }
      std::cerr << "load_simple_png: Failed to read chunk length in " << filename << "\n";
      return false;
    }
    uint32_t len = (len_buf[0] << 24) | (len_buf[1] << 16) | (len_buf[2] << 8) | len_buf[3];

    char type[5] = { 0 };
    if (!f.read(type, 4)) {
      std::cerr << "load_simple_png: Failed to read chunk type in " << filename << "\n";
      return false;
    }

    debug_log("Reading chunk type: %s, length: %u", type, len);

    std::vector<uint8_t> chunk(len);
    if (len > 0) {
      if (!f.read(reinterpret_cast<char*>(chunk.data()), len)) {
        std::cerr << "load_simple_png: Failed to read chunk data in " << filename << "\n";
        return false;
      }
    }

    // Read and discard CRC (4 bytes)
    f.seekg(4, std::ios::cur);
    if (!f) {
      std::cerr << "load_simple_png: Failed to skip CRC in " << filename << "\n";
      return false;
    }

    if (std::strcmp(type, "IHDR") == 0) {
      if (len != 13) {
        std::cerr << "load_simple_png: Invalid IHDR length in " << filename << "\n";
        return false;
      }
      width = (chunk[0] << 24) | (chunk[1] << 16) | (chunk[2] << 8) | chunk[3];
      height = (chunk[4] << 24) | (chunk[5] << 16) | (chunk[6] << 8) | chunk[7];
      bit_depth = chunk[8];
      color_type = chunk[9];
      interlace = chunk[12];

      debug_log("IHDR: width=%u, height=%u, bit_depth=%d, color_type=%d, interlace=%d",
        width, height, (int)bit_depth, (int)color_type, (int)interlace);

      // Only support 8-bit RGBA (color type 6) and no interlacing for this simple loader
      if (color_type != 6 || bit_depth != 8 || interlace != 0) {
        std::cerr << "load_simple_png: Unsupported format (color_type=" << (int)color_type
          << ", bit_depth=" << (int)bit_depth << ", interlace=" << (int)interlace
          << ") in " << filename << ". Only 8-bit RGBA non-interlaced supported.\n";
        return false;
      }
      found_ihdr = true;
    }
    else if (std::strcmp(type, "IDAT") == 0) {
      idat_data.insert(idat_data.end(), chunk.begin(), chunk.end());
      debug_log("Appended IDAT chunk. Total IDAT size: %zu", idat_data.size());
    }
    else if (std::strcmp(type, "IEND") == 0) {
      debug_log("Found IEND chunk. Breaking chunk reading loop.");
      break; // End of PNG
    }
    // Other chunks (PLTE, tRNS, gAMA, etc.) are ignored for simplicity
  }

  if (!found_ihdr) {
    std::cerr << "load_simple_png: Missing IHDR chunk in " << filename << "\n";
    return false;
  }
  if (idat_data.empty()) {
    std::cerr << "load_simple_png: No IDAT data in " << filename << "\n";
    return false;
  }

  // Decompress IDAT data using zlib
  z_stream strm{};
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = static_cast<uInt>(idat_data.size());
  strm.next_in = idat_data.data();

  // Initialize zlib for inflation (decompression)
  if (inflateInit(&strm) != Z_OK) {
    std::cerr << "load_simple_png: zlib inflateInit failed for " << filename << "\n";
    return false;
  }

  // Expected size of decompressed data: (filter_byte + width * bytes_per_pixel) * height
  const int bytes_per_pixel = 4; // RGBA
  const size_t scanline_stride = 1 + width * bytes_per_pixel; // 1 for filter byte + pixel data
  const size_t expected_decompressed_size = static_cast<size_t>(height) * scanline_stride;
  std::vector<uint8_t> decompressed_data;
  decompressed_data.reserve(expected_decompressed_size); // Pre-allocate to reduce reallocations

  std::vector<uint8_t> buffer(16384); // Buffer for zlib to write into
  int ret = Z_OK;
  do {
    strm.avail_out = static_cast<uInt>(buffer.size());
    strm.next_out = buffer.data();

    ret = inflate(&strm, Z_NO_FLUSH); // Inflate data

    if (ret != Z_OK && ret != Z_STREAM_END) {
      inflateEnd(&strm);
      std::cerr << "load_simple_png: zlib inflate error (ret=" << ret << ", msg=" << (strm.msg ? strm.msg : "unknown") << ") for " << filename << "\n";
      return false;
    }

    size_t have = buffer.size() - strm.avail_out;
    decompressed_data.insert(decompressed_data.end(), buffer.begin(), buffer.begin() + have);

  } while (ret != Z_STREAM_END);

  inflateEnd(&strm); // Clean up zlib stream

  if (decompressed_data.size() < expected_decompressed_size) {
    std::cerr << "load_simple_png: Decompressed data too short: " << decompressed_data.size() << " < " << expected_decompressed_size << " for " << filename << "\n";
    return false;
  }

  debug_log("Decompressed data size: %zu, expected: %zu", decompressed_data.size(), expected_decompressed_size);

  out.width = width;
  out.height = height;
  out.pixels.resize(width * height);

  // Buffer to hold the unfiltered bytes of the *previous* scanline, initialized to all zeros
  std::vector<uint8_t> prev_scanline_raw_bytes(width * bytes_per_pixel, 0);

  const uint8_t* current_decompressed_ptr = decompressed_data.data();

  for (uint32_t y = 0; y < height; ++y) {
    uint8_t filter_type = *current_decompressed_ptr++; // Read filter byte

    // Temporary buffer to store the unfiltered bytes of the *current* scanline
    std::vector<uint8_t> current_scanline_raw_bytes(width * bytes_per_pixel);

    for (int i = 0; i < width * bytes_per_pixel; ++i) {
      uint8_t filtered_byte = *current_decompressed_ptr++; // Read the filtered pixel byte

      // Get predicted values (a=left, b=above, c=upper-left)
      uint8_t a = (i >= bytes_per_pixel) ? current_scanline_raw_bytes[i - bytes_per_pixel] : 0;
      uint8_t b = prev_scanline_raw_bytes[i];
      uint8_t c = (i >= bytes_per_pixel) ? prev_scanline_raw_bytes[i - bytes_per_pixel] : 0;

      uint8_t unfiltered_byte;

      // Apply unfiltering based on filter type
      switch (filter_type) {
      case 0: // None
        unfiltered_byte = filtered_byte;
        break;
      case 1: // Sub
        unfiltered_byte = filtered_byte + a;
        break;
      case 2: // Up
        unfiltered_byte = filtered_byte + b;
        break;
      case 3: // Average
        unfiltered_byte = filtered_byte + (uint8_t)((a + b) / 2);
        break;
      case 4: // Paeth
        unfiltered_byte = filtered_byte + paeth_predictor(a, b, c);
        break;
      default:
        std::cerr << "load_simple_png: Unsupported filter type " << (int)filter_type << " found for scanline " << y << " in " << filename << "\n";
        return false;
      }
      current_scanline_raw_bytes[i] = unfiltered_byte;
    }

    // Copy unfiltered bytes to the Pixel struct vector
    for (uint32_t x = 0; x < width; ++x) {
      size_t pixel_idx = y * width + x;
      size_t byte_offset = x * bytes_per_pixel;
      Pixel& px = out.pixels[pixel_idx];
      px.r = current_scanline_raw_bytes[byte_offset];
      px.g = current_scanline_raw_bytes[byte_offset + 1];
      px.b = current_scanline_raw_bytes[byte_offset + 2];
      px.a = current_scanline_raw_bytes[byte_offset + 3];
    }

    // The current scanline becomes the previous scanline for the next iteration
    prev_scanline_raw_bytes = current_scanline_raw_bytes;
  }

  debug_log("Loaded PNG %s (%ux%u, pixels=%zu)", filename.c_str(), out.width, out.height, out.pixels.size());

  return true;
}

void resize_nn(const PNGImage& src, PNGImage& dst, uint32_t w, uint32_t h) {
  if (src.pixels.size() < src.width * src.height) {
    std::cerr << "resize_nn: Invalid source pixels size " << src.pixels.size()
      << ", expected " << src.width * src.height << "\n";
    return;
  }

  debug_log("Resizing from %ux%u to %ux%u", src.width, src.height, w, h);

  dst.width = w;
  dst.height = h;
  dst.pixels.resize(w * h);

  for (uint32_t y = 0; y < h; y++) {
    for (uint32_t x = 0; x < w; x++) {
      uint32_t sx = x * src.width / w;
      uint32_t sy = y * src.height / h;
      size_t sidx = sy * src.width + sx;
      size_t didx = y * w + x;

      if (sidx >= src.pixels.size()) {
        std::cerr << "resize_nn: Out-of-bounds sidx=" << sidx << ", src size=" << src.pixels.size() << "\n";
        return;
      }

      Pixel px = src.pixels[sidx];

      // If the sampled pixel is fully transparent, try to find a non-transparent neighbor
      if (px.a == 0) {
        bool found = false;

        // 5x5 neighborhood search
        for (int dy = -2; dy <= 2 && !found; dy++) {
          for (int dx = -2; dx <= 2 && !found; dx++) {
            int nx = static_cast<int>(sx) + dx;
            int ny = static_cast<int>(sy) + dy;
            if (nx >= 0 && nx < static_cast<int>(src.width) && ny >= 0 && ny < static_cast<int>(src.height)) {
              size_t nidx = ny * src.width + nx;
              if (nidx < src.pixels.size() && src.pixels[nidx].a > 0) { // Check for valid index and non-zero alpha
                px = src.pixels[nidx];
                found = true;
              }
            }
          }
        }

        // Larger step search if still transparent (only if 5x5 failed)
        if (!found) {
          uint32_t step_x = std::max(1u, src.width / 10);
          uint32_t step_y = std::max(1u, src.height / 10);
          for (uint32_t ny_step = 0; ny_step < src.height && !found; ny_step += step_y) {
            for (uint32_t nx_step = 0; nx_step < src.width && !found; nx_step += step_x) {
              size_t nidx = ny_step * src.width + nx_step;
              if (nidx < src.pixels.size() && src.pixels[nidx].a > 0) { // Check for valid index and non-zero alpha
                px = src.pixels[nidx];
                found = true;
              }
            }
          }
        }

        if (!found) {
          px = Pixel{ 255, 255, 255, 255 }; // fallback white if no non-transparent pixel found
          debug_log("No non-transparent pixel found for (%u,%u) after search, using white fallback.", x, y);
        }
      }

      dst.pixels[didx] = px;
    }
  }

  // Sample a few resized pixels for debugging
  for (uint32_t i = 0; i < 3 && i < dst.pixels.size(); ++i) {
    const Pixel& p = dst.pixels[i];
    debug_log("Resized pixel sample (%u,%u): R=%u, G=%u, B=%u, A=%u",
      (i % w), (i / w), (unsigned)p.r, (unsigned)p.g, (unsigned)p.b, (unsigned)p.a);
  }
}
