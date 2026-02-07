cmake_minimum_required(VERSION 3.16)
project(fastlog VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Volle Optimierung aber ohne march=native damit es überall läuft
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto -fomit-frame-pointer -funroll-loops")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto -s")

# LTO anmachen falls der compiler das kann - macht nochmal paar % schneller
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

add_executable(fastlog
    src/main.cpp
    src/analyzer.cpp
)

target_include_directories(fastlog PRIVATE src)

# Windows braucht wininet für Update-Check
if(WIN32)
    target_link_libraries(fastlog PRIVATE wininet)
endif()

# Für cmake install falls jemand das braucht
install(TARGETS fastlog DESTINATION bin)
