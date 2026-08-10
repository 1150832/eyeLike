import os
import cv2
import numpy as np
import matplotlib.pyplot as plt

# 1. Obter a diretoria onde o próprio script está guardado
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# 2. Definir o caminho da imagem de entrada (Caminho limpo, sem aspas e com a extensão correta)
IMAGE_PATH = r"C:\Users\rmarques\Downloads\thesis_example_eye.jpeg"

# 3. Definir a pasta de saída (Garante que a pasta 'figures' existe)
OUTPUT_DIR = os.path.join(SCRIPT_DIR, "figures")
os.makedirs(OUTPUT_DIR, exist_ok=True)  # Cria a pasta automaticamente se não existir!

# 4. Carregar a imagem
roi_eye = cv2.imread(IMAGE_PATH, cv2.IMREAD_GRAYSCALE)

if roi_eye is None:
    print(f"[AVISO] Não foi possível carregar a imagem em: {IMAGE_PATH}")
    print("[INFO] A gerar imagem sintética de teste...")
    roi_eye = np.zeros((200, 300), dtype=np.uint8) + 180
    cv2.circle(roi_eye, (150, 100), 60, (90), -1)   # Íris
    cv2.circle(roi_eye, (150, 100), 25, (20), -1)   # Pupila

# 5. Aplicar Operador de Scharr para Magnitude do Gradiente
gx = cv2.Scharr(roi_eye, cv2.CV_64F, 1, 0)
gy = cv2.Scharr(roi_eye, cv2.CV_64F, 0, 1)
magnitude_scharr = cv2.magnitude(gx, gy)
magnitude_scharr = cv2.normalize(magnitude_scharr, None, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_8U)

# 6. Aplicar Detetor de Bordas de Canny
canny_edges = cv2.Canny(roi_eye, threshold1=50, threshold2=150)

# 7. Criar a figura composta
fig, axes = plt.subplots(1, 3, figsize=(12, 4), dpi=300)

axes[0].imshow(roi_eye, cmap='gray')
axes[0].set_title('(a) ROI Ocular Original', fontsize=11)
axes[0].axis('off')

axes[1].imshow(magnitude_scharr, cmap='gray')
axes[1].set_title('(b) Magnitude do Gradiente (Scharr)', fontsize=11)
axes[1].axis('off')

axes[2].imshow(canny_edges, cmap='gray')
axes[2].set_title('(c) Detetor de Canny (Bordas Finas)', fontsize=11)
axes[2].axis('off')

plt.tight_layout()

# 8. Guardar nos caminhos absolutos seguros
pdf_path = os.path.join(OUTPUT_DIR, "edge_detection_comparison.pdf")
png_path = os.path.join(OUTPUT_DIR, "edge_detection_comparison.png")

plt.savefig(pdf_path, bbox_inches='tight')
plt.savefig(png_path, bbox_inches='tight', dpi=300)

print(f"[SUCESSO] Figuras guardadas com sucesso em:\n - {pdf_path}\n - {png_path}")