import os
import pandas as pd
import matplotlib.pyplot as plt

CSV_FILE = os.path.join("test_reports", "telemetry", "3d_telemetry_log.csv")

if not os.path.exists(CSV_FILE) or os.stat(CSV_FILE).st_size == 0:
    print(f"Erro: Ficheiro de telemetria não encontrado em '{CSV_FILE}'.")
    exit()

df = pd.read_csv(CSV_FILE)

if df.empty:
    print("O ficheiro CSV está vazio. Aguarda por capturas de dados.")
    exit()

print(f"A carregar {len(df)} pontos 3D capturados...")

fig = plt.figure(figsize=(10, 7))
ax = fig.add_subplot(111, projection='3d')

# Desenha trajetórias dos dois olhos
ax.scatter(df['Left_X'], df['Left_Y'], df['Left_Z'], c='r', marker='o', label='Olho Esquerdo')
ax.scatter(df['Right_X'], df['Right_Y'], df['Right_Z'], c='b', marker='^', label='Olho Direito')

ax.set_xlabel('Eixo X (Horizontal)')
ax.set_ylabel('Eixo Y (Vertical)')
ax.set_zlabel('Eixo Z (Profundidade)')
ax.set_title('Reconstrução Tridimensional de Atenção Ocular')
ax.legend()

plt.show()