import os
import pandas as pd
import matplotlib.pyplot as plt

CSV_FILE = os.path.join("test_reports", "telemetry", "3d_telemetry_log_ensaio_final_16cm.csv")

if not os.path.exists(CSV_FILE) or os.stat(CSV_FILE).st_size == 0:
    print(f"Erro: Ficheiro de telemetria não encontrado em '{CSV_FILE}'.")
    exit()

df = pd.read_csv(CSV_FILE)

if df.empty:
    print("O ficheiro CSV está vazio. Aguarda por capturas de dados.")
    exit()

print(f"A carregar {len(df)} pontos 3D capturados...")

fig = plt.figure(figsize=(8, 6))
ax = fig.add_subplot(111, projection='3d')

# Desenha trajetórias dos dois olhos
ax.scatter(df['Left_X'], df['Left_Y'], df['Left_Z'], c='r', marker='o', label='Olho Esquerdo')
ax.scatter(df['Right_X'], df['Right_Y'], df['Right_Z'], c='b', marker='^', label='Olho Direito')

ax.set_xlabel('Eixo X (Horizontal)')
ax.set_ylabel('Eixo Y (Vertical)')
ax.set_zlabel('Eixo Z (Profundidade)')
ax.set_title('Reconstrução Tridimensional dos Centros Pupilares (Referencial $P_1$)', pad=12)
ax.legend()

ax.xaxis.pane.fill = False
ax.yaxis.pane.fill = False
ax.zaxis.pane.fill = False
ax.view_init(elev=22, azim=-62)

# Guarda figuras de alta resolução para a dissertação
output_dir = os.path.join("tools", "figures")
os.makedirs(output_dir, exist_ok=True)
plt.savefig(os.path.join(output_dir, "reconstrucao_3d_scatter.pdf"), bbox_inches='tight', pad_inches=0.05)
plt.savefig(os.path.join(output_dir, "reconstrucao_3d_scatter.png"), dpi=300, bbox_inches='tight', pad_inches=0.05)
print("Figura guardada em tools/figures/reconstrucao_3d_scatter.pdf")

plt.show()