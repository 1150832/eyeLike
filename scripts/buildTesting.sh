#!/bin/bash

echo "=== Building Eye Tracking Testing Tools ==="
echo ""

# 1. Update paths to check testing/src/ instead of testing/
ANNO_SIZE=$(wc -c < testing/src/annotationTool.cpp 2>/dev/null || echo 0)
TEST_SIZE=$(wc -c < testing/src/testEyeTracking.cpp 2>/dev/null || echo 0)

if [ "$ANNO_SIZE" -lt 1000 ]; then
    echo "✗ Error: annotationTool.cpp is too small ($ANNO_SIZE bytes)"
    echo "Checked path: testing/src/annotationTool.cpp"
    echo "Please ensure the file exists and has content"
    exit 1
fi

if [ "$TEST_SIZE" -lt 1000 ]; then
    echo "✗ Error: testEyeTracking.cpp is too small ($TEST_SIZE bytes)"
    echo "Checked path: testing/src/testEyeTracking.cpp"
    echo "Please ensure the file exists and has content"
    exit 1
fi

echo "✓ Source files validated"
echo ""

# 2. Use the ROOT build directory instead of testing/build
mkdir -p build
cd build

# Run CMake from root
echo "Running CMake from root..."
cmake ..

if [ $? -ne 0 ]; then
    echo ""
    echo "✗ CMake configuration failed"
    exit 1
fi

# 3. Build specifically the test targets defined in your main CMakeLists.txt
echo ""
echo "Building Test Tools..."
make annotationTool testEyeTracking -j4

if [ $? -eq 0 ]; then
    echo ""
    echo "╔═══════════════════════════════════════╗"
    echo "║  ✓ Build Successful!                  ║"
    echo "╚═══════════════════════════════════════╝"
    echo ""
    echo "Executables created in build/:"
    echo "  $(pwd)/annotationTool"
    echo "  $(pwd)/testEyeTracking"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

cd ..