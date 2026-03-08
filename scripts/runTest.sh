#!/bin/bash

TEST_EXEC="./build/testEyeTracking"

# Verifica se o utilizador passou o vídeo como argumento
if [ "$#" -lt 1 ]; then
    echo "=== Eye Tracking Test Framework ==="
    echo "Usage: $0 <video_file>"
    echo "Example: $0 testing/test_data/test1.mov"
    exit 1
fi

VIDEO_PATH="$1"

# Extrair a diretoria e o nome do ficheiro para construir os caminhos vizinhos
DIRNAME=$(dirname "$VIDEO_PATH")
BASENAME=$(basename "$VIDEO_PATH")
FILENAME="${BASENAME%.*}"

GROUND_TRUTH="$DIRNAME/ground_truth_$FILENAME.json"
PREDICTIONS="$DIRNAME/predictions_$FILENAME.csv"

echo "=== Running Eye Tracking Tests ==="
echo "Video:        $VIDEO_PATH"
echo "Ground Truth: $GROUND_TRUTH"
echo "Predictions:  $PREDICTIONS"
echo ""

# Validações de ficheiros
if [ ! -f "$TEST_EXEC" ]; then
    echo "✗ Error: testEyeTracking not found at $TEST_EXEC"
    echo "Please run: ./scripts/buildTesting.sh"
    exit 1
fi

if [ ! -f "$GROUND_TRUTH" ]; then
    echo "✗ Error: Ground truth file not found!"
    echo "Expected at: $GROUND_TRUTH"
    echo "Please run the annotation tool to create it:"
    echo "./build/annotationTool $VIDEO_PATH"
    exit 1
fi

if [ ! -f "$PREDICTIONS" ]; then
    echo "✗ Error: Predictions file not found!"
    echo "Expected at: $PREDICTIONS"
    echo "Please run eyeLike to generate it EXACTLY with this command:"
    echo "./build/bin/eyeLike -v $VIDEO_PATH -o $PREDICTIONS"
    exit 1
fi

# Garante que a pasta de relatórios existe antes de correr o teste
mkdir -p testing/test_reports

# Corre o teste final
$TEST_EXEC "$VIDEO_PATH" "$GROUND_TRUTH" "$PREDICTIONS"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Tests completed successfully."
else
    echo ""
    echo "✗ Tests failed."
    exit 1
fi