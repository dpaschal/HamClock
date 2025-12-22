# Phase 7c Test Configuration
# Comprehensive test suite for HamClock

enable_testing()

# Test utilities and source files needed
set(TEST_SOURCES
    ../src/core/log.c
    ../src/utils/string_utils.c
    ../src/utils/json_simple.c
    ../src/utils/timezone.c
    ../src/utils/maidenhead.c
    ../src/astro/sun.c
    ../src/astro/moon.c
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Test 1: Timezone module
add_executable(test_timezone test_timezone.c ${TEST_SOURCES})
target_link_libraries(test_timezone m)
target_include_directories(test_timezone PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
add_test(NAME timezone_tests COMMAND test_timezone)

# Test 2: Sun position and seasons
add_executable(test_sun test_sun.c ${TEST_SOURCES})
target_link_libraries(test_sun m)
target_include_directories(test_sun PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
add_test(NAME sun_tests COMMAND test_sun)

# Test 3: Maidenhead grid
add_executable(test_maidenhead test_maidenhead.c ${TEST_SOURCES})
target_link_libraries(test_maidenhead m)
target_include_directories(test_maidenhead PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
add_test(NAME maidenhead_tests COMMAND test_maidenhead)

# Summary
message(STATUS "Test suite configured:")
message(STATUS "  - test_timezone   : Timezone conversion, DST, formatting")
message(STATUS "  - test_sun        : Solar position, seasons, solstices")
message(STATUS "  - test_maidenhead : Grid conversion, validation, roundtrip")
message(STATUS "")
message(STATUS "Run tests with: ctest -V")
