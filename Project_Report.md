# Project Report: Video Render Storage Estimator

| Field | Details |
|---|---|
| **Student Name** | Salam |
| **Registration Number** | 25BAI10036 |
| **Programme** | B.Tech — Artificial Intelligence & Machine Learning |
| **Year / Semester** | Year 1 / Semester 2 |
| **Institution** | VIT Bhopal University |
| **Project Type** | Bring Your Own Project (BYOP) Capstone |
| **Language / Standard** | C++17 |
| **Submission Date** | 2025 |

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [Why This Problem Matters](#2-why-this-problem-matters)
3. [Approach and Solution Design](#3-approach-and-solution-design)
4. [Key Technical Decisions](#4-key-technical-decisions)
5. [System Architecture](#5-system-architecture)
6. [Challenges Faced](#6-challenges-faced)
7. [Learnings and Takeaways](#7-learnings-and-takeaways)
8. [Future Scope](#8-future-scope)
9. [Conclusion](#9-conclusion)
10. [References](#10-references)

---

## 1. Problem Statement

I have been learning video editing using **DaVinci Resolve**, a professional
non-linear editing (NLE) application. A persistent, frustrating problem I
encountered repeatedly was this:

> A long video export — sometimes taking 30–90 minutes — would fail late in the
> process (often at 80–95% completion) because the target hard drive ran out of
> storage space.

The program would either crash silently or show a generic error message. The
render had to be started from scratch after freeing space, wasting significant
time and system resources.

The core cause of this problem is a **mismatch between user expectation and
technical reality**: high-fidelity video codecs such as Apple ProRes 422 or
Avid DNxHR produce files that are far larger than most beginners expect. A
5-minute ProRes 422 export at 4K resolution can exceed 50–60 GB — approximately
one-third of a typical laptop's SSD.

---

## 2. Why This Problem Matters

### 2.1 Scale of the Problem

Video content creation is at an all-time high. Platforms like YouTube, Instagram
Reels, and short-form video apps have motivated millions of students and
independent creators to learn video editing. Many of these creators work on
entry-level hardware — laptops with 256 GB or 512 GB SSDs where storage is
precious.

### 2.2 Why Existing Solutions Are Insufficient

- **DaVinci Resolve** does not prominently display an estimated output size
  before beginning a render.
- **Operating system disk warnings** only appear once the drive is completely
  full — too late for an already-running render.
- Online calculators exist, but they are web-only, generic, and do not read
  actual OS disk information — they are pure calculators, not system utilities.

### 2.3 The Gap This Project Fills

A local terminal utility that:
1. Computes an estimated file size using real codec bitrate data, and
2. Reads the actual free space from the OS before the render starts

...bridges this gap entirely and prevents the problem before it occurs.

---

## 3. Approach and Solution Design

### 3.1 Language Choice: C++

C++ was chosen for several reasons aligned with both the project goal and the
course curriculum:

- The `<filesystem>` header introduced in **C++17** provides a portable,
  standardised API for querying disk space directly from the operating system —
  something that is impossible from a purely algorithmic language.
- C++ programs compile to native executables with no runtime dependency, making
  the tool immediately usable on any system.
- Practising C++ reinforces core computer science concepts: types, memory,
  standard library, and compilation.

### 3.2 Estimation Model

Video file size is fundamentally determined by:

```
File Size = Bitrate × Duration
```

More precisely:

```
Effective Bitrate (Mbps) = Base Codec Bitrate
                           × Resolution Scale Factor
                           × (Target FPS / Reference FPS)

File Size (GiB) = (Effective Bitrate × 1,000,000 × Duration in seconds)
                  / 8 / 1,073,741,824
```

**Resolution Scale Factor** accounts for the proportionally larger amount of
pixel data at higher resolutions. It is calculated as the ratio of the
resolution's total pixel count to the baseline (1920×1080):

| Resolution | Pixels | Scale Factor |
|---|---|---|
| 720p | 921,600 | 0.44× |
| 1080p (baseline) | 2,073,600 | 1.00× |
| 1440p | 3,686,400 | 1.78× |
| 4K UHD | 8,294,400 | 4.00× |
| 8K | 33,177,600 | 16.00× |

**Framerate scaling** is linear: a 60 fps export generates exactly twice as many
frames per second as a 30 fps export, so it uses approximately twice the
bitrate.

### 3.3 Codec Bitrate Table

Rather than prompting the user for a bitrate (which most beginners do not
know), the program maintains a lookup table of representative average bitrates
for each major export codec used in DaVinci Resolve:

| Codec | Base Mbps (1080p 30fps) |
|---|---|
| H.264 (AVC) | 16 |
| H.265 (HEVC) | 8 |
| AV1 | 6 |
| Apple ProRes 422 | 220 |
| Apple ProRes 4444 | 330 |
| Avid DNxHD | 145 |
| Avid DNxHR HQ | 290 |
| GoPro CineForm | 200 |
| Motion JPEG | 50 |

### 3.4 Disk Space Check

The `std::filesystem::space()` function returns a `space_info` struct containing:

- `capacity` — total drive capacity in bytes
- `free` — total free bytes (including reserved OS blocks)
- `available` — bytes available to normal user processes (**this is used**)

The `available` field is deliberately used over `free` because reserved system
blocks should not be counted as user-accessible space.

### 3.5 Verdict Logic

After computing the estimated size and available space, the tool applies a
three-tier verdict:

| Condition | Verdict |
|---|---|
| `available < estimated` | ❌ DANGER — render will fail |
| `available < estimated / 0.90` | ⚠ WARNING — within 10% safety margin |
| Otherwise | ✅ SUCCESS — clear to render |

The 10% safety margin accounts for DaVinci Resolve's scratch disk usage during
rendering (temporary preview cache, audio files, etc.).

---

## 4. Key Technical Decisions

### 4.1 Using `<filesystem>` — The Defining Design Choice

The single most important technical decision in this project was using
`std::filesystem::space()` rather than asking the user to type in their
available space manually.

**Why this matters:**

A tool that simply computes `bitrate × duration` is a *calculator*. Any
spreadsheet or web form can do that. By integrating `<filesystem>`, the program
becomes a true **system utility** — it interacts with the operating system to
retrieve real, live data at runtime. This is the kind of program that could
realistically be packaged and distributed as a professional developer tool.

This decision also required choosing C++17 specifically (the standard that
stabilised `<filesystem>` as part of the STL, previously it was an experimental
`<experimental/filesystem>` header).

### 4.2 `std::map` for Lookup Tables

Codec and resolution data are stored in `std::map<std::string, T>`, providing
O(log n) lookup and automatic ordering. A `struct CodecInfo` groups related
data (display name + bitrate) to avoid parallel arrays.

### 4.3 Input Normalisation

User input for codec and resolution names is normalised before lookup:
- Converted to lowercase
- Spaces and hyphens removed

This means `"ProRes 422"`, `"prores422"`, and `"PRORES-422"` all resolve to the
same entry, making the tool robust against varied user input.

### 4.4 Error-Safe Input Loops

Every numeric input uses a loop that checks `std::cin` state flags and clears
the error + buffer on invalid input. The program will never crash on unexpected
user input — a key requirement for a tool intended for non-technical users.

### 4.5 `std::error_code` for Filesystem Errors

`std::filesystem::space()` is called with an `std::error_code& ec` argument
instead of relying on exceptions. This allows the program to detect and report
invalid paths gracefully without crashing.

---

## 5. System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    main.cpp (single-file)                │
├───────────────┬────────────────────┬────────────────────┤
│   INPUT LAYER │  COMPUTATION LAYER │  OUTPUT LAYER      │
│               │                    │                    │
│ readString()  │ RESOLUTION_SCALE   │ printBanner()      │
│ readPositive  │ CODEC_TABLE        │ printResult()      │
│   Double()    │ estimateFileSize   │ printRule()        │
│ normaliseKey()│   GB()             │ printCodecMenu()   │
│               │                    │ printResolution    │
│               │  FILESYSTEM LAYER  │   Menu()           │
│               │ getAvailableSpace  │                    │
│               │   Bytes()          │                    │
│               │ std::filesystem    │                    │
│               │   ::space()        │                    │
└───────────────┴────────────────────┴────────────────────┘
```

Data flows in one direction: user input → computation → display. There is no
mutable global state, making the logic straightforward to trace and debug.

---

## 6. Challenges Faced

### 6.1 Understanding the `<filesystem>` API

The C++17 `<filesystem>` API was entirely new to me. The main challenge was
understanding the distinction between `free` and `available` in `space_info`,
and why using `available` produces more accurate results for user-accessible
space.

**Solution:** I read the cppreference documentation for `std::filesystem::space`
carefully and ran small test programs to verify the values matched what `df -h`
reported on Linux.

### 6.2 Cross-Platform Compilation

The `<filesystem>` header behaved differently across compilers:
- GCC versions before 9.1 required linking `-lstdc++fs` as a separate library.
- On some macOS versions with older Clang, `<filesystem>` was not available even
  with `-std=c++17`.

**Solution:** I specified GCC ≥ 9.1 as a prerequisite (where `<filesystem>` is
fully integrated) and documented platform-specific compile instructions.

### 6.3 Input Validation Without Crashes

Making `std::cin` robust against invalid input (letters where a number is
expected, empty lines, etc.) required carefully managing the stream's error
state with `std::cin.clear()` and `ignore()`. Early iterations would enter
infinite loops or crash on unexpected input.

**Solution:** Encapsulated all input reading into `readPositiveDouble()` and
`readString()` helper functions with proper error handling loops.

### 6.4 Bitrate Accuracy

Video bitrates vary significantly based on scene complexity, encoder settings
(CRF, VBR, CBR), and content type. The table values are averages — an
action scene encodes at a higher bitrate than a static interview shot.

**Solution:** Added a 10% safety margin to the verdict thresholds, and made
the WARNING message explicitly note that the estimate is approximate.

---

## 7. Learnings and Takeaways

### 7.1 C++17 Standard Library is Powerful

This project demonstrated that the C++ standard library alone — no third-party
dependencies — is sufficient to build a tool that interacts with the OS,
manages complex string input, and produces well-formatted terminal output.

### 7.2 The Difference Between a Calculator and a System Utility

The key insight of this project is architectural: integrating `<filesystem>`
transforms a calculator into a system utility. This distinction is important in
software engineering — a tool's value often comes not from its algorithm alone,
but from how it integrates with its environment.

### 7.3 Defensive Programming

Writing code that handles unexpected user input gracefully — without panicking
or crashing — is as important as writing correct business logic. The `readPositiveDouble()`
and `readString()` functions embody defensive programming practice.

### 7.4 Codec Knowledge as a Developer

Understanding why ProRes files are vastly larger than H.264 files — because they
use intra-frame compression with minimal temporal compression — is relevant
knowledge for a developer building media tools. Bridging domain expertise
(video production) with software engineering is a core skill.

---

## 8. Future Scope

The following enhancements would be natural next steps for this project:

| Enhancement | Description |
|---|---|
| **GUI Version** | Port to Qt or a web frontend for non-technical users |
| **DaVinci Resolve Plugin** | A Lua plugin that runs this check automatically from within Resolve |
| **Per-scene Analysis** | Accept a timeline export (EDL/XML) and estimate per-clip sizes |
| **Cloud Drive Support** | Query available space on Google Drive, Dropbox, etc. |
| **Bitrate Profile Editor** | Allow users to input custom bitrate profiles for their specific encoder settings |
| **Multi-drive Check** | Simultaneously check multiple target drives and recommend the best one |
| **History / Log File** | Save estimation history to a CSV for project budget planning |

---

## 9. Conclusion

The **Video Render Storage Estimator** successfully addresses a real,
frequently experienced problem in video production. By combining a
mathematically grounded estimation model with C++17's `<filesystem>` library
for live OS disk space queries, the project delivers a practical system utility
rather than a simple calculator.

The project provided hands-on experience with:
- C++17 features (`<filesystem>`, structured bindings, `std::error_code`)
- Standard library containers (`std::map`)
- Defensive input handling
- Terminal UI design
- Cross-platform considerations for system programming

Most importantly, it demonstrated that first-year programming knowledge is
sufficient to build tools that solve real problems — a motivating outcome for
continued study.

---

## 10. References

1. **cppreference — std::filesystem::space**
   https://en.cppreference.com/w/cpp/filesystem/space

2. **cppreference — std::filesystem::space_info**
   https://en.cppreference.com/w/cpp/filesystem/space_info

3. **Apple ProRes White Paper** — Apple Inc.
   https://www.apple.com/final-cut-pro/docs/Apple_ProRes_White_Paper.pdf

4. **Avid DNxHR Codec Technical Overview** — Avid Technology
   https://resources.avid.com/SupportFiles/AMA/AvidDNxHR_TechOverview.pdf

5. **ISO/IEC 14496-10 (H.264/AVC)** — International Organisation for
   Standardisation

6. **Bjarne Stroustrup, *The C++ Programming Language*, 4th Edition** —
   Addison-Wesley, 2013

7. **Nicolai M. Josuttis, *The C++ Standard Library*, 2nd Edition** —
   Addison-Wesley, 2012

---

*Report prepared as part of the BYOP Capstone Assessment, VIT Bhopal, 2025.*
