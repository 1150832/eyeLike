#!/bin/bash
# Move to project root
cd "$(dirname "$0")/.."

if [ ! -f "build/annotationTool" ]; then
    echo "Error: annotationTool not built. Run scripts/cmakeBuild.sh first."
    exit 1
fi

exec build/annotationTool "$@"