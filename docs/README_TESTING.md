# Eye Tracking Testing Framework & Annotation Tool

Este repositório inclui um ecossistema completo para testar, anotar e avaliar a precisão do algoritmo `eyeLike`. As ferramentas foram desenhadas para serem robustas, altamente visuais e gerarem relatórios de qualidade académica para análise de dados.

## 🚀 Como Compilar as Ferramentas

As ferramentas de teste partilham o mesmo sistema de build do projeto principal (CMake). Para compilar as ferramentas de anotação e avaliação, corre o seguinte script na raiz do projeto:

```bash
./scripts/buildTesting.sh
```
Isto irá gerar os executáveis `annotationTool` e `testEyeTracking` dentro da pasta `build/`.

---

## 1. Ferramenta de Anotação (`annotationTool`)

Uma ferramenta gráfica para criar a "Verdade Terrestre" (Ground Truth) num vídeo. Permite marcar manualmente o centro dos olhos humana e visualmente para futura comparação com a previsão do algoritmo.

**Como correr:**
```bash
./build/annotationTool testing/test_data/video_de_teste.mov
```

### ✨ Funcionalidades Avançadas de Anotação:
* **Validação de Olhos Fechados (Tecla X):** Se a pessoa fechar os olhos no vídeo, prime `X`. A frame será marcada como "olhos fechados". O teste de erro ignorará a matemática destas frames específicas para não penalizar a precisão final do algoritmo com "falsos positivos".
* **Timeline Visual (Barra de Progresso):** Na base do ecrã, uma barra de progresso dinâmica mapeia o teu trabalho:
  * **Cinzento:** Frames não anotadas.
  * **Verde:** Frames corretamente anotadas (Olhos Abertos).
  * **Amarelo:** Frames marcadas propositadamente como "Olhos Fechados".
  * **Linha Vermelha:** O cursor indicando a frame atual.
* **Auto-Save Inteligente:** Para prevenir perda de dados, o progresso é guardado automaticamente a cada 20 cliques/alterações diretamente na mesma pasta do vídeo (ex: `testing/test_data/ground_truth_video.json`).

### ⌨️ Controlos da Interface Gráfica:
| Tecla | Ação |
|-------|------|
| `A` / `D` | Frame anterior / Frame seguinte |
| `Espaço` | Saltar para a próxima frame não anotada (Acelera o fluxo) |
| `1` / `2` | Marcar Olho Esquerdo (Azul) / Direito (Verde) com o rato |
| `X` | Alternar estado de "Olhos Fechados" |
| `F` | Propagar a marcação atual para as próximas 15 frames |
| `Backspace` | Limpar anotação da frame atual |
| `W` / `S` | Aumentar / Diminuir Brilho do vídeo |
| `C` | Ativar/Desativar contraste adaptativo (CLAHE) - útil para olhos escuros |
| `M` | Guardar progresso manualmente |

---

## 2. Gerar Previsões (`eyeLike` em Modo Headless)

O `eyeLike` pode agora correr em modo **Headless** (sem interface gráfica e sem renderização de janelas). Isto permite processar vídeos a velocidades extremamente altas usando 100% da capacidade de cálculo do processador, sendo também o modo ideal para correr o software em sistemas IoT limitados (como o Raspberry Pi).

**Como gerar o ficheiro de previsões (.csv):**
```bash
./build/bin/eyeLike -v testing/test_data/video_de_teste.mov -o testing/test_data/predictions_video_de_teste.csv --headless
```

---

## 3. Testes de Qualidade Académica (`testEyeTracking`)

Esta ferramenta cruza os dados do teu Ground Truth com as previsões geradas pelo algoritmo e extrai um relatório científico.

**Como correr (usando o script que deteta as dependências automaticamente):**
```bash
./scripts/runTest.sh testing/test_data/video_de_teste.mov
```

### 📊 Relatórios Analíticos Gerados (na pasta `testing/test_reports/`):
A ferramenta exporta os dados em dois formatos distintos para facilitar a interpretação e a análise estatística:

1. **`report_video.txt` (Executive Summary):** Um resumo de fácil leitura que categoriza o erro global e divide a performance do algoritmo entre o olho esquerdo e direito.
   * **RMSE (Root Mean Square Error):** Métrica de rigor científico que penaliza desvios/erros grandes de tracking.
   * **Mediana (Robustez):** Apresenta o erro típico ignorando "outliers" (ex: quando o algoritmo perde o rosto temporariamente).
   * **Separação Espacial (Eixo X vs Eixo Y):** Desmonta o erro de tracking para entender se o algoritmo tem maior dificuldade em seguir o movimento horizontal (X) ou vertical (Y) dos olhos.
2. **`report_video.csv` (Base de Dados):** Dados numéricos limpos frame-a-frame, concebidos para serem importados no Excel, Python ou R para a criação de gráficos de dispersão (Scatter Plots) ou futuro treino de modelos de Machine Learning.

### 👁️ Interface de Validação Visual (HUD):
Durante o teste, a janela exibe o vídeo em tempo real com os dados sobrepostos:
* **Círculos Preenchidos:** Verdade Terrestre (Onde o olho realmente está).
* **Círculos Vazados + Linhas:** Previsão do algoritmo e a direção do desvio.
* **Dashboard em Tempo Real:** Uma consola no ecrã apresenta as distâncias de erro exatas. Se o erro médio da frame ultrapassar os 15 pixeis, o texto pinta-se de vermelho para evidenciar a falha de deteção.

---

## 4. Automação em Lote (Batch Testing Pipeline)

Para lidar com bases de dados grandes, o repositório inclui um script de pipeline "ponta-a-ponta" que automatiza a recolha de resultados.

**Modo de Vídeo Único (Com validações guiadas):**
```bash
./scripts/run_complete_test.sh testing/test_data/video_de_teste.mov
```
*(Verifica as dependências, permite anotar na hora se faltar o Ground Truth e corre os testes em sequência).*

**Modo BATCH (Processar todos os vídeos em simultâneo):**
```bash
./scripts/run_complete_test.sh --all
```
*(O sistema irá procurar todos os vídeos na pasta de testes, isolar apenas os que já contêm anotações, gerar as previsões `--headless` no background e calcular os relatórios científicos para todos eles num único fluxo de trabalho).*

---

## 📡 Integração MQTT para IoT

O `eyeLike` suporta o envio contínuo de coordenadas processadas para um Broker MQTT, o que permite criar dashboards na cloud em tempo real.

**Exemplo de Execução (Modo IoT Oculto):**
```bash
./build/bin/eyeLike -c -m tcp://localhost:1883 -t eyetracker/coordinates --mqtt-mode heartbeat --headless
```
* **Modos de Operação MQTT:** * `production`: Envio contínuo (QoS 0, Payload leve).
  * `debug`: Payload extenso contendo coordenadas brutas e ID da câmara.
  * `heartbeat`: Envio reativo (QoS 1, Payload retido). Transmite apenas quando existe uma alteração significativa de posição na face ou olhos, poupando largura de banda de rede.