# CMake最低版本号要求
cmake_minimum_required(VERSION 3.10)

# 配置项目名
project(simpleuniquepointer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Options ─────────────────────────────────────────────────
option(SIMPLE_UNIQUE_POINTER_BUILD_BENCHMARKS "Build benchmarks" ON)

# ── Sources ─────────────────────────────────────────────────
# UniquePointer<T> 是 header-only 模板，不需要 .cpp 源文件
set(SIMPLE_UNIQUE_POINTER_PUBLIC_HEADERS
  include/unique_pointer.h
)

# ── Library ─────────────────────────────────────────────────
add_library(simpleuniquepointer INTERFACE)
target_include_directories(simpleuniquepointer INTERFACE include)

# ── Benchmarks ────────────────────────────────────────────
if(SIMPLE_UNIQUE_POINTER_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()