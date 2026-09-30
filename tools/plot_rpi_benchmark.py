import matplotlib.pyplot as plt
import numpy as np

# Configuração de estilo académico
plt.rcParams.update({
    'font.family': 'serif',
    'font.size': 11,
    'axes.labelsize': 11,
    'axes.titlesize': 12,
    'xtick.labelsize': 10,
    'ytick.labelsize': 10,
    'legend.fontsize': 10,
    'figure.titlesize': 13
})

# Dados consolidados da Tabela 5.2
ensaios = ['Ensaio A\n(Vídeo Offline 480p)', 'Ensaio B\n(Câmara CSI 720p)']
fps = [6.362, 0.635]
cores = [1.620, 2.388]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(8.5, 3.8), constrained_layout=True)

# Cores sóbrias para publicação
cor_a = '#2b5c8f'
cor_b = '#d95f02'
bar_colors = [cor_a, cor_b]

# --- Painel 1: Cadência de Inferência (FPS) ---
bars1 = ax1.bar(ensaios, fps, color=bar_colors, width=0.55, edgecolor='black', linewidth=0.8)
ax1.set_ylabel('Cadência Efetiva (FPS)')
ax1.set_title('(a) Desempenho Temporal')
ax1.set_ylim(0, 7.5)
ax1.grid(axis='y', linestyle='--', alpha=0.5)

# Rótulos de dados no topo das barras
for bar in bars1:
    yval = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2.0, yval + 0.15, f'{yval:.2f} FPS', 
             ha='center', va='bottom', fontweight='bold')

# --- Painel 2: Ocupação de CPU (Núcleos ARM) ---
bars2 = ax2.bar(ensaios, cores, color=bar_colors, width=0.55, edgecolor='black', linewidth=0.8)
ax2.axhline(4.0, color='red', linestyle=':', linewidth=1.2, label='Teto de Hardware (4 Cores)')
ax2.set_ylabel('Núcleos Equivalentes Alocados')
ax2.set_title('(b) Carga de Processamento')
ax2.set_ylim(0, 4.5)
ax2.grid(axis='y', linestyle='--', alpha=0.5)
ax2.legend(loc='upper right')

for bar in bars2:
    yval = bar.get_height()
    pct = (yval / 4.0) * 100
    ax2.text(bar.get_x() + bar.get_width()/2.0, yval + 0.10, f'{yval:.2f} ({pct:.1f}%)', 
             ha='center', va='bottom', fontweight='bold')

# Exportação direta para a pasta de figuras da dissertação
output_path_pdf = '../MSc_Thesis/figures/benchmark_rpi4_desempenho.pdf'
output_path_png = '../MSc_Thesis/figures/benchmark_rpi4_desempenho.png'

plt.savefig(output_path_pdf, bbox_inches='tight')
plt.savefig(output_path_png, dpi=300, bbox_inches='tight')
print("Gráficos gerados com sucesso em MSc_Thesis/figures/")