#include "icns.h"


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