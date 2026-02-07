#include "analyzer.hpp"
#include "simd.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include <cstring>
#include <functional>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace fastlog {

namespace {

struct MappedFile {
    const char* data = nullptr;
    size_t size = 0;
#ifdef _WIN32
    HANDLE file_handle = INVALID_HANDLE_VALUE;
    HANDLE mapping_handle = nullptr;
#else
    int fd = -1;
#endif

    ~MappedFile() { close(); }
    MappedFile() = default;
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;
    
    bool open(const std::filesystem::path& path) {
#ifdef _WIN32
        file_handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (file_handle == INVALID_HANDLE_VALUE) return false;
        
        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(file_handle, &file_size)) {
            CloseHandle(file_handle);
            file_handle = INVALID_HANDLE_VALUE;
            return false;
        }
        size = static_cast<size_t>(file_size.QuadPart);
        if (size == 0) return true;
        
        mapping_handle = CreateFileMappingW(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping_handle) {
            CloseHandle(file_handle);
            file_handle = INVALID_HANDLE_VALUE;
            return false;
        }
        
        data = static_cast<const char*>(MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0));
        if (!data) {
            CloseHandle(mapping_handle);
            CloseHandle(file_handle);
            mapping_handle = nullptr;
            file_handle = INVALID_HANDLE_VALUE;
            return false;
        }
#else
        fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) return false;
        
        struct stat st;
        if (fstat(fd, &st) < 0) { ::close(fd); fd = -1; return false; }
        
        size = static_cast<size_t>(st.st_size);
        if (size == 0) return true;
        
        data = static_cast<const char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (data == MAP_FAILED) { data = nullptr; ::close(fd); fd = -1; return false; }
        madvise(const_cast<char*>(data), size, MADV_SEQUENTIAL | MADV_WILLNEED);
#endif
        return true;
    }
    
    void close() {
#ifdef _WIN32
        if (data) { UnmapViewOfFile(data); data = nullptr; }
        if (mapping_handle) { CloseHandle(mapping_handle); mapping_handle = nullptr; }
        if (file_handle != INVALID_HANDLE_VALUE) { CloseHandle(file_handle); file_handle = INVALID_HANDLE_VALUE; }
#else
        if (data && size > 0) { munmap(const_cast<char*>(data), size); data = nullptr; }
        if (fd >= 0) { ::close(fd); fd = -1; }
#endif
        size = 0;
    }
};

} // anonymous namespace

FileStats Analyzer::analyze_file(const std::filesystem::path& filepath) {
    FileStats result;
    result.filename = filepath.filename().string();
    
    MappedFile mf;
    if (!mf.open(filepath)) return result;
    
    result.total_bytes = mf.size;
    if (mf.size == 0) return result;
    
    // Gucken ob da ein BOM am Anfang ist - UTF-8, UTF-16 LE/BE etc
    const unsigned char* bom = reinterpret_cast<const unsigned char*>(mf.data);
    if (mf.size >= 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
        result.has_bom = true;
        result.encoding_type = 2;
    } else if (mf.size >= 2 && bom[0] == 0xFF && bom[1] == 0xFE) {
        result.has_bom = true;
        result.encoding_type = 3;
    } else if (mf.size >= 2 && bom[0] == 0xFE && bom[1] == 0xFF) {
        result.has_bom = true;
        result.encoding_type = 4;
    }
    
    // Unter 4MB lohnt sich multithreading nicht - overhead frisst den speedup auf
    constexpr size_t PARALLEL_THRESHOLD = 4 * 1024 * 1024;
    
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;
    
    if (mf.size < PARALLEL_THRESHOLD || num_threads == 1) {
        // Kleine files: single thread reicht, SIMD ballert trotzdem
        auto counts = simd::count_chars(mf.data, mf.size);
        
        result.stats.lf_count = counts.newlines;
        result.stats.crlf_count = 0;
        result.stats.delimiters[0] = counts.commas;
        result.stats.delimiters[1] = counts.semicolons;
        result.stats.delimiters[2] = counts.colons;
        result.stats.delimiters[3] = counts.equals;
        result.stats.delimiters[4] = counts.tabs;
        result.stats.delimiters[5] = counts.pipes;
        result.stats.delimiters[6] = counts.spaces;
        result.stats.non_ascii_chars = counts.non_ascii;
        result.stats.ascii_chars = mf.size - counts.non_ascii - counts.newlines;
        
        // Alle newlines finden und dann line lengths ausrechnen
        std::vector<size_t> nl_positions(counts.newlines + 1);
        size_t nl_count = simd::find_newlines(mf.data, mf.size, nl_positions.data(), counts.newlines + 1);
        
        result.stats.lines = nl_count;
        if (mf.data[mf.size - 1] != '\n') {
            result.stats.lines++; // letzte zeile hat kein newline am ende
        }
        
        size_t prev = 0;
        for (size_t i = 0; i < nl_count; i++) {
            size_t pos = nl_positions[i];
            size_t line_len = pos - prev;
            
            if (line_len > 0 && mf.data[pos - 1] == '\r') {
                result.stats.crlf_count++;
                line_len--;
            }
            
            result.stats.line_length_sum += line_len;
            if (line_len > result.stats.max_line_length) {
                result.stats.max_line_length = line_len;
            }
            if (line_len == 0) {
                result.stats.empty_lines++;
            }
            
            prev = pos + 1;
        }
        
        // Falls die datei nicht mit newline endet gibts noch ne letzte zeile
        if (prev < mf.size) {
            size_t line_len = mf.size - prev;
            result.stats.line_length_sum += line_len;
            if (line_len > result.stats.max_line_length) {
                result.stats.max_line_length = line_len;
            }
            if (line_len == 0) {
                result.stats.empty_lines++;
            }
        }
        
        result.stats.lf_count = nl_count - result.stats.crlf_count;
        
    } else {
        // Große files: multi-threading anschmeißen, jeder thread kriegt n stück
        
        // Erstmal alle chars zählen mit SIMD - geht am schnellsten
        auto counts = simd::count_chars(mf.data, mf.size);
        
        // Jetzt alle newline positionen sammeln damit wir die arbeit aufteilen können
        std::vector<size_t> nl_positions(counts.newlines + 1);
        size_t nl_count = simd::find_newlines(mf.data, mf.size, nl_positions.data(), counts.newlines + 1);
        
        // Die char counts schon mal abspeichern - das war der einfache teil
        result.stats.delimiters[0] = counts.commas;
        result.stats.delimiters[1] = counts.semicolons;
        result.stats.delimiters[2] = counts.colons;
        result.stats.delimiters[3] = counts.equals;
        result.stats.delimiters[4] = counts.tabs;
        result.stats.delimiters[5] = counts.pipes;
        result.stats.delimiters[6] = counts.spaces;
        result.stats.non_ascii_chars = counts.non_ascii;
        result.stats.ascii_chars = mf.size - counts.non_ascii - counts.newlines;
        
        result.stats.lines = nl_count;
        if (mf.size > 0 && mf.data[mf.size - 1] != '\n') {
            result.stats.lines++;
        }
        
        // Jetzt line lengths parallel ausrechnen - jeder thread kriegt ~gleich viele lines
        size_t lines_per_thread = (nl_count + num_threads - 1) / num_threads;
        std::vector<ChunkStats> thread_stats(num_threads);
        std::vector<std::thread> threads;
        
        std::atomic<size_t> next_chunk{0};
        
        auto worker = [&]() {
            while (true) {
                size_t chunk_idx = next_chunk.fetch_add(1, std::memory_order_relaxed);
                if (chunk_idx >= num_threads) break;
                
                size_t start_line = chunk_idx * lines_per_thread;
                size_t end_line = std::min(start_line + lines_per_thread, nl_count);
                
                if (start_line >= nl_count) break;
                
                ChunkStats& local = thread_stats[chunk_idx];
                local.lines = end_line - start_line;
                
                size_t prev_pos = (start_line == 0) ? 0 : nl_positions[start_line - 1] + 1;
                
                for (size_t i = start_line; i < end_line; i++) {
                    size_t nl_pos = nl_positions[i];
                    size_t line_len = nl_pos - prev_pos;
                    
                    if (line_len > 0 && mf.data[nl_pos - 1] == '\r') {
                        local.crlf_count++;
                        line_len--;
                    }
                    
                    local.line_length_sum += line_len;
                    if (line_len > local.max_line_length) {
                        local.max_line_length = line_len;
                    }
                    if (line_len == 0) {
                        local.empty_lines++;
                    }
                    
                    prev_pos = nl_pos + 1;
                }
            }
        };
        
        for (unsigned int i = 1; i < num_threads; i++) {
            threads.emplace_back(worker);
        }
        worker(); // main thread macht auch mit statt nur rumzusitzen
        
        for (auto& t : threads) t.join();
        
        // Ergebnisse zusammenführen
        for (const auto& ts : thread_stats) {
            result.stats.crlf_count += ts.crlf_count;
            result.stats.empty_lines += ts.empty_lines;
            result.stats.line_length_sum += ts.line_length_sum;
            if (ts.max_line_length > result.stats.max_line_length) {
                result.stats.max_line_length = ts.max_line_length;
            }
        }
        
        result.stats.lf_count = nl_count - result.stats.crlf_count;
        
        // Noch checken ob letzte zeile ohne newline - kommt oft vor
        if (mf.size > 0 && mf.data[mf.size - 1] != '\n') {
            size_t prev = nl_count > 0 ? nl_positions[nl_count - 1] + 1 : 0;
            size_t line_len = mf.size - prev;
            result.stats.line_length_sum += line_len;
            if (line_len > result.stats.max_line_length) {
                result.stats.max_line_length = line_len;
            }
        }
    }
    
    // Encoding raten wenn kein BOM da ist - ASCII wenn nur ascii chars, sonst UTF-8
    if (!result.has_bom) {
        result.encoding_type = (result.stats.non_ascii_chars == 0) ? 0 : 1;
    }
    
    return result;
}

// Native directory scanning - viel schneller als std::filesystem bei vielen files
#ifdef _WIN32
namespace {

void scan_directory_native(
    const std::wstring& dir_path,
    bool recursive,
    std::vector<std::filesystem::path>& out_files,
    const std::function<bool(const std::wstring&)>& filter
) {
    std::wstring search_path = dir_path;
    if (!search_path.empty() && search_path.back() != L'\\' && search_path.back() != L'/') {
        search_path += L'\\';
    }
    search_path += L'*';
    
    WIN32_FIND_DATAW find_data;
    HANDLE find_handle = FindFirstFileExW(
        search_path.c_str(),
        FindExInfoBasic,           // Schneller - brauchen keinen short name
        &find_data,
        FindExSearchNameMatch,
        nullptr,
        FIND_FIRST_EX_LARGE_FETCH  // Batch fetching für mehr speed
    );
    
    if (find_handle == INVALID_HANDLE_VALUE) return;
    
    std::wstring base_path = dir_path;
    if (!base_path.empty() && base_path.back() != L'\\' && base_path.back() != L'/') {
        base_path += L'\\';
    }
    
    do {
        // Skip . und ..
        if (find_data.cFileName[0] == L'.') {
            if (find_data.cFileName[1] == L'\0') continue;
            if (find_data.cFileName[1] == L'.' && find_data.cFileName[2] == L'\0') continue;
        }
        
        std::wstring full_path = base_path + find_data.cFileName;
        
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Rekursiv wenn gewünscht
            if (recursive) {
                scan_directory_native(full_path, true, out_files, filter);
            }
        } else {
            // Reguläre file - check extension
            if (filter(full_path)) {
                out_files.emplace_back(full_path);
            }
        }
    } while (FindNextFileW(find_handle, &find_data));
    
    FindClose(find_handle);
}

} // anonymous namespace
#endif

std::vector<FileStats> Analyzer::analyze_directory(
    const std::filesystem::path& dirpath, 
    bool recursive
) {
    std::vector<std::filesystem::path> files;
    
#ifdef _WIN32
    // Native Windows API - deutlich schneller als std::filesystem
    auto filter = [](const std::wstring& path) -> bool {
        // Extension extrahieren
        size_t dot_pos = path.rfind(L'.');
        if (dot_pos == std::wstring::npos || dot_pos == path.length() - 1) {
            return false; // Keine extension = evtl. text file ohne ext
        }
        
        std::wstring ext = path.substr(dot_pos);
        // Lowercase
        for (wchar_t& c : ext) {
            if (c >= L'A' && c <= L'Z') c += 32;
        }
        
        // Die üblichen text extensions
        static const wchar_t* text_exts[] = {
            L".txt", L".log", L".csv", L".json", L".jsonl", L".xml", L".yaml", L".yml",
            L".md", L".markdown", L".ini", L".cfg", L".conf", L".config",
            L".tsv", L".ndjson", L".sql", L".sh", L".bash", L".ps1", L".bat", L".cmd",
            L".py", L".js", L".ts", L".cpp", L".c", L".h", L".hpp", L".java", L".cs",
            L".html", L".htm", L".css", L".scss", L".sass", L".less",
            L".env", L".gitignore", L".dockerignore", L".editorconfig"
        };
        
        for (const wchar_t* te : text_exts) {
            if (ext == te) return true;
        }
        return false;
    };
    
    files.reserve(1024); // Vorallokieren für weniger reallocs
    scan_directory_native(dirpath.wstring(), recursive, files, filter);
#else
    // Auf Linux/Mac bleibt std::filesystem erstmal - TODO: opendir/readdir
    try {
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dirpath)) {
                if (entry.is_regular_file() && is_text_file(entry.path())) {
                    files.push_back(entry.path());
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(dirpath)) {
                if (entry.is_regular_file() && is_text_file(entry.path())) {
                    files.push_back(entry.path());
                }
            }
        }
    } catch (...) {
        return {};
    }
#endif
    
    if (files.empty()) return {};
    
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;
    num_threads = std::min(num_threads, static_cast<unsigned int>(files.size()));
    
    std::vector<FileStats> results(files.size());
    std::atomic<size_t> next_file{0};
    
    auto worker = [&]() {
        while (true) {
            size_t idx = next_file.fetch_add(1, std::memory_order_relaxed);
            if (idx >= files.size()) break;
            results[idx] = analyze_file(files[idx]);
        }
    };
    
    std::vector<std::thread> threads;
    for (unsigned int i = 1; i < num_threads; i++) {
        threads.emplace_back(worker);
    }
    worker();
    
    for (auto& t : threads) t.join();
    
    return results;
}

AggregateStats Analyzer::aggregate(const std::vector<FileStats>& file_stats) noexcept {
    AggregateStats agg;
    for (const auto& fs : file_stats) agg.add(fs);
    return agg;
}

bool Analyzer::is_text_file(const std::filesystem::path& filepath) {
    static const char* text_extensions[] = {
        ".txt", ".log", ".csv", ".json", ".jsonl", ".xml", ".yaml", ".yml",
        ".md", ".markdown", ".ini", ".cfg", ".conf", ".config",
        ".tsv", ".ndjson", ".sql", ".sh", ".bash", ".ps1", ".bat", ".cmd",
        ".py", ".js", ".ts", ".cpp", ".c", ".h", ".hpp", ".java", ".cs",
        ".html", ".htm", ".css", ".scss", ".sass", ".less",
        ".env", ".gitignore", ".dockerignore", ".editorconfig", ""
    };
    
    std::string ext = filepath.extension().string();
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    
    for (const char* text_ext : text_extensions) {
        if (ext == text_ext) return true;
    }
    
    return false; // binary check skippen weil zu langsam - extension reicht
}

} // namespace fastlog
