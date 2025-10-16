set(TEST_SOURCES
    test_compression.cpp
    test_vfstream.cpp
    test_archive.cpp
    test_vfs.cpp
    test_integration.cpp
)

add_executable(datapak_tests ${TEST_SOURCES})

target_link_libraries(datapak_tests
    datapak
    GTest::gtest_main
    GTest::gtest
)

include(GoogleTest)
gtest_discover_tests(datapak_tests)