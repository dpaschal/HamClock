# CMake Toolchain for ARMv7 (Raspberry Pi 3/3B+/Zero 2)
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-armv7-rpi.cmake ..

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

# Compiler settings
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_AR arm-linux-gnueabihf-ar)
set(CMAKE_RANLIB arm-linux-gnueabihf-ranlib)

# Compiler flags for ARMv7
set(CMAKE_C_FLAGS_INIT "-march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "")
set(CMAKE_CXX_FLAGS_INIT "-march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "")

# Skip compiler tests
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Find cross-compiled libraries in standard ARM locations
set(CMAKE_FIND_ROOT_PATH
    /usr/arm-linux-gnueabihf
    /usr/local/arm-linux-gnueabihf
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Optional: pkg-config for cross-compilation
set(PKG_CONFIG_EXECUTABLE arm-linux-gnueabihf-pkg-config CACHE FILEPATH "")

# Build type recommendations
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
    message(STATUS "Setting build type to Release for ARM")
endif()

# Display configuration
message(STATUS "Configuring for ARMv7 (Raspberry Pi 3/3B+/Zero 2)")
message(STATUS "Compiler: arm-linux-gnueabihf-gcc")
message(STATUS "Flags: -march=armv7-a -mfpu=neon -mfloat-abi=hard")
