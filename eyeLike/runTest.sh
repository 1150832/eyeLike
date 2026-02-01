#!/bin/bash
if [ ! -f "testing/build/testEyeTracking" ]; then
    echo "Error: testEyeTracking not built"
    echo "Run: ./buildTesting.sh"
    exit 1
fi

exec testing/build/testEyeTracking "$@"
