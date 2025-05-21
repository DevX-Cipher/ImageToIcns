# Image to ICNS Converter 

![Platform: Windows](https://img.shields.io/badge/platform-Windows-blue)
![Builds macOS Apps](https://img.shields.io/badge/builds-macOS-brightgreen)
![License: MIT (Non-Commercial)](https://img.shields.io/badge/license-MIT--NC-yellow)

Convert PNG and JPEG images into macOS ICNS icon files easily. More image formats may be supported in the future!

## Features ✨
- Converts PNG and JPEG images to ICNS format
- Supports multiple icon sizes (16x16 to 1024x1024)
- Keeps transparency intact
- Uses GDI+ on Windows for image processing
- Debug resized images output for verification

## Usage 🛠️

```bash
imagetoicns.exe input.png output.icns
imagetoicns.exe input.jpg output.icns
```

## Build ⚙️

- Requires Windows with Visual Studio
- Uses GDI+ for image processing

## Notes ℹ️
- Make sure your console supports Unicode emojis for best experience!
- Debug resized PNG files are saved in a separate folder during processing.
---

