cmake_minimum_required(VERSION 3.6)

# Preparation
find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

project(freedomdb VERSION 0.0.1 LANGUAGES CXX)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)
add_subdirectory(modules/fmt EXCLUDE_FROM_ALL)
add_subdirectory(modules/pybind11)

option(USE_CLANG "Use Clang compiler" OFF) # TODO
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(CLANG_FORMAT "Enable clang-format target" OFF)
option(CLANG_TIDY "Enable clang-tidy checks during compilation" OFF)
option(COVERAGE "Enable generation of coverage info" OFF)
option(ASAN "Enable address sanitizer" OFF)
option(LSAN "Enable leak sanitizer" OFF)
option(MSAN "Enable memory sanitizer" OFF)
option(TSAN "Enable thread sanitizer" OFF)
option(UBSAN "Enable UB sanitizer" OFF)

if(USE_CLANG)
  set(CMAKE_CXX_COMPILER "clang++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11 -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++ -lc++abi")
else()
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -flto")
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ version selection")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS -pthread)

add_library(freedomdb-static STATIC
  source/freedom_db.cpp
  source/db/db.cpp
  source/db/pool.cpp
  source/p2p/p2p.cpp
  source/p2p/p2p_msg.cpp
  source/core/log.cpp
)
include_directories(
  ${PROJECT_SOURCE_DIR}/source
)
include_directories(SYSTEM
  ${PROJECT_SOURCE_DIR}/modules/fmt/include
  ${PROJECT_SOURCE_DIR}/modules/procxx/include
  ${PROJECT_SOURCE_DIR}/modules/magic_enum/include
  ${PROJECT_SOURCE_DIR}/modules/msgpack/include
  ${PROJECT_SOURCE_DIR}/modules/cxxopts/include
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(freedomdb-static
  PUBLIC
  $<$<CONFIG:Release>:-Wl,-s>
  -Wl,--gc-sections,--exclude-libs,ALL
)
target_compile_definitions(freedomdb-static PRIVATE VERSION_INFO=${VERSION_INFO})
target_compile_options(freedomdb-static
  PUBLIC
  -fvisibility=hidden
  -ffast-math
  -fno-finite-math-only
  -ftree-vectorize
)
link_libraries(
  freedomdb-static
  ${PQXX_LIB} ${PQ_LIB}
  fmt::fmt
)

#### PYBIND11 library
pybind11_add_module(freedomdb MODULE
  source/pybindings.cpp
)
pybind11_extension(freedomdb)

#### MAIN
add_executable(freedomdb-tool
  source/main.cpp
)

#### Tests
if (BUILD_TESTS)
  add_subdirectory(modules/Catch2)
  add_executable(native-tests
    tests/main.cpp
    tests/test_db.cpp
    tests/test_db_pool.cpp
    tests/test_log.cpp
    tests/test_p2p.cpp
    tests/test_nocopyormove.cpp
    tests/test_utils.cpp
  )
  target_link_libraries(native-tests PRIVATE Catch2::Catch2WithMain)
endif ()

#### installation TODO
install(TARGETS freedomdb
  DESTINATION nodest #${PYTHON_INSTALL}
)
