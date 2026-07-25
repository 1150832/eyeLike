#!/bin/bash
# Move to the project root from /scripts
cd "$(dirname "$0")/.."

print_usage() {
    echo "╔═══════════════════════════════════════╗"
    echo "║  Eye Tracking Quality Test Pipeline  ║"
    echo "╚═══════════════════════════════════════╝"
    echo "Usage: $0 [video_file | --all]"
    echo ""
    echo "Examples:"
    echo "  $0 testing/test_data/test1.mov  (Run for a single video)"
    echo "  $0 --all                        (Batch test ALL annotated videos)"
    exit 1
}

if [ $# -lt 1 ]; then
    print_usage
fi

# ==========================================
# FUNÇÃO PARA PROCESSAR UM ÚNICO VÍDEO
# ==========================================
process_video() {
    local VIDEO="$1"
    local DIRNAME=$(dirname "$VIDEO")
    local BASENAME=$(basename "$VIDEO")
    local FILENAME="${BASENAME%.*}"
    local GT="$DIRNAME/ground_truth_$FILENAME.json"
    local PRED="$DIRNAME/predictions_$FILENAME.csv"

    echo "---------------------------------------------------"
    echo "Processing: $VIDEO"

    # 1. Verificar Ground Truth
    if [ ! -f "$GT" ]; then
        echo "⚠️  Ground truth not found for: $BASENAME"
        
        # Só pergunta se quer anotar se NÃO estivermos no modo batch (--all)
        if [ "$BATCH_MODE" = false ]; then
            echo "Create it now? (y/n)"
            read -r response
            if [ "$response" = "y" ] || [ "$response" = "Y" ]; then
                echo "Opening annotation tool..."
                ./build/annotationTool "$VIDEO"
            else
                echo "Exiting."
                return 1
            fi
        else
            echo "Skipping video (Annotate it first using the annotation tool)."
            return 1
        fi
    fi

    # Se ainda assim não existir GT após a possível anotação
    if [ ! -f "$GT" ]; then
        echo "✗ Ground truth not created. Skipping."
        return 1
    fi

    echo "✓ Ground truth confirmed."

    # 2. Gerar Previsões (Usando a nova flag HEADLESS!)
    echo "▶ Generating predictions with eyeLike (Headless Mode)..."
    ./build/eyeLike -v "$VIDEO" -o "$PRED" --headless
    
    if [ $? -ne 0 ]; then
        echo "✗ eyeLike prediction failed"
        return 1
    fi

    echo "✓ Predictions generated."

    # 3. Correr os Testes de Qualidade
    echo "▶ Running quality tests..."
    # O runTest.sh que atualizámos já só precisa do caminho do vídeo!
    ./scripts/runTest.sh "$VIDEO"
    
    if [ $? -ne 0 ]; then
        echo "✗ Quality tests failed"
        return 1
    fi
    
    return 0
}


# ==========================================
# LÓGICA PRINCIPAL (BATCH vs SINGLE)
# ==========================================

BATCH_MODE=false

if [ "$1" = "--all" ]; then
    BATCH_MODE=true
    echo "Starting BATCH TESTING mode..."
    echo "Looking for videos in testing/test_data/..."
    
    success_count=0
    total_count=0
    
    # Encontra todos os vídeos na pasta
    for VIDEO in testing/test_data/*.{mov,mp4,avi,mkv}; do
        # Evita falhas se a pasta estiver vazia
        [ -e "$VIDEO" ] || continue
        
        # Antes de processar, verifica silenciosamente se tem anotações
        DIRNAME=$(dirname "$VIDEO")
        FILENAME=$(basename "$VIDEO" | sed 's/\.[^.]*$//')
        GT_CHECK="$DIRNAME/ground_truth_$FILENAME.json"
        
        if [ -f "$GT_CHECK" ]; then
            total_count=$((total_count+1))
            process_video "$VIDEO"
            if [ $? -eq 0 ]; then
                success_count=$((success_count+1))
            fi
        else
            echo "Skipping $VIDEO (No annotations found)"
        fi
    done
    
    echo "==================================================="
    echo "BATCH TESTING COMPLETE"
    echo "Successfully processed: $success_count / $total_count annotated videos"
    echo "All reports are available in testing/test_reports/"
    
else
    # MODO SINGLE VIDEO
    VIDEO="$1"
    if [ ! -f "$VIDEO" ]; then
        echo "✗ Error: Video file not found: $VIDEO"
        exit 1
    fi
    
    process_video "$VIDEO"
    
    echo ""
    echo "╔═══════════════════════════════════════╗"
    echo "║  Testing Complete!                    ║"
    echo "╚═══════════════════════════════════════╝"
fi