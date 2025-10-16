cmake_minimum_required(VERSION 3.20)
project(datapak VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZLIB REQUIRED zlib)

# Testing support
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# Code coverage support
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add coverage flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage --coverage -g -O0")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage --coverage -g -O0")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage --coverage")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage --coverage")

        # Find required tools
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)

        if(LCOV_PATH AND GENHTML_PATH)
            # Create coverage directory
            file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/coverage)

            # Add coverage target
            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND ${LCOV_PATH} --directory . --zerocounters --ignore-errors inconsistent,unused,mismatch
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${LCOV_PATH} --directory . --capture --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info --ignore-errors inconsistent,unused,mismatch
                COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage/coverage.info
                    '/usr/*'
                    '${CMAKE_SOURCE_DIR}/tests/*'
                    '${CMAKE_BINARY_DIR}/*'
                    '*/gtest/*'
                    '*/c++/*'
                    --output-file ${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info --ignore-errors inconsistent,unused,mismatch
                COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info
                    --output-directory ${CMAKE_BINARY_DIR}/coverage/html
                    --title "DataPak Coverage Report"
                    --show-details
                    --legend
                COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in ${CMAKE_BINARY_DIR}/coverage/html/index.html"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS datapak_tests
                COMMENT "Generating code coverage report"
            )

            # Add coverage summary target
            add_custom_target(coverage-summary
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND ${LCOV_PATH} --directory . --zerocounters --ignore-errors inconsistent,unused,mismatch
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${LCOV_PATH} --directory . --capture --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info --ignore-errors inconsistent,unused,mismatch
                COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage/coverage.info
                    '/usr/*'
                    '${CMAKE_SOURCE_DIR}/tests/*'
                    '${CMAKE_BINARY_DIR}/*'
                    '*/gtest/*'
                    '*/c++/*'
                    --output-file ${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info --ignore-errors inconsistent,unused,mismatch
                COMMAND ${LCOV_PATH} --summary ${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info --ignore-errors inconsistent,unused,mismatch
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS datapak_tests
                COMMENT "Generating code coverage summary"
            )
        else()
            message(WARNING "lcov and/or genhtml not found. Coverage targets will not be available.")
        endif()
    else()
        message(WARNING "Code coverage is only supported with GCC or Clang.")
    endif()
endif()

include_directories(include)

set(DATAPAK_SOURCES
    src/archive.cpp
    src/archive_builder.cpp
    src/vfs.cpp
    src/vfstream.cpp
    src/compression.cpp
)

set(DATAPAK_HEADERS
    include/datapak/archive.hpp
    include/datapak/archive_builder.hpp
    include/datapak/vfs.hpp
    include/datapak/vfstream.hpp
    include/datapak/compression.hpp
    include/datapak/format.hpp
)

add_library(datapak STATIC ${DATAPAK_SOURCES} ${DATAPAK_HEADERS})

target_include_directories(datapak PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(datapak ${ZLIB_LIBRARIES})
target_include_directories(datapak PRIVATE ${ZLIB_INCLUDE_DIRS})

add_executable(datapak_example examples/main.cpp)
target_link_libraries(datapak_example datapak)

add_executable(datapak_cli tools/datapak_cli.cpp)
target_link_libraries(datapak_cli datapak)

install(TARGETS datapak
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# Tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()