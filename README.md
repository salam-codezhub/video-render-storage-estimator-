# 🎬 Video Render Storage Estimator

> **Stop your DaVinci Resolve renders from failing at 90%.**

A modern C++17 terminal utility that calculates the estimated output file size
for a video project and checks your drive's actual free space — before you
waste hours on a doomed render.

---

## 🚀 The Problem It Solves

Anyone who edits video has experienced the dread: you start a long render
export in DaVinci Resolve, walk away, come back 45 minutes later, and find it
failed with a **"Disk Full"** error at 87%. All that rendering time — wasted.

The root cause is simple: high-quality codecs like **Apple ProRes 422** or
**Avid DNxHR** can generate files many times larger than most people expect. A
10-minute ProRes 4K project can easily exceed 50 GB.

**This tool lets you check first.**

---

## ✨ Features

| Feature | Description |
|---|---|
| 🔢 **Accurate Estimation** | Applies codec bitrate × resolution scale × framerate |
| 💾 **Real OS Disk Check** | Uses C++17 `<filesystem>` to query actual available bytes |
| ✅ / ⚠ / ❌ **Clear Verdict** | SUCCESS, WARNING, or DANGER with actionable advice |
| 🎞 **Multi-Codec Support** | H.264, H.265, AV1, ProRes 422/4444, DNxHD/HR, CineForm, MJPEG |
| 📐 **Multi-Resolution** | 720p → 8K, with correct pixel-area scaling |
| ♻️ **Repeat Runs** | Estimate multiple codec/resolution combos in one session |

---

## 🛠 Prerequisites

| Requirement | Version |
|---|---|
| C++ Compiler | GCC ≥ 9.1, Clang ≥ 9, or MSVC 2019+ |
| C++ Standard | **C++17** (required for `<filesystem>`) |
| OS | Linux, macOS, or Windows (MSYS2/MinGW) |

No external libraries. No package manager. Compile and run.

---

## ⚙️ How to Compile

### Linux & macOS

```bash
# Clone / enter the project directory
cd video-render-storage-estimator

# Compile
g++ -std=c++17 -O2 -o render_estimator main.cpp

# Run
./render_estimator
```

### Windows (MinGW / MSYS2)

```bash
g++ -std=c++17 -O2 -o render_estimator.exe main.cpp
render_estimator.exe
```

### Windows (MSVC in Developer Command Prompt)

```cmd
cl /std:c++17 /O2 /EHsc main.cpp /Fe:render_estimator.exe
render_estimator.exe
```

---

## 🖥 Sample Usage

```
════════════════════════════════════════════════════════════
  ██╗   ██╗██████╗ ███████╗███████╗
  ...
       Video Render Storage Estimator
  Stop renders from failing at 90%!
════════════════════════════════════════════════════════════

  This tool estimates the output file size of your DaVinci Resolve
  project and checks whether your drive has enough space before you
  start a long render.

  Supported resolutions:
  ──────────────────────────────
    720p        (scale: 0.44x)
    1080p       (scale: 1.00x)
    1440p       (scale: 1.78x)
    4k          (scale: 4.00x)
    8k          (scale: 16.00x)
  ──────────────────────────────

  Enter resolution (e.g., 1080p, 4K): 4K

  Enter framerate (fps, e.g., 24, 30, 60): 24

  Enter project duration in minutes: 5

  Supported export codecs:
  ────────────────────────────────────────
    h264           → H.264 (AVC)       (~16.0 Mbps @ 1080p 30fps)
    h265           → H.265 (HEVC)      (~8.0 Mbps @ 1080p 30fps)
    prores422      → Apple ProRes 422  (~220.0 Mbps @ 1080p 30fps)
    ...
  ────────────────────────────────────────

  Enter export codec: prores422

  Path (Enter for current dir): [Enter]

════════════════════════════════════════════════════════════
  ESTIMATION REPORT
════════════════════════════════════════════════════════════

  Project Parameters:
    Resolution   : 4K
    Framerate    : 24.00 fps
    Duration     : 5.00 min (300.00 sec)
    Export Codec : Apple ProRes 422
    Check Drive  : .

────────────────────────────────────────────────────────────
  Estimated file size   : 58.93 GB
  Available disk space  : 120.41 GB
  Headroom after render : 61.48 GB
────────────────────────────────────────────────────────────

  ✅  SUCCESS — YOU'RE CLEAR TO RENDER!

      The render will use ~48.94% of your free space.
      You'll have 61.48 GB remaining after the render.

  ➜  Go ahead and hit that Render button in DaVinci Resolve!

════════════════════════════════════════════════════════════

  Run another estimate? (y/n): n

  Happy editing! 🎬
```

---

## 🗂 Project Structure

```
video-render-storage-estimator/
├── main.cpp          ← Complete C++17 source
├── README.md         ← This file
├── Project_Report.md ← BYOP Academic Report
└── GIT_STRATEGY.md   ← Git commit history guide
```

---

## 📐 How the Estimation Works

```
Effective Bitrate (Mbps) = Base Codec Mbps
                           × Resolution Scale Factor
                           × (Target FPS / 30 fps)

Total Bits = Effective Bitrate × 1,000,000 × Duration (seconds)

File Size (GiB) = Total Bits / 8 / 1,073,741,824
```

**Example — 4K 24fps ProRes 422 for 5 minutes:**
- Base: 220 Mbps × 4.0 (4K scale) × (24/30) = 704 Mbps
- Total bits: 704,000,000 × 300 = 211.2 Gbits
- File size: 211.2 Gbits ÷ 8 ÷ 1.074 ≈ **24.6 GB** per channel/pass

---

## 👤 Author

**Salam** · Reg No: 25BAI10036
B.Tech — Artificial Intelligence & Machine Learning, Year 1
VIT Bhopal University

---

## 📄 License

MIT — free to use, modify, and distribute.
