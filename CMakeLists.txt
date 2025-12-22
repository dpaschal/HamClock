cmake_minimum_required(VERSION 3.10)
project(hamclock-c VERSION 1.0.0 LANGUAGES C)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Feature test macros for POSIX functions
add_compile_definitions(_POSIX_C_SOURCE=200809L _DEFAULT_SOURCE)

# Optimization and warnings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -O0 -g3 -fsanitize=address")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -O2 -flto -DNDEBUG")
endif()

# Dependencies
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

# Check for SDL2 (optional, for Phase 3+)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 QUIET sdl2)
    pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
    pkg_check_modules(SDL2_IMAGE QUIET SDL2_image)
endif()

# Fallback: try old-style find_package if pkg-config didn't work
if(NOT SDL2_FOUND)
    find_package(SDL2 QUIET)
    find_package(SDL2_ttf QUIET)
    find_package(SDL2_image QUIET)
endif()

# Source files
set(SOURCES
    src/main.c
    src/core/log.c
    src/core/config.c
    src/core/state.c
    src/core/timing.c
    src/data/cache.c
    src/data/database.c
    src/api/http_client.c
    src/api/api_manager.c
    src/api/noaa.c
    src/utils/string_utils.c
    src/utils/json_simple.c
)

# Only add display sources if SDL2 is available
if(SDL2_FOUND AND SDL2_TTF_FOUND AND SDL2_IMAGE_FOUND)
    message(STATUS "SDL2 found - display modules enabled")
    # Phase 3+ display modules
    list(APPEND SOURCES
        src/display/renderer.c
    )
    # Phase 4+ modules (optional, created on demand)
    # list(APPEND SOURCES
    #     src/display/earthmap.c
    #     src/astro/sun.c
    #     src/astro/moon.c
    # )
else()
    message(STATUS "SDL2 not found - building data layer only (Phase 1-2)")
endif()

# Executable
add_executable(hamclock ${SOURCES})

# Link libraries
target_link_libraries(hamclock
    PRIVATE
    SQLite::SQLite3
    CURL::libcurl
    m
)

# Link SDL2 if available
if(SDL2_FOUND AND SDL2_TTF_FOUND AND SDL2_IMAGE_FOUND)
    # Try to link with CMake targets first
    if(TARGET SDL2::SDL2)
        target_link_libraries(hamclock PRIVATE SDL2::SDL2 SDL2::SDL2_ttf SDL2::SDL2_image)
    else()
        # Fall back to pkg-config libraries
        target_link_libraries(hamclock PRIVATE ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
        target_include_directories(hamclock PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    endif()
endif()

# Include directories
target_include_directories(hamclock
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLite3_INCLUDE_DIRS}
)

# Version info
target_compile_definitions(hamclock
    PRIVATE
    HAMCLOCK_VERSION="1.0.0"
    HAMCLOCK_BUILD_DATE="${CMAKE_BUILD_DATE}"
)

# Installation
install(TARGETS hamclock DESTINATION bin)

# Testing
enable_testing()
add_subdirectory(tests)
