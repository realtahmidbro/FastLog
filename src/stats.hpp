#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace fastlog {

// Muss auf 64 bytes aligned sein wegen cache line bouncing bei multithreading
struct alignas(64) ChunkStats {
    uint64_t lines = 0;
    uint64_t empty_lines = 0;
    uint64_t line_length_sum = 0;
    uint64_t max_line_length = 0;
    uint64_t ascii_chars = 0;
    uint64_t non_ascii_chars = 0;
    uint64_t crlf_count = 0;
    uint64_t lf_count = 0;
    std::array<uint64_t, 7> delimiters = {0}; // die üblichen verdächtigen: , ; : = \t | space
    
    void merge(const ChunkStats& other) noexcept {
        lines += other.lines;
        empty_lines += other.empty_lines;
        line_length_sum += other.line_length_sum;
        if (other.max_line_length > max_line_length) 
            max_line_length = other.max_line_length;
        ascii_chars += other.ascii_chars;
        non_ascii_chars += other.non_ascii_chars;
        crlf_count += other.crlf_count;
        lf_count += other.lf_count;
        for (int i = 0; i < 7; i++) delimiters[i] += other.delimiters[i];
    }
};

struct FileStats {
    std::string filename;
    uint64_t total_bytes = 0;
    ChunkStats stats;
    bool has_bom = false;
    uint8_t encoding_type = 0;
    
    double empty_line_percent() const noexcept {
        if (stats.lines == 0) return 0.0;
        return (static_cast<double>(stats.empty_lines) / stats.lines) * 100.0;
    }
    
    double avg_line_length() const noexcept {
        if (stats.lines == 0) return 0.0;
        return static_cast<double>(stats.line_length_sum) / stats.lines;
    }
    
    double ascii_ratio() const noexcept {
        uint64_t total = stats.ascii_chars + stats.non_ascii_chars;
        if (total == 0) return 100.0;
        return (static_cast<double>(stats.ascii_chars) / total) * 100.0;
    }
    
    const char* line_ending_str() const noexcept {
        if (stats.crlf_count >= stats.lf_count) 
            return stats.crlf_count == 0 ? "N/A" : "CRLF";
        return "LF";
    }
    
    const char* encoding_str() const noexcept {
        switch (encoding_type) {
            case 0: return "ASCII";
            case 1: return "UTF-8";
            case 2: return "UTF-8 (BOM)";
            case 3: return "UTF-16 LE";
            case 4: return "UTF-16 BE";
            default: return "Unknown";
        }
    }
};

struct AggregateStats {
    uint64_t total_files = 0;
    uint64_t total_bytes = 0;
    ChunkStats stats;
    
    void add(const FileStats& fs) noexcept {
        total_files++;
        total_bytes += fs.total_bytes;
        stats.merge(fs.stats);
    }
    
    double empty_line_percent() const noexcept {
        if (stats.lines == 0) return 0.0;
        return (static_cast<double>(stats.empty_lines) / stats.lines) * 100.0;
    }
    
    double avg_line_length() const noexcept {
        if (stats.lines == 0) return 0.0;
        return static_cast<double>(stats.line_length_sum) / stats.lines;
    }
    
    double ascii_ratio() const noexcept {
        uint64_t total = stats.ascii_chars + stats.non_ascii_chars;
        if (total == 0) return 100.0;
        return (static_cast<double>(stats.ascii_chars) / total) * 100.0;
    }
};

} // namespace fastlog
