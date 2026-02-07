# FastLog

Text file analyzer. Counts lines, bytes, delimiters, detects encoding. Runs at ~1.4 GB/s on NVMe.

```text
$ fastlog server.log

file=server.log
lines=1,000,001
bytes=89.65 MB
empty_lines=0.0%
avg_line_length=93.0
max_line_length=96
encoding=UTF-8 (BOM)
line_ending=LF
top_delimiters: '=' ',' ':'
ascii_ratio=100.00%
```

## Installation

**Windows:** Run installer from `dist/`. Done. No dependencies.

**From source:**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Requires C++17 and CMake 3.16+.

## Usage

```text
fastlog <file>              # Analyze single file
fastlog <directory>         # Analyze all text files recursively
fastlog <directory> --flat  # Analyze directory without subdirectories
fastlog -h                  # Show help
fastlog -v                  # Show version
fastlog --update            # Check for updates
```

### Options

| Option | Description |
| ------ | ----------- |
| `-h`, `--help` | Show usage information |
| `-v`, `--version` | Show version number |
| `--update` | Check GitHub for new releases |
| `--flat` | Disable recursive directory scanning |

## How it works

### Memory-mapped I/O

No `fread()`, no `std::ifstream`. The file gets mapped directly into address space:

```cpp
// Windows
CreateFileW(..., FILE_FLAG_SEQUENTIAL_SCAN, ...);
CreateFileMappingW(...);
MapViewOfFile(...);

// Linux
mmap(..., MAP_PRIVATE, ...);
madvise(..., MADV_SEQUENTIAL | MADV_WILLNEED);
```

OS handles prefetching, no copy operations needed.

### SIMD character counting

Instead of checking bytes one by one, we process 32 bytes at a time with AVX2:

```cpp
__m256i v = _mm256_loadu_si256(data + i);
__m256i cmp = _mm256_cmpeq_epi8(v, vnewline);
// cmp is 0xFF for matches, 0x00 otherwise
acc_nl = _mm256_sub_epi8(acc_nl, cmp);
```

After 255 iterations (8-bit overflow), horizontal sum:

```cpp
__m256i sad = _mm256_sad_epu8(acc, _mm256_setzero_si256());
// 4x 64-bit sums, add them up
```

SSE2 fallback for older CPUs. Every x86-64 has SSE2.

### Runtime dispatch

```cpp
inline bool cpu_has_avx2() noexcept {
    static int cached = -1;
    if (cached >= 0) return cached;
    
#ifdef _WIN32
    int info[4];
    __cpuid(info, 0);
    if (info[0] >= 7) {
        __cpuidex(info, 7, 0);
        cached = (info[1] & (1 << 5)) ? 1 : 0;
    }
#else
    cached = __builtin_cpu_supports("avx2") ? 1 : 0;
#endif
    return cached;
}
```

Functions use `__attribute__((target("avx2")))` / `target("sse2")` so GCC compiles both versions into one binary.

### Threading

For files >4 MB:

1. SIMD counts all characters (single-threaded, I/O bound anyway)
2. Find all newline positions
3. Calculate line lengths in parallel

```cpp
std::atomic<size_t> next_chunk{0};

auto worker = [&]() {
    while (true) {
        size_t idx = next_chunk.fetch_add(1, std::memory_order_relaxed);
        if (idx >= num_chunks) break;
        // process chunk
    }
};

for (int i = 1; i < num_threads; i++)
    threads.emplace_back(worker);
worker();  // main thread works too
for (auto& t : threads) t.join();
```

Work stealing via atomics. No fixed chunk assignment, threads grab work until done.

### Directory scanning

`std::filesystem` is slow. Multiple syscalls per file. On Windows we use:

```cpp
FindFirstFileExW(
    path,
    FindExInfoBasic,           // skip 8.3 name lookup
    &find_data,
    FindExSearchNameMatch,
    nullptr,
    FIND_FIRST_EX_LARGE_FETCH  // batch fetching
);
```

Matters when you have 5000+ files.

## What gets measured

| Metric          | How                                          | Accuracy                    |
| --------------- | -------------------------------------------- | --------------------------- |
| lines           | count `\n`, +1 if file doesn't end with `\n` | exact                       |
| bytes           | file size from OS                            | exact                       |
| empty_lines     | lines with length 0 after CRLF strip         | exact                       |
| avg_line_length | sum / count                                  | exact                       |
| max_line_length | max over all lines                           | exact                       |
| encoding        | BOM detection, else ASCII/UTF-8 guess        | ~95%, can't detect Latin-1  |
| line_ending     | CRLF if `\r` before `\n`, else LF            | exact                       |
| top_delimiters  | count `, ; : = \t \| space`                  | exact counts                |
| ascii_ratio     | ascii_chars / total * 100                    | exact                       |

## Performance

Tested on Kingston NV1 500GB (1812 MB/s sequential read):

| File   | Time   | Throughput |
| ------ | ------ | ---------- |
| 90 MB  | 78 ms  | 1.15 GB/s  |
| 448 MB | 320 ms | 1.40 GB/s  |

That's 77% of raw SSD speed. Rest is NTFS overhead, page faults, actual processing.

## Files

```text
src/
├── main.cpp       CLI, argument parsing, output formatting
├── analyzer.cpp   memory mapping, threading, directory scanning
├── analyzer.hpp   public API
├── stats.hpp      data structures (ChunkStats, FileStats, etc.)
└── simd.hpp       AVX2/SSE2 character counting
```

## Requirements

- C++17
- CMake 3.16+
- x86-64 (SSE2 minimum, AVX2 for full speed)

## License

MIT

---

Built this because I got tired of waiting for slow log parsers. Turns out when you stop abstracting everything and just let the CPU do what it's good at, things get fast. Who knew.

— AGDNoob
