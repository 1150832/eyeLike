#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 <video_file>"
    echo "Example: $0 test_data/my_video.mp4"
    exit 1
fi

VIDEO="$1"
BASENAME=$(basename "$VIDEO" | sed 's/\.[^.]*$//')
GT="test_data/ground_truth_${BASENAME}.json"
PRED="test_data/predictions_${BASENAME}.csv"
REPORT="test_reports/report_${BASENAME}.txt"

echo "╔═══════════════════════════════════════╗"
echo "║  Eye Tracking Quality Test Pipeline  ║"
echo "╚═══════════════════════════════════════╝"
echo ""
echo "Video: $VIDEO"
echo ""

# Check for ground truth
if [ ! -f "$GT" ]; then
    echo "⚠️  Ground truth not found: $GT"
    echo ""
    echo "Create it now? (y/n)"
    read -r response
    if [ "$response" = "y" ] || [ "$response" = "Y" ]; then
        echo ""
        echo "Opening annotation tool..."
        echo ""
        echo "Instructions:"
        echo "  1. Click LEFT eye (blue)"
        echo "  2. Click RIGHT eye (green)"
        echo "  3. Press 'N' for next unannotated frame"
        echo "  4. Press 'S' to save"
        echo "  5. Press 'Q' to quit"
        echo ""
        read -p "Press Enter to start..."
        
        ./annotate.sh "$VIDEO"
        
        # Check if GT was created in testing/build
        if [ -f "testing/build/ground_truth_${BASENAME}.json" ]; then
            mv "testing/build/ground_truth_${BASENAME}.json" "$GT"
            echo "✓ Ground truth saved to: $GT"
        fi
        
        if [ ! -f "$GT" ]; then
            echo "✗ Ground truth not created. Exiting."
            exit 1
        fi
    else
        echo "Exiting. Create ground truth with:"
        echo "  ./annotate.sh $VIDEO"
        exit 1
    fi
fi

echo "✓ Ground truth: $GT"
echo ""

# Generate predictions
echo "Step 1: Generating predictions with eyeLike..."

if [ ! -f "build/bin/eyeLike" ]; then
    echo "✗ eyeLike not found. Build it with: ./cmakeBuild.sh"
    exit 1
fi

build/bin/eyeLike -v "$VIDEO" -o "$PRED"

if [ $? -ne 0 ]; then
    echo "✗ eyeLike failed"
    exit 1
fi

echo "✓ Predictions: $PRED"
echo ""

# Run tests
echo "Step 2: Running quality tests..."
./runTest.sh "$VIDEO" "$GT" "$PRED"

# Move report
if [ -f "testing/build/test_report.txt" ]; then
    mv "testing/build/test_report.txt" "$REPORT"
    echo ""
    echo "✓ Report: $REPORT"
fi

echo ""
echo "╔═══════════════════════════════════════╗"
echo "║  Testing Complete!                    ║"
echo "╚═══════════════════════════════════════╝"
