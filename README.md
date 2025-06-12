# Image to ICNS Converter

![Platform: Windows](https://img.shields.io/badge/platform-Windows-blue)
![Builds macOS Icons](https://img.shields.io/badge/outputs-ICNS%20(macOS)-brightgreen)
![License: MIT (Non-Commercial)](https://img.shields.io/badge/license-MIT--NC-yellow)
![Build System: CMake](https://img.shields.io/badge/build-CMake-informational)

Convert PNG and JPEG images into macOS `.icns` icon files easily. Built on Windows, designed with cross-platform support in mind.

---

## ✨ Features

- Converts **PNG** and **JPEG** images to **ICNS** format
- Supports all macOS icon sizes (16x16 up to 1024x1024)
- Preserves transparency (alpha channel)
- Uses **GDI+** on Windows for image decoding
- Outputs debug-resized images for verification

---

## 🛠️ Usage

```bash
imagetoicns.exe input.png output.icns
imagetoicns.exe input.jpg output.icns
```

---

## ⚙️ Build Instructions

> The project now uses **CMake** for build configuration.

### Windows (Visual Studio):

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

> Requires:
> - Visual Studio 2019 or later
> - GDI+ (included with Windows)

---

## 📁 Debug Output

- During processing, resized PNGs are saved to a `debug_output/` folder for inspection.

---

## 🔭 Future Plans

- Cross-platform support (Linux/macOS builds)
- Replace GDI+ with a cross-platform image library (e.g., libpng or stb_image)
- Optional GUI frontend using Qt or ImGui

---

## 🤝 Contributing

Pull requests welcome! Especially if you're interested in:
- Making the image loader cross-platform
- Adding CI builds for Linux/macOS
- Improving ICNS compatibility or compression

---

## 📄 License

MIT (Non-Commercial): Free for open source and personal use.
