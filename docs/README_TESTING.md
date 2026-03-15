# Eye Tracking Testing Framework & Annotation Tool

Este repositório inclui um ecossistema completo para testar, anotar e avaliar a precisão do algoritmo `eyeLike`. As ferramentas foram desenhadas para serem robustas, altamente visuais e gerarem relatórios de qualidade académica para análise de dados, lidando de forma inteligente com oclusões (olhos ocultos).

## 🚀 Como Compilar

Para compilar as ferramentas de anotação e avaliação (independentes do projeto principal), corre o script na raiz do projeto:
```bash
./scripts/buildTesting.sh
```
*(Nota: Para compilar o algoritmo `eyeLike` principal, deves usar o `./scripts/cmakeBuild.sh`)*

---

## 1. Ferramenta de Anotação (`annotationTool`)

Uma interface gráfica para criar o "Ground Truth" num vídeo. 

**Como correr:**
```bash
./scripts/annotate.sh testing/test_data/video_de_teste.mov
```

### ✨ Funcionalidades Avançadas de Anotação:
* **Suporte para Oclusões (Teclas 3 e 4):** Se a pessoa virar a cara e um dos olhos desaparecer da câmara, podes marcar esse olho especificamente como "Oculto". O teste estatístico avaliará os olhos de forma independente, não descartando a frame inteira.
* **Validação de Olhos Fechados (Tecla X):** Para marcares um frame completo com olhos fechados, prime `X`. O cálculo do erro ignorará estas frames para não penalizar a precisão do algoritmo.
* **Timeline Visual (Barra de Progresso):** Uma barra na base mapeia o progresso: Cinzento (não anotado), Verde (ambos olhos abertos), Azul (1 olho oculto), Amarelo (olhos fechados).
* **Auto-Save Inteligente:** O progresso é guardado automaticamente a cada 20 alterações diretamente na mesma pasta do vídeo.

### ⌨️ Controlos da Interface Gráfica:
| Tecla | Ação |
|-------|------|
| `A` / `D` | Frame anterior / Frame seguinte |
| `Espaço` | Saltar para a próxima frame não anotada (Acelera o fluxo) |
| `1` / `2` | Limpar a marcação do Olho Esquerdo / Direito (para marcar novamente com o rato) |
| `3` / `4` | Alternar estado "Oculto/Fora do Vídeo" para o Olho Esquerdo / Direito |
| `X` | Alternar estado de "Ambos os Olhos Fechados" |
| `F` | Propagar as marcações da frame atual para as próximas 15 frames |
| `Backspace` | Limpar completamente a anotação da frame atual |
| `W` / `S` | Aumentar / Diminuir Brilho do vídeo |
| `C` | Ativar/Desativar contraste adaptativo (CLAHE) - útil para olhos escuros |
| `M` | Guardar progresso manualmente |
| `ESQ` | Terminar execução |

---

## 2. Gerar Previsões (`eyeLike` em Modo Headless)

O `eyeLike` pode correr em modo **Headless** (sem interface gráfica), permitindo processar os videos mais rapidamente ou correr em sistemas IoT limitados (i.e. Raspeberry Pi sem display para output gráfico).

**Como gerar o ficheiro de previsões (.csv):**
```bash
./build/bin/eyeLike -v testing/test_data/video_de_teste.mov -o testing/test_data/predictions_video_de_teste.csv --headless
```

---

## 3. Testes de Qualidade Académica (`testEyeTracking`)

Ferramenta que cruza o Ground Truth com as previsões do algoritmo, calculando a matemática olho a olho de forma independente.

**Como avaliar um vídeo já processado:**
```bash
./scripts/runTest.sh testing/test_data/video_de_teste.mov
```

### 📊 Relatórios Analíticos Gerados (na pasta `testing/test_reports/`):
1. **`report_video.txt` (Executive Summary):** * **RMSE (Root Mean Square Error):** Penaliza desvios grandes, métrica de excelência para *tracking*.
   * **Desvio Padrão (StdDev) & Média:** Avalia a estabilidade e o erro global da deteção.
   * **Mediana (Robustez):** Apresenta o erro típico isolando "outliers" (ex: perdas de face temporárias).
   * **Separação Espacial (Eixos X/Y):** Avalia a precisão horizontal versus vertical.
2. **`report_video.csv` (Base de Dados):** Dados numéricos limpos frame-a-frame para importação no Excel ou Python.

### 👁️ Interface de Validação Visual (HUD):
* `A` / `D`: Navegar frame a frame.
* `W` / `S`: Saltar rápido de 10 em 10 frames.
* **Dashboard em Tempo Real:** Uma consola sobreposta mostra os erros em pixeis, e desenha o texto a vermelho se o erro médio ultrapassar 15px.

---

## 4. Automação em Lote (Batch Testing Pipeline)

Pipeline "ponta-a-ponta" que automatiza a recolha de resultados para bases de dados.

**Modo de Vídeo Único (Com validações guiadas):**
```bash
./scripts/run_complete_test.sh testing/test_data/video_de_teste.mov
```
*(Verifica se o JSON existe, permite abrir o `annotateTool` na hora, corre o `eyeLike` em `--headless` e avalia no fim).*

**Modo BATCH (Processar múltiplos vídeos):**
```bash
./scripts/run_complete_test.sh --all
```
*(Procura todos os vídeos com anotações em `test_data/`, gera as previsões no background e compila os relatórios científicos para todos eles num único fluxo).*

---

## 📡 Integração MQTT para IoT
**Exemplo de Execução (Modo IoT Oculto):**
```bash
./build/bin/eyeLike -c -m tcp://localhost:1883 -t eyetracker/coordinates --mqtt-mode heartbeat --headless
```