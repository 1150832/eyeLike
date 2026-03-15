#!/bin/bash
cd "$(dirname "$0")/.."

if [ "$#" -lt 1 ]; then
    echo "=== Anottation Tool ==="
    echo "Error: Missing arguments."
    echo "Use: $0 <video_file>"
    echo "Example: $0 testing/test_data/test1.mov"
    exit 1
fi

VIDEO_PATH="$1"

if [ ! -f "$VIDEO_PATH" ]; then
    echo "✗ Error: the video '$VIDEO_PATH' was not found!"
    exit 1
fi

./build/annotationTool "$VIDEO_PATH"