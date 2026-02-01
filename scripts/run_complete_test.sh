#!/bin/bash
# Move to the project root from /scripts
cd "$(dirname "$0")/.."

if [ $# -lt 1 ]; then
    echo "Usage: $0 <video_file>"
    echo "Example: $0 testing/test_data/my_video.mp4"
    exit 1
fi

VIDEO="$1"
BASENAME=$(basename "$VIDEO" | sed 's/\.[^.]*$//')

# Updated paths for the consolidated testing structure
GT="testing/test_data/ground_truth_${BASENAME}.json"
PRED="testing/test_data/predictions_${BASENAME}.csv"
REPORT="testing/test_reports/report_${BASENAME}.txt"

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
        
        # Call the relocated annotation script
        ./scripts/annotate.sh "$VIDEO"
        
        # Note: annotationTool now saves directly to testing/test_data/ 
        # via the "../../" path adjustment we made in the C++ source.
        
        if [ ! -f "$GT" ]; then
            echo "✗ Ground truth not created. Exiting."
            exit 1
        fi
    else
        echo "Exiting. Create ground truth with:"
        echo "  ./scripts/annotate.sh $VIDEO"
        exit 1
    fi
fi

echo "✓ Ground truth: $GT"
echo ""

# Step 1: Generate predictions
echo "Step 1: Generating predictions with eyeLike..."
if [ ! -f "build/eyeLike" ]; then
    echo "✗ eyeLike not found. Build it with: ./scripts/cmakeBuild.sh"
    exit 1
fi

# Run eyeLike from root
./build/eyeLike -v "$VIDEO" -o "$PRED"

if [ $? -ne 0 ]; then
    echo "✗ eyeLike failed"
    exit 1
fi

echo "✓ Predictions: $PRED"
echo ""

# Step 2: Run quality tests
echo "Step 2: Running quality tests..."
./scripts/runTest.sh "$VIDEO" "$GT" "$PRED"

# Move and rename the generic report to the specific BASENAME report
# testEyeTracking now saves to testing/test_reports/test_report.txt
if [ -f "testing/test_reports/test_report.txt" ]; then
    mv "testing/test_reports/test_report.txt" "$REPORT"
    echo ""
    echo "✓ Report saved to: $REPORT"
fi

echo ""
echo "╔═══════════════════════════════════════╗"
echo "║  Testing Complete!                    ║"
echo "╚═══════════════════════════════════════╝"