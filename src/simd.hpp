#pragma once

#include <cstdint>
#include <cstddef>
#include <immintrin.h>

#ifdef _WIN32
    #include <intrin.h>
#endif

namespace fastlog::simd {

struct CharCounts {
    uint64_t newlines = 0;
    uint64_t carriage_returns = 0;
    uint64_t commas = 0;
    uint64_t semicolons = 0;
    uint64_t colons = 0;
    uint64_t equals = 0;
    uint64_t tabs = 0;
    uint64_t pipes = 0;
    uint64_t spaces = 0;
    uint64_t non_ascii = 0;
};

// Checkt zur runtime ob die CPU AVX2 kann - cached weil sonst jedes mal cpuid aufrufen wär dumm
inline bool cpu_has_avx2() noexcept {
    static int cached = -1;
    if (cached >= 0) return cached;
    
#ifdef _WIN32
    int info[4];
    __cpuid(info, 0);
    if (info[0] >= 7) {
        __cpuidex(info, 7, 0);
        cached = (info[1] & (1 << 5)) ? 1 : 0;
    } else {
        cached = 0;
    }
#else
    __builtin_cpu_init();
    cached = __builtin_cpu_supports("avx2") ? 1 : 0;
#endif
    return cached;
}

// ============================================================================
// Brauch eigene function weil lambdas das target attribute nicht erben... compiler halt
// ============================================================================

__attribute__((target("avx2")))
inline uint64_t hsum_avx2(__m256i v) noexcept {
    __m256i sad = _mm256_sad_epu8(v, _mm256_setzero_si256());
    return static_cast<uint64_t>(_mm256_extract_epi64(sad, 0)) + 
           static_cast<uint64_t>(_mm256_extract_epi64(sad, 1)) +
           static_cast<uint64_t>(_mm256_extract_epi64(sad, 2)) + 
           static_cast<uint64_t>(_mm256_extract_epi64(sad, 3));
}

// ============================================================================
// Die schnelle version - ballert 32 bytes auf einmal durch, aber nur wenn CPU das kann
// ============================================================================

__attribute__((target("avx2")))
inline CharCounts count_chars_avx2_impl(const char* data, size_t len) noexcept {
    CharCounts result = {};
    
    const __m256i vnewline = _mm256_set1_epi8('\n');
    const __m256i vcr = _mm256_set1_epi8('\r');
    const __m256i vcomma = _mm256_set1_epi8(',');
    const __m256i vsemi = _mm256_set1_epi8(';');
    const __m256i vcolon = _mm256_set1_epi8(':');
    const __m256i vequals = _mm256_set1_epi8('=');
    const __m256i vtab = _mm256_set1_epi8('\t');
    const __m256i vpipe = _mm256_set1_epi8('|');
    const __m256i vspace = _mm256_set1_epi8(' ');
    const __m256i v127 = _mm256_set1_epi8(127);
    
    size_t i = 0;
    
    __m256i acc_nl = _mm256_setzero_si256();
    __m256i acc_cr = _mm256_setzero_si256();
    __m256i acc_comma = _mm256_setzero_si256();
    __m256i acc_semi = _mm256_setzero_si256();
    __m256i acc_colon = _mm256_setzero_si256();
    __m256i acc_eq = _mm256_setzero_si256();
    __m256i acc_tab = _mm256_setzero_si256();
    __m256i acc_pipe = _mm256_setzero_si256();
    __m256i acc_space = _mm256_setzero_si256();
    __m256i acc_nonascii = _mm256_setzero_si256();
    
    int batch = 0;
    
    for (; i + 32 <= len; i += 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        
        acc_nl = _mm256_sub_epi8(acc_nl, _mm256_cmpeq_epi8(v, vnewline));
        acc_cr = _mm256_sub_epi8(acc_cr, _mm256_cmpeq_epi8(v, vcr));
        acc_comma = _mm256_sub_epi8(acc_comma, _mm256_cmpeq_epi8(v, vcomma));
        acc_semi = _mm256_sub_epi8(acc_semi, _mm256_cmpeq_epi8(v, vsemi));
        acc_colon = _mm256_sub_epi8(acc_colon, _mm256_cmpeq_epi8(v, vcolon));
        acc_eq = _mm256_sub_epi8(acc_eq, _mm256_cmpeq_epi8(v, vequals));
        acc_tab = _mm256_sub_epi8(acc_tab, _mm256_cmpeq_epi8(v, vtab));
        acc_pipe = _mm256_sub_epi8(acc_pipe, _mm256_cmpeq_epi8(v, vpipe));
        acc_space = _mm256_sub_epi8(acc_space, _mm256_cmpeq_epi8(v, vspace));
        acc_nonascii = _mm256_sub_epi8(acc_nonascii, _mm256_cmpgt_epi8(v, v127));
        
        if (++batch >= 255) {
            result.newlines += hsum_avx2(acc_nl);
            result.carriage_returns += hsum_avx2(acc_cr);
            result.commas += hsum_avx2(acc_comma);
            result.semicolons += hsum_avx2(acc_semi);
            result.colons += hsum_avx2(acc_colon);
            result.equals += hsum_avx2(acc_eq);
            result.tabs += hsum_avx2(acc_tab);
            result.pipes += hsum_avx2(acc_pipe);
            result.spaces += hsum_avx2(acc_space);
            result.non_ascii += hsum_avx2(acc_nonascii);
            
            acc_nl = acc_cr = acc_comma = acc_semi = acc_colon = _mm256_setzero_si256();
            acc_eq = acc_tab = acc_pipe = acc_space = acc_nonascii = _mm256_setzero_si256();
            batch = 0;
        }
    }
    
    if (batch > 0) {
        result.newlines += hsum_avx2(acc_nl);
        result.carriage_returns += hsum_avx2(acc_cr);
        result.commas += hsum_avx2(acc_comma);
        result.semicolons += hsum_avx2(acc_semi);
        result.colons += hsum_avx2(acc_colon);
        result.equals += hsum_avx2(acc_eq);
        result.tabs += hsum_avx2(acc_tab);
        result.pipes += hsum_avx2(acc_pipe);
        result.spaces += hsum_avx2(acc_space);
        result.non_ascii += hsum_avx2(acc_nonascii);
    }
    
    for (; i < len; i++) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        switch (c) {
            case '\n': result.newlines++; break;
            case '\r': result.carriage_returns++; break;
            case ',': result.commas++; break;
            case ';': result.semicolons++; break;
            case ':': result.colons++; break;
            case '=': result.equals++; break;
            case '\t': result.tabs++; break;
            case '|': result.pipes++; break;
            case ' ': result.spaces++; break;
        }
        if (c > 127) result.non_ascii++;
    }
    
    return result;
}

__attribute__((target("avx2")))
inline size_t find_newlines_avx2_impl(const char* data, size_t len, size_t* positions, size_t max_pos) noexcept {
    size_t count = 0;
    const __m256i vnewline = _mm256_set1_epi8('\n');
    
    size_t i = 0;
    for (; i + 32 <= len && count < max_pos; i += 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        uint32_t mask = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, vnewline)));
        
        while (mask && count < max_pos) {
            int bit = __builtin_ctz(mask);
            positions[count++] = i + bit;
            mask &= mask - 1;
        }
    }
    
    for (; i < len && count < max_pos; i++) {
        if (data[i] == '\n') positions[count++] = i;
    }
    
    return count;
}

// ============================================================================
// Fallback für ältere CPUs - SSE2 hat quasi jeder x86-64, also safe
// ============================================================================

__attribute__((target("sse2")))
inline CharCounts count_chars_sse2_impl(const char* data, size_t len) noexcept {
    CharCounts result = {};
    
    const __m128i vnewline = _mm_set1_epi8('\n');
    const __m128i vcr = _mm_set1_epi8('\r');
    const __m128i vcomma = _mm_set1_epi8(',');
    const __m128i vsemi = _mm_set1_epi8(';');
    const __m128i vcolon = _mm_set1_epi8(':');
    const __m128i vequals = _mm_set1_epi8('=');
    const __m128i vtab = _mm_set1_epi8('\t');
    const __m128i vpipe = _mm_set1_epi8('|');
    const __m128i vspace = _mm_set1_epi8(' ');
    const __m128i v127 = _mm_set1_epi8(127);
    
    size_t i = 0;
    
    for (; i + 64 <= len; i += 64) {
        for (int j = 0; j < 4; j++) {
            __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i + j*16));
            
            result.newlines += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vnewline)));
            result.carriage_returns += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vcr)));
            result.commas += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vcomma)));
            result.semicolons += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vsemi)));
            result.colons += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vcolon)));
            result.equals += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vequals)));
            result.tabs += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vtab)));
            result.pipes += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vpipe)));
            result.spaces += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vspace)));
            result.non_ascii += __builtin_popcount(_mm_movemask_epi8(_mm_cmpgt_epi8(v, v127)));
        }
    }
    
    for (; i + 16 <= len; i += 16) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        
        result.newlines += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vnewline)));
        result.carriage_returns += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vcr)));
        result.commas += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vcomma)));
        result.semicolons += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vsemi)));
        result.colons += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vcolon)));
        result.equals += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vequals)));
        result.tabs += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vtab)));
        result.pipes += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vpipe)));
        result.spaces += __builtin_popcount(_mm_movemask_epi8(_mm_cmpeq_epi8(v, vspace)));
        result.non_ascii += __builtin_popcount(_mm_movemask_epi8(_mm_cmpgt_epi8(v, v127)));
    }
    
    for (; i < len; i++) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        switch (c) {
            case '\n': result.newlines++; break;
            case '\r': result.carriage_returns++; break;
            case ',': result.commas++; break;
            case ';': result.semicolons++; break;
            case ':': result.colons++; break;
            case '=': result.equals++; break;
            case '\t': result.tabs++; break;
            case '|': result.pipes++; break;
            case ' ': result.spaces++; break;
        }
        if (c > 127) result.non_ascii++;
    }
    
    return result;
}

__attribute__((target("sse2")))
inline size_t find_newlines_sse2_impl(const char* data, size_t len, size_t* positions, size_t max_pos) noexcept {
    size_t count = 0;
    const __m128i vnewline = _mm_set1_epi8('\n');
    
    size_t i = 0;
    for (; i + 16 <= len && count < max_pos; i += 16) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(v, vnewline));
        
        while (mask && count < max_pos) {
            int bit = __builtin_ctz(mask);
            positions[count++] = i + bit;
            mask &= mask - 1;
        }
    }
    
    for (; i < len && count < max_pos; i++) {
        if (data[i] == '\n') positions[count++] = i;
    }
    
    return count;
}

// ============================================================================
// Guckt welche SIMD version die CPU kann und ruft die richtige auf
// ============================================================================

inline CharCounts count_chars(const char* data, size_t len) noexcept {
    if (cpu_has_avx2()) {
        return count_chars_avx2_impl(data, len);
    }
    return count_chars_sse2_impl(data, len);
}

inline size_t find_newlines(const char* data, size_t len, size_t* positions, size_t max_pos) noexcept {
    if (cpu_has_avx2()) {
        return find_newlines_avx2_impl(data, len, positions, max_pos);
    }
    return find_newlines_sse2_impl(data, len, positions, max_pos);
}

} // namespace fastlog::simd
