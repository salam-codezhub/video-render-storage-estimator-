/**
 * ============================================================
 *  Video Render Storage Estimator
 *  BYOP Capstone Project — VIT Bhopal
 *  Author  : Salam  |  Reg No: 25BAI10036
 *  Course  : B.Tech Artificial Intelligence & Machine Learning
 *  Standard: C++17  (requires <filesystem>)
 * ============================================================
 *
 *  Problem:
 *    DaVinci Resolve renders often fail late (e.g., at 90%) because the
 *    target drive runs out of space.  This utility lets the user calculate
 *    the estimated output file size for a given project *before* starting
 *    the render and compares it against actual OS-reported free space.
 *
 *  Compile (Linux / macOS):
 *    g++ -std=c++17 -O2 -o render_estimator main.cpp
 *
 *  Compile (Windows, MinGW / MSYS2):
 *    g++ -std=c++17 -O2 -o render_estimator.exe main.cpp
 *
 *  Run:
 *    ./render_estimator
 */

// ─── Standard Library Headers ────────────────────────────────────────────────
#include <iostream>      // std::cin, std::cout
#include <iomanip>       // std::setprecision, std::fixed
#include <string>        // std::string, std::stod, std::stoi
#include <map>           // std::map  (codec → bitrate lookup table)
#include <limits>        // std::numeric_limits (for input clearing)
#include <stdexcept>     // std::invalid_argument, std::out_of_range
#include <algorithm>     // std::transform (for toLower helper)
#include <filesystem>    // std::filesystem::space  ← C++17 key feature
// ─────────────────────────────────────────────────────────────────────────────

namespace fs = std::filesystem;

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                         CONSTANTS & DATA TABLES                         ║
// ╚══════════════════════════════════════════════════════════════════════════╝

/**
 * Average bitrates (Megabits per second) for common export codecs.
 *
 * Values are representative mid-range figures widely used in the video
 * production community.  They are multiplied by a resolution/framerate
 * scalar inside the estimation function.
 *
 * Key  →  {codec display name, base Mbps at 1080p 30 fps}
 */
struct CodecInfo {
    std::string displayName;
    double      baseMbps;       // Megabits per second @ 1080p 30 fps
};

const std::map<std::string, CodecInfo> CODEC_TABLE = {
    //  key (lowercase)    display name          base Mbps
    { "h264",             { "H.264 (AVC)",             16.0  } },
    { "h265",             { "H.265 (HEVC)",             8.0  } },
    { "hevc",             { "H.265 (HEVC)",             8.0  } },
    { "av1",              { "AV1",                      6.0  } },
    { "prores422",        { "Apple ProRes 422",        220.0  } },
    { "prores4444",       { "Apple ProRes 4444",       330.0  } },
    { "dnxhd",            { "Avid DNxHD",              145.0  } },
    { "dnxhr",            { "Avid DNxHR HQ",           290.0  } },
    { "cineform",         { "GoPro CineForm",          200.0  } },
    { "mjpeg",            { "Motion JPEG",              50.0  } },
};

/**
 * Resolution scaling factors relative to 1080p (1920×1080 = 2,073,600 px).
 * Higher resolutions push more data through the encoder, proportionally
 * increasing bitrate.
 */
const std::map<std::string, double> RESOLUTION_SCALE = {
    { "720p",   (1280.0  * 720.0)  / (1920.0 * 1080.0) },  // ≈ 0.44
    { "1080p",  1.0                                      },  // baseline
    { "1440p",  (2560.0  * 1440.0) / (1920.0 * 1080.0) },  // ≈ 1.78
    { "4k",     (3840.0  * 2160.0) / (1920.0 * 1080.0) },  // ≈ 4.0
    { "4kuhd",  (3840.0  * 2160.0) / (1920.0 * 1080.0) },  // alias
    { "6k",     (6144.0  * 3160.0) / (1920.0 * 1080.0) },  // ≈ 9.3
    { "8k",     (7680.0  * 4320.0) / (1920.0 * 1080.0) },  // ≈ 16.0
};

/** Framerate scaling factor relative to 30 fps (the base used in CODEC_TABLE). */
constexpr double BASE_FPS = 30.0;

/** Conversion constants */
constexpr double BITS_PER_MEGABIT  = 1'000'000.0;   // 1 Mbps = 10^6 bits/s
constexpr double BYTES_PER_GIGABYTE = 1'073'741'824.0; // 1 GiB = 2^30 bytes

// Safety headroom: warn if estimated size exceeds available * this ratio
constexpr double SAFETY_MARGIN = 0.90;

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                          HELPER UTILITIES                                ║
// ╚══════════════════════════════════════════════════════════════════════════╝

/** Convert a string to lowercase in-place. */
void toLower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}

/** Remove spaces and hyphens so "ProRes 422" normalises to "prores422". */
std::string normaliseKey(std::string s) {
    toLower(s);
    s.erase(std::remove_if(s.begin(), s.end(),
            [](char c){ return c == ' ' || c == '-'; }), s.end());
    return s;
}

/** Print a horizontal rule for visual separation. */
void printRule(char ch = '─', int width = 60) {
    std::cout << std::string(width, ch) << '\n';
}

/**
 * Safely read a positive floating-point value from stdin.
 * Keeps prompting until the user enters something valid and positive.
 */
double readPositiveDouble(const std::string& prompt) {
    double value = 0.0;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value && value > 0.0) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cout << "  [!] Invalid input. Please enter a positive number.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

/**
 * Safely read a non-empty trimmed string from stdin.
 */
std::string readString(const std::string& prompt) {
    std::string s;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, s);
        // Trim leading/trailing whitespace
        auto start = s.find_first_not_of(" \t\r\n");
        auto end   = s.find_last_not_of (" \t\r\n");
        if (start != std::string::npos) {
            return s.substr(start, end - start + 1);
        }
        std::cout << "  [!] Input cannot be empty. Please try again.\n";
    }
}

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                       CORE ESTIMATION LOGIC                              ║
// ╚══════════════════════════════════════════════════════════════════════════╝

/**
 * Estimate the render output file size in Gigabytes.
 *
 * Formula:
 *   effectiveMbps = baseMbps × resolutionScale × (fps / BASE_FPS)
 *   totalBits     = effectiveMbps × 1,000,000 × durationSeconds
 *   sizeGB        = totalBits / (8 × BYTES_PER_GIGABYTE)
 *
 * @param baseMbps        Codec base bitrate at 1080p 30 fps
 * @param resolutionScale Multiplier from RESOLUTION_SCALE table
 * @param fps             Target framerate in frames per second
 * @param durationMins    Project duration in minutes
 * @return Estimated size in GiB
 */
double estimateFileSizeGB(double baseMbps,
                          double resolutionScale,
                          double fps,
                          double durationMins) {
    double effectiveMbps    = baseMbps * resolutionScale * (fps / BASE_FPS);
    double durationSeconds  = durationMins * 60.0;
    double totalBits        = effectiveMbps * BITS_PER_MEGABIT * durationSeconds;
    double sizeBytes        = totalBits / 8.0;
    return sizeBytes / BYTES_PER_GIGABYTE;
}

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                       FILESYSTEM INTEGRATION                             ║
// ╚══════════════════════════════════════════════════════════════════════════╝

/**
 * Query OS-level free disk space for a given path using C++17 <filesystem>.
 *
 * std::filesystem::space() returns a space_info struct with three fields:
 *   .capacity  — total drive capacity
 *   .free      — total free bytes (including reserved blocks)
 *   .available — bytes available to normal (non-root) processes  ← we use this
 *
 * @param path  Any path on the target volume (defaults to current directory ".")
 * @return      Available bytes, or -1 on error
 */
long long getAvailableSpaceBytes(const std::string& path = ".") {
    std::error_code ec;
    fs::space_info si = fs::space(path, ec);
    if (ec) {
        return -1LL;  // Signal failure; caller handles the error
    }
    return static_cast<long long>(si.available);
}

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                         DISPLAY / OUTPUT HELPERS                         ║
// ╚══════════════════════════════════════════════════════════════════════════╝

/** Print the application banner. */
void printBanner() {
    printRule('═');
    std::cout
        << "  ██╗   ██╗██████╗ ███████╗███████╗\n"
        << "  ██║   ██║██╔══██╗██╔════╝██╔════╝\n"
        << "  ██║   ██║██████╔╝███████╗█████╗  \n"
        << "  ╚██╗ ██╔╝██╔══██╗╚════██║██╔══╝  \n"
        << "   ╚████╔╝ ██║  ██║███████║███████╗\n"
        << "    ╚═══╝  ╚═╝  ╚═╝╚══════╝╚══════╝\n\n"
        << "       Video Render Storage Estimator\n"
        << "  Stop renders from failing at 90%!\n";
    printRule('═');
    std::cout << '\n';
}

/** Print supported codecs for user reference. */
void printCodecMenu() {
    std::cout << "\n  Supported export codecs:\n";
    std::cout << "  " << std::string(40, '-') << '\n';
    for (const auto& [key, info] : CODEC_TABLE) {
        std::cout << "    " << std::left << std::setw(14) << key
                  << " → " << info.displayName
                  << "  (~" << info.baseMbps << " Mbps @ 1080p 30fps)\n";
    }
    std::cout << "  " << std::string(40, '-') << '\n';
}

/** Print supported resolutions for user reference. */
void printResolutionMenu() {
    std::cout << "\n  Supported resolutions:\n";
    std::cout << "  " << std::string(30, '-') << '\n';
    for (const auto& [key, scale] : RESOLUTION_SCALE) {
        std::cout << "    " << std::left << std::setw(10) << key
                  << "  (scale: " << std::fixed << std::setprecision(2)
                  << scale << "x)\n";
    }
    std::cout << "  " << std::string(30, '-') << '\n';
}

/**
 * Display the final analysis card — estimated size vs available space.
 *
 * EXIT CODES (conceptual):
 *   SUCCESS  — plenty of space
 *   WARNING  — tight (within safety margin) but technically enough
 *   DANGER   — not enough space; render will likely fail
 */
void printResult(double estimatedGB,
                 double availableGB,
                 const std::string& resolution,
                 double fps,
                 double durationMins,
                 const std::string& codecName,
                 const std::string& outputPath) {

    std::cout << '\n';
    printRule('═');
    std::cout << "  ESTIMATION REPORT\n";
    printRule('═');

    // ── Project Summary ──────────────────────────────────────────────────
    std::cout << std::fixed << std::setprecision(2);
    std::cout
        << "\n  Project Parameters:\n"
        << "    Resolution   : " << resolution << '\n'
        << "    Framerate    : " << fps        << " fps\n"
        << "    Duration     : " << durationMins << " min ("
                                 << (durationMins * 60.0) << " sec)\n"
        << "    Export Codec : " << codecName  << '\n'
        << "    Check Drive  : " << outputPath << "\n\n";

    printRule();

    // ── Size Comparison ──────────────────────────────────────────────────
    std::cout
        << "  Estimated file size   : " << estimatedGB  << " GB\n"
        << "  Available disk space  : " << availableGB  << " GB\n"
        << "  Headroom after render : "
        << (availableGB - estimatedGB) << " GB\n";

    printRule();

    // ── Verdict ──────────────────────────────────────────────────────────
    if (availableGB < estimatedGB) {
        // ❌ DANGER — will definitely fail
        std::cout
            << "\n  ❌  DANGER — NOT ENOUGH SPACE!\n\n"
            << "      You need " << estimatedGB  << " GB but only "
            << availableGB << " GB is available.\n"
            << "      Shortfall: " << (estimatedGB - availableGB) << " GB\n\n"
            << "  ➜  Recommended actions:\n"
            << "       • Free up space on the target drive.\n"
            << "       • Choose a higher-compression codec (e.g., H.265).\n"
            << "       • Reduce resolution or shorten the project.\n"
            << "       • Render to an external drive with more room.\n";

    } else if (availableGB < (estimatedGB / SAFETY_MARGIN)) {
        // ⚠ WARNING — technically enough but dangerously close
        double percentUsed = (estimatedGB / availableGB) * 100.0;
        std::cout
            << "\n  ⚠   WARNING — SPACE IS TIGHT!\n\n"
            << "      The render will use ~" << percentUsed << "% of your"
            << " free space.\n"
            << "      DaVinci Resolve may also write temporary scratch files.\n\n"
            << "  ➜  Recommended actions:\n"
            << "       • Close unnecessary applications to free RAM/disk.\n"
            << "       • Delete old project exports from the drive first.\n"
            << "       • Proceed with caution — monitor space during render.\n";

    } else {
        // ✅ SUCCESS — clear to render
        double percentUsed = (estimatedGB / availableGB) * 100.0;
        std::cout
            << "\n  ✅  SUCCESS — YOU'RE CLEAR TO RENDER!\n\n"
            << "      The render will use ~" << percentUsed << "% of your"
            << " free space.\n"
            << "      You'll have " << (availableGB - estimatedGB)
            << " GB remaining after the render.\n\n"
            << "  ➜  Go ahead and hit that Render button in DaVinci Resolve!\n";
    }

    printRule('═');
    std::cout << '\n';
}

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                              MAIN ENTRY POINT                            ║
// ╚══════════════════════════════════════════════════════════════════════════╝

int main() {
    // ── 1. Banner ─────────────────────────────────────────────────────────
    printBanner();
    std::cout
        << "  This tool estimates the output file size of your DaVinci Resolve\n"
        << "  project and checks whether your drive has enough space before you\n"
        << "  start a long render.\n\n";

    // ── 2. Resolution Input ───────────────────────────────────────────────
    printResolutionMenu();
    std::string resInput = readString("\n  Enter resolution (e.g., 1080p, 4K): ");
    std::string resKey   = normaliseKey(resInput);

    // Aliases: users often type "4k" or "4K UHD"
    if (resKey == "4kuhd" || resKey == "uhd4k") resKey = "4k";

    auto resIt = RESOLUTION_SCALE.find(resKey);
    while (resIt == RESOLUTION_SCALE.end()) {
        std::cout << "  [!] Resolution '" << resInput
                  << "' not recognised. Please choose from the list above.\n";
        resInput = readString("  Enter resolution: ");
        resKey   = normaliseKey(resInput);
        resIt    = RESOLUTION_SCALE.find(resKey);
    }
    double resScale = resIt->second;

    // ── 3. Framerate Input ────────────────────────────────────────────────
    double fps = readPositiveDouble("\n  Enter framerate (fps, e.g., 24, 30, 60): ");

    // ── 4. Duration Input ─────────────────────────────────────────────────
    double durationMins = readPositiveDouble(
        "\n  Enter project duration in minutes (e.g., 3.5 for 3 min 30 sec): ");

    // ── 5. Codec Input ────────────────────────────────────────────────────
    printCodecMenu();
    std::string codecInput = readString("\n  Enter export codec (e.g., h264, prores422): ");
    std::string codecKey   = normaliseKey(codecInput);

    auto codecIt = CODEC_TABLE.find(codecKey);
    while (codecIt == CODEC_TABLE.end()) {
        std::cout << "  [!] Codec '" << codecInput
                  << "' not recognised. Please choose from the list above.\n";
        codecInput = readString("  Enter export codec: ");
        codecKey   = normaliseKey(codecInput);
        codecIt    = CODEC_TABLE.find(codecKey);
    }
    const CodecInfo& codec = codecIt->second;

    // ── 6. Output Path (optional) ─────────────────────────────────────────
    std::cout << "\n  Enter the output directory path to check free space on.\n"
              << "  Press [Enter] to use the current directory ('.'):\n";
    std::cout << "  Path: ";
    std::string outputPath;
    std::getline(std::cin, outputPath);
    if (outputPath.empty() || outputPath.find_first_not_of(" \t") == std::string::npos) {
        outputPath = ".";
    }

    // ── 7. Disk Space Query via <filesystem> ──────────────────────────────
    long long availableBytes = getAvailableSpaceBytes(outputPath);
    if (availableBytes < 0LL) {
        std::cerr << "\n  [ERROR] Could not query disk space for path: '"
                  << outputPath << "'.\n"
                  << "  Please ensure the path exists and you have read permission.\n\n";
        return 1;
    }
    double availableGB = static_cast<double>(availableBytes) / BYTES_PER_GIGABYTE;

    // ── 8. Estimation ─────────────────────────────────────────────────────
    double estimatedGB = estimateFileSizeGB(codec.baseMbps,
                                            resScale,
                                            fps,
                                            durationMins);

    // ── 9. Output Result ──────────────────────────────────────────────────
    printResult(estimatedGB,
                availableGB,
                resInput,
                fps,
                durationMins,
                codec.displayName,
                outputPath);

    // ── 10. Another Estimate? ─────────────────────────────────────────────
    std::cout << "  Run another estimate? (y/n): ";
    char again;
    std::cin >> again;
    if (again == 'y' || again == 'Y') {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "\n\n";
        // Restart by calling main recursively (acceptable for a small CLI tool)
        return main();
    }

    std::cout << "\n  Happy editing! 🎬\n\n";
    return 0;
}
