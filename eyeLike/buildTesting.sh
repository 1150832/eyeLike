#!/bin/bash

echo "=== Building Eye Tracking Testing Tools ==="
echo ""

# Check if files exist and have reasonable size
ANNO_SIZE=$(wc -c < testing/annotationTool.cpp 2>/dev/null || echo 0)
TEST_SIZE=$(wc -c < testing/testEyeTracking.cpp 2>/dev/null || echo 0)

if [ "$ANNO_SIZE" -lt 1000 ]; then
    echo "✗ Error: annotationTool.cpp is too small ($ANNO_SIZE bytes)"
    echo "Expected: ~15-20KB"
    echo "Please copy the full content from the artifact"
    exit 1
fi

if [ "$TEST_SIZE" -lt 1000 ]; then
    echo "✗ Error: testEyeTracking.cpp is too small ($TEST_SIZE bytes)"
    echo "Expected: ~20-25KB"
    echo "Please copy the full content from the artifact"
    exit 1
fi

echo "✓ Source files validated"
echo "  annotationTool.cpp: $ANNO_SIZE bytes"
echo "  testEyeTracking.cpp: $TEST_SIZE bytes"
echo ""

# Create build directory
mkdir -p testing/build
cd testing/build

# Run CMake
echo "Running CMake..."
cmake ..

if [ $? -ne 0 ]; then
    echo ""
    echo "✗ CMake configuration failed"
    exit 1
fi

# Build
echo ""
echo "Building..."
make -j4

if [ $? -eq 0 ]; then
    echo ""
    echo "╔═══════════════════════════════════════╗"
    echo "║  ✓ Build Successful!                 ║"
    echo "╚═══════════════════════════════════════╝"
    echo ""
    echo "Executables created:"
    echo "  $(pwd)/annotationTool"
    echo "  $(pwd)/testEyeTracking"
    echo ""
else
    echo ""
    echo "✗ Build failed - check compiler errors above"
    exit 1
fi

cd ../..
