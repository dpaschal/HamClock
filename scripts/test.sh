#!/bin/bash
# HamClock test runner

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "❌ Build directory not found. Run scripts/build.sh first"
    exit 1
fi

cd "$BUILD_DIR"

echo "🧪 Running tests..."
ctest -V

echo "✅ All tests passed!"
