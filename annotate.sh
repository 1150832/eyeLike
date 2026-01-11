#!/bin/bash
if [ ! -f "testing/build/annotationTool" ]; then
    echo "Error: annotationTool not built"
    echo "Run: ./buildTesting.sh"
    exit 1
fi

exec testing/build/annotationTool "$@"
