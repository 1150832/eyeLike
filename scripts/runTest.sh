#!/bin/bash
cd "$(dirname "$0")/.."

if [ "$#" -lt 1 ]; then
    echo "=== Summary of Results (testEyeTracking) ==="
    echo "Use: $0 <video_file>"
    exit 1
fi

VIDEO_PATH="$1"
DIRNAME=$(dirname "$VIDEO_PATH")
BASENAME=$(basename "$VIDEO_PATH")
FILENAME="${BASENAME%.*}"

GROUND_TRUTH="$DIRNAME/ground_truth_$FILENAME.json"
PREDICTIONS="$DIRNAME/predictions_$FILENAME.csv"

if [ ! -f "$GROUND_TRUTH" ] || [ ! -f "$PREDICTIONS" ]; then
    echo "✗ Error: Missing .json or .csv files!"
    echo "Run ./scripts/run_complete_test.sh first to generate them."
    exit 1
fi

mkdir -p testing/test_reports
./build/testEyeTracking "$VIDEO_PATH" "$GROUND_TRUTH" "$PREDICTIONS"