#!/bin/bash
# Move to the project root from /scripts
cd "$(dirname "$0")/.."

# Check if the validation binary exists in the new build location
if [ ! -f "build/bin/testEyeTracking" ]; then
    echo "Error: testEyeTracking not built"
    echo "Run: ./scripts/cmakeBuild.sh"
    exit 1
fi

# Execute from the project root using relative paths for arguments
exec ./build/testEyeTracking "$@"