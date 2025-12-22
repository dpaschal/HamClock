# CMake Toolchain for ARM64 (Raspberry Pi 4 native mode)
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm64-rpi.cmake ..

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Compiler settings
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_AR aarch64-linux-gnu-ar)
set(CMAKE_RANLIB aarch64-linux-gnu-ranlib)

# Compiler flags for ARM64
set(CMAKE_C_FLAGS_INIT "-march=armv8-a -mtune=cortex-a72" CACHE STRING "")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a -mtune=cortex-a72" CACHE STRING "")

# Skip compiler tests
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Find cross-compiled libraries in standard ARM64 locations
set(CMAKE_FIND_ROOT_PATH
    /usr/aarch64-linux-gnu
    /usr/local/aarch64-linux-gnu
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Optional: pkg-config for cross-compilation
set(PKG_CONFIG_EXECUTABLE aarch64-linux-gnu-pkg-config CACHE FILEPATH "")

# Build type recommendations
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
    message(STATUS "Setting build type to Release for ARM64")
endif()

# Display configuration
message(STATUS "Configuring for ARM64 (Raspberry Pi 4 native)")
message(STATUS "Compiler: aarch64-linux-gnu-gcc")
message(STATUS "Flags: -march=armv8-a -mtune=cortex-a72")
