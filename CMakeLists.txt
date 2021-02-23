cmake_minimum_required(VERSION 3.6)

# Preparation
find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()


project(freedomdb VERSION 0.0.1 LANGUAGES CXX)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)

add_subdirectory(modules/pybind11)
#sudo aptitude install libpython-dev libboost-dev libpq-dev libpqxx-dev
# This line adds both include paths and libraries for Postgres

option(USE_CLANG "Use Clang compiler" OFF) # TODO
option(TESTING "Build tests" ON)
option(EXAMPLES "Build examples" ON)
option(CLANG_FORMAT "Enable clang-format target" ON)
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
  source/p2p/p2p.cpp
  source/core/log.cpp
)
include_directories(
  # project includes
  ${PROJECT_SOURCE_DIR}/source
  ${PROJECT_SOURCE_DIR}/modules/msgpack/include
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
add_subdirectory(modules/Catch2)
add_executable(native-tests
  tests/main.cpp
)
target_link_libraries(native-tests PRIVATE Catch2::Catch2WithMain)

#### installation TODO
install(TARGETS freedomdb
  DESTINATION nodest #${PYTHON_INSTALL}
)
