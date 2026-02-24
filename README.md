# Prism

[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![JUCE](https://img.shields.io/badge/JUCE-8.0.12-8DC63F?style=for-the-badge)](https://juce.com/)
[![Formats](https://img.shields.io/badge/Formats-VST3%20%7C%20AU%20%7C%20Standalone-222222?style=for-the-badge)](#build)
[![Platforms](https://img.shields.io/badge/Platforms-macOS%20%7C%20Linux%20%7C%20Windows-222222?style=for-the-badge)](#build)

Prism is a granular sampler instrument based on oual_qroccss from pdmeyer.
[Original Github](https://github.com/pdmeyer/philip-meyer-max-tutorials/tree/main/patchers/opn-oval)

![Prism Screenshot](https://github.com/powerpcme/Prism/blob/main/Images/screenshot.png)

## Features

- 10 independent voices, each with its own sample, loop, and modulation state.
- Per-voice DSP chain: speed/pitch shift, overdrive, bit crush, AM, envelope shaping.
- Per-voice host sync (`1/1` to `1/64`, plus triplets) with optional free-running mode.
- Interactive waveform strip with draggable loop start/end markers and playhead display.
- XY pad control for per-voice pan/volume placement.
- Drag-and-drop or file-picker sample loading (`.wav`, `.aif`, `.aiff`, `.mp3`, `.flac`).
- Built formats: `VST3`, `AU` (macOS), and `Standalone`.

## Build

### Linux host: Linux + Windows artifacts

```bash
./build.sh
```

Optional flags:

```bash
./build.sh --linux-only
./build.sh --windows-only
./build.sh --debug
./build.sh --clean
./build.sh --jobs 8
./build.sh --skip-deps
```

Output folders:

- `linux-artifacts/`
- `windows-artifacts/`

### macOS host: macOS artifacts

```bash
./build-mac.sh
```

Output folder:

- `mac-artifacts/` (`Prism.vst3`, `Prism.component`, `Prism.app`)


## Project Layout

```text
.
├── CMakeLists.txt
├── build.sh
├── build-mac.sh
├── cmake/windows-toolchain.cmake
├── Resources/Presets/
└── Source/
    ├── DSP/
    ├── Sync/
    ├── UI/
    ├── Voice/
    ├── PluginEditor.*
    └── PluginProcessor.*
```

## Development Notes

- CMake fetches JUCE (`8.0.12`) automatically.
- Plugin formats are configured in `CMakeLists.txt` via `juce_add_plugin(...)`.
- The Linux build script can cross-compile Windows artifacts with MinGW.

