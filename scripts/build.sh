#!/bin/bash
# HamClock build helper script

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BUILD_TYPE="${1:-Release}"

echo "🔨 Building HamClock (${BUILD_TYPE})..."

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..

# Build
make -j$(nproc)

echo "✅ Build complete: $BUILD_DIR/hamclock"
echo ""
echo "Run with: $BUILD_DIR/hamclock"
