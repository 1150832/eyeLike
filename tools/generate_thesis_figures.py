import os
import cv2
import numpy as np
import matplotlib.pyplot as plt

# 1. Definir caminhos absolutos e garantir a pasta de saída
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
IMAGE_PATH = r"C:\Users\rmarques\Downloads\thesis_example_eye.jpeg"
OUTPUT_DIR = os.path.join(SCRIPT_DIR, "figures")
os.makedirs(OUTPUT_DIR, exist_ok=True)

# 2. Carregar a ROI do olho
roi_eye = cv2.imread(IMAGE_PATH, cv2.IMREAD_GRAYSCALE)

if roi_eye is None:
    print(f"[AVISO] Não foi possível carregar a imagem em: {IMAGE_PATH}")
    print("[INFO] A gerar imagem sintética de teste...")
    roi_eye = np.zeros((200, 300), dtype=np.uint8) + 180
    cv2.circle(roi_eye, (150, 100), 60, (90), -1)   # Íris
    cv2.circle(roi_eye, (150, 100), 25, (20), -1)   # Pupila

# ==============================================================================
# FIGURA 1: Comparação de Deteção de Bordas (Scharr vs Canny)
# ==============================================================================
def generate_figure_edge_comparison(roi):
    print("[1/2] A gerar figura de comparação de bordas (Scharr & Canny)...")
    
    # Calcular Scharr
    gx = cv2.Scharr(roi, cv2.CV_64F, 1, 0)
    gy = cv2.Scharr(roi, cv2.CV_64F, 0, 1)
    mag_scharr = cv2.magnitude(gx, gy)
    mag_scharr = cv2.normalize(mag_scharr, None, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_8U)

    # Calcular Canny
    canny_edges = cv2.Canny(roi, threshold1=50, threshold2=150)

    # Plotar
    fig, axes = plt.subplots(1, 3, figsize=(12, 4), dpi=300)
    axes[0].imshow(roi, cmap='gray')
    axes[0].set_title('(a) ROI Ocular Original', fontsize=11)
    axes[0].axis('off')

    axes[1].imshow(mag_scharr, cmap='gray')
    axes[1].set_title('(b) Magnitude do Gradiente (Scharr)', fontsize=11)
    axes[1].axis('off')

    axes[2].imshow(canny_edges, cmap='gray')
    axes[2].set_title('(c) Detetor de Canny (Bordas Finas)', fontsize=11)
    axes[2].axis('off')

    plt.tight_layout()
    
    pdf_path = os.path.join(OUTPUT_DIR, "edge_detection_comparison.pdf")
    plt.savefig(pdf_path, bbox_inches='tight')
    plt.close()
    print(f"   └─ Guardado em: {pdf_path}")

# ==============================================================================
# FIGURA 2: Acumulador e Resposta do Algoritmo de Timm & Barth
# ==============================================================================
def generate_figure_timm_barth(roi):
    print("[2/2] A calcular e gerar mapa de calor de Timm & Barth...")
    
    # Redimensionar para velocidade de cálculo na figura
    roi_small = cv2.resize(roi, (120, 120))
    
    # Gradientes
    gx = cv2.Scharr(roi_small, cv2.CV_64F, 1, 0)
    gy = cv2.Scharr(roi_small, cv2.CV_64F, 0, 1)
    mag = cv2.magnitude(gx, gy)

    mag[mag == 0] = 1.0
    gx_norm = gx / mag
    gy_norm = gy / mag

    # Filtrar gradientes fracos
    threshold = np.max(mag) * 0.1
    mask = mag > threshold

    # Weight map (Inversão e suavização)
    smoothed = cv2.GaussianBlur(roi_small, (5, 5), 0)
    weight_map = 255.0 - smoothed.astype(np.float64)
    weight_map /= np.max(weight_map)

    # Acumulador
    rows, cols = roi_small.shape
    accumulator = np.zeros((rows, cols), dtype=np.float64)
    y_indices, x_indices = np.where(mask)

    for cy in range(rows):
        for cx in range(cols):
            rx = x_indices - cx
            ry = y_indices - cy
            d_mag = np.sqrt(rx**2 + ry**2)
            d_mag[d_mag == 0] = 1.0
            
            dx = rx / d_mag
            dy = ry / d_mag
            
            dot_product = (dx * gx_norm[y_indices, x_indices] + dy * gy_norm[y_indices, x_indices]) ** 2
            accumulator[cy, cx] = weight_map[cy, cx] * np.sum(dot_product)

    _, _, _, max_loc = cv2.minMaxLoc(accumulator)

    # Plotar
    fig, axes = plt.subplots(1, 3, figsize=(13, 4), dpi=300)

    axes[0].imshow(roi_small, cmap='gray')
    axes[0].plot(max_loc[0], max_loc[1], 'r+', markersize=14, markeredgewidth=2)
    axes[0].set_title('(a) ROI com Centro Detetado ($c^*$)', fontsize=10)
    axes[0].axis('off')

    axes[1].imshow(weight_map, cmap='magma')
    axes[1].set_title('(b) Mapa de Escuridão ($w_c$)', fontsize=10)
    axes[1].axis('off')

    im = axes[2].imshow(accumulator, cmap='jet')
    axes[2].plot(max_loc[0], max_loc[1], 'w*', markersize=10)
    axes[2].set_title('(c) Superfície de Resposta (Acumulador)', fontsize=10)
    axes[2].axis('off')

    plt.colorbar(im, ax=axes[2], fraction=0.046, pad=0.04)
    plt.tight_layout()

    pdf_path = os.path.join(OUTPUT_DIR, "timm_barth_response.pdf")
    plt.savefig(pdf_path, bbox_inches='tight')
    plt.close()
    print(f"   └─ Guardado em: {pdf_path}")

# ==============================================================================
# Execução Principal
# ==============================================================================
if __name__ == "__main__":
    generate_figure_edge_comparison(roi_eye)
    generate_figure_timm_barth(roi_eye)
    print("\n[CONCLUÍDO] Todas as figuras foram geradas com sucesso!")