#pragma once

#include "stats.hpp"
#include <filesystem>
#include <vector>

namespace fastlog {

class Analyzer {
public:
    static FileStats analyze_file(const std::filesystem::path& filepath);
    
    static std::vector<FileStats> analyze_directory(
        const std::filesystem::path& dirpath, 
        bool recursive = true
    );
    
    static AggregateStats aggregate(const std::vector<FileStats>& file_stats) noexcept;
    
private:
    static bool is_text_file(const std::filesystem::path& filepath);
};

} // namespace fastlog
