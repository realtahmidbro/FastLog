#include "analyzer.hpp"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <wininet.h>
#endif

using namespace fastlog;

constexpr const char* VERSION = "1.0.0";
constexpr const char* GITHUB_REPO = "AGDNoob/FastLog";

constexpr const char* DELIM_NAMES[] = {"','", "';'", "':'", "'='", "'\\t'", "'|'", "' '"};

std::string format_number(uint64_t num) {
    if (num == 0) return "0";
    std::string result = std::to_string(num);
    int pos = static_cast<int>(result.length()) - 3;
    while (pos > 0) {
        result.insert(pos, ",");
        pos -= 3;
    }
    return result;
}

std::string format_bytes(uint64_t bytes) {
    constexpr const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int idx = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && idx < 4) { size /= 1024.0; idx++; }
    
    char buf[32];
    if (idx == 0) snprintf(buf, sizeof(buf), "%llu %s", (unsigned long long)bytes, units[0]);
    else snprintf(buf, sizeof(buf), "%.2f %s", size, units[idx]);
    return buf;
}

void print_top_delimiters(const std::array<uint64_t, 7>& counts) {
    std::array<std::pair<uint64_t, int>, 7> sorted;
    for (int i = 0; i < 7; i++) sorted[i] = {counts[i], i};
    std::partial_sort(sorted.begin(), sorted.begin() + 3, sorted.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });
    
    std::cout << "top_delimiters:";
    for (int i = 0; i < 3 && sorted[i].first > 0; i++) {
        std::cout << " " << DELIM_NAMES[sorted[i].second];
    }
    std::cout << "\n";
}

void print_stats(const FileStats& s) {
    std::cout << "file=" << s.filename << "\n"
              << "lines=" << format_number(s.stats.lines) << "\n"
              << "bytes=" << format_bytes(s.total_bytes) << "\n"
              << "empty_lines=" << std::fixed << std::setprecision(1) << s.empty_line_percent() << "%\n"
              << "avg_line_length=" << std::fixed << std::setprecision(1) << s.avg_line_length() << "\n"
              << "max_line_length=" << format_number(s.stats.max_line_length) << "\n"
              << "encoding=" << s.encoding_str() << "\n"
              << "line_ending=" << s.line_ending_str() << "\n";
    print_top_delimiters(s.stats.delimiters);
    std::cout << "ascii_ratio=" << std::fixed << std::setprecision(2) << s.ascii_ratio() << "%\n";
}

void print_aggregate(const AggregateStats& a) {
    std::cout << "=== AGGREGATE STATS ===\n"
              << "files=" << format_number(a.total_files) << "\n"
              << "total_lines=" << format_number(a.stats.lines) << "\n"
              << "total_bytes=" << format_bytes(a.total_bytes) << "\n"
              << "empty_lines=" << std::fixed << std::setprecision(1) << a.empty_line_percent() << "%\n"
              << "avg_line_length=" << std::fixed << std::setprecision(1) << a.avg_line_length() << "\n"
              << "max_line_length=" << format_number(a.stats.max_line_length) << "\n";
    print_top_delimiters(a.stats.delimiters);
    std::cout << "ascii_ratio=" << std::fixed << std::setprecision(2) << a.ascii_ratio() << "%\n";
}

void print_version() {
    std::cout << "FastLog v" << VERSION << "\n"
              << "Ultra High-Performance Text Analyzer\n"
              << "https://github.com/" << GITHUB_REPO << "\n";
}

#ifdef _WIN32
// Holt die neueste Version von GitHub API - returns empty string bei Fehler
std::string fetch_latest_version() {
    HINTERNET hInternet = InternetOpenA("FastLog", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!hInternet) return "";
    
    std::string url = "https://api.github.com/repos/";
    url += GITHUB_REPO;
    url += "/releases/latest";
    
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), 
        "Accept: application/vnd.github.v3+json\r\nUser-Agent: FastLog\r\n", -1,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
    
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }
    
    char buffer[4096];
    DWORD bytesRead;
    std::string response;
    
    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }
    
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    // Simpel "tag_name" parsen - kein JSON lib nötig
    size_t pos = response.find("\"tag_name\"");
    if (pos == std::string::npos) return "";
    
    pos = response.find("\"", pos + 10);
    if (pos == std::string::npos) return "";
    
    size_t end = response.find("\"", pos + 1);
    if (end == std::string::npos) return "";
    
    std::string tag = response.substr(pos + 1, end - pos - 1);
    // "v1.0.0" -> "1.0.0"
    if (!tag.empty() && tag[0] == 'v') tag = tag.substr(1);
    return tag;
}

void check_update() {
    std::cout << "Checking for updates...\n";
    std::string latest = fetch_latest_version();
    
    if (latest.empty()) {
        std::cerr << "Could not check for updates. Check your internet connection.\n";
        return;
    }
    
    if (latest == VERSION) {
        std::cout << "You're running the latest version (v" << VERSION << ")\n";
    } else {
        std::cout << "New version available: v" << latest << " (current: v" << VERSION << ")\n"
                  << "Download: https://github.com/" << GITHUB_REPO << "/releases/latest\n";
    }
}
#else
void check_update() {
    std::cout << "Update check: https://github.com/" << GITHUB_REPO << "/releases/latest\n";
}
#endif

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    
    if (argc >= 2) {
        if (std::strcmp(argv[1], "--version") == 0 || std::strcmp(argv[1], "-v") == 0) {
            print_version();
            return 0;
        }
        if (std::strcmp(argv[1], "--update") == 0) {
            check_update();
            return 0;
        }
    }
    
    if (argc < 2 || std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0) {
        std::cout << "FastLog v" << VERSION << " - Ultra High-Performance Text Analyzer\n\n"
                  << "Usage:\n"
                  << "  fastlog <file>              Analyze single file\n"
                  << "  fastlog <directory>         Analyze directory (recursive)\n"
                  << "  fastlog <directory> --flat  Non-recursive\n\n"
                  << "Options:\n"
                  << "  -h, --help      Show this help\n"
                  << "  -v, --version   Show version info\n"
                  << "  --update        Check for updates\n";
        return argc < 2 ? 1 : 0;
    }
    
    std::filesystem::path path(argv[1]);
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: Path does not exist: " << argv[1] << "\n";
        return 1;
    }
    
    bool recursive = !(argc >= 3 && std::strcmp(argv[2], "--flat") == 0);
    
    try {
        if (std::filesystem::is_regular_file(path)) {
            print_stats(Analyzer::analyze_file(path));
        } else if (std::filesystem::is_directory(path)) {
            auto stats = Analyzer::analyze_directory(path, recursive);
            if (stats.empty()) { std::cout << "No text files found.\n"; return 0; }
            for (const auto& s : stats) { print_stats(s); std::cout << "\n"; }
            if (stats.size() > 1) print_aggregate(Analyzer::aggregate(stats));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
