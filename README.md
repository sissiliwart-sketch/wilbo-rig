# Wilbo Rig — pedaleira virtual para guitarra

Programa para Windows (e plugin VST3) feito em C++ com o framework JUCE.

```
Guitarra > Noise Gate > Wah > Drive > Amplificador > Gabinete > Delay > Reverb > Saída
                 └──> Afinador
```

| Bloco | O que faz |
|---|---|
| **Noise Gate** | Corta o chiado quando você não está tocando. Suba o knob GATE até o chiado sumir. |
| **Afinador** | Afinador cromático (algoritmo YIN). O botão AFINADOR silencia a saída enquanto você afina. |
| **Wah** | Wah manual (knob PEDAL) ou **Auto-wah**, que abre conforme a força da palhetada. |
| **Drive** | Três modos: Overdrive, Distortion e Fuzz. Usa oversampling 4x para evitar o "chiado metálico" (aliasing). |
| **Amplificador** | Pré de válvula com dois estágios e equalização Bass, Middle, Treble e Presence, mais o Master. |
| **Gabinete** | Usa um simulador interno de caixa 4x12 ou um **arquivo IR (.wav)** que você carrega. |
| **Delay** | De 20 ms a 2 s, com repetições que vão escurecendo (TONE). |
| **Reverb** | Reverb estéreo de sala. |

Duplo clique em qualquer knob volta ao valor padrão. O programa guarda os ajustes ao fechar.

---

## 1. Como compilar no Windows

### Opção A: no seu PC

1. Instale o **Visual Studio 2022 Community** (gratuito). No instalador, marque a carga de trabalho **"Desenvolvimento para desktop com C++"**, que já inclui o CMake.
2. Instale o **Git for Windows**.
3. Abra o **Developer PowerShell for VS 2022**, entre na pasta do projeto e rode:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

A primeira vez demora alguns minutos, porque o CMake baixa o JUCE sozinho. O programa fica em:

```
build\WilboRig_artefacts\Release\Standalone\Wilbo Rig.exe
build\WilboRig_artefacts\Release\VST3\Wilbo Rig.vst3   (plugin para Reaper, Cakewalk etc.)
```

Se preferir usar uma IDE, abra `build\WilboRig.sln` no Visual Studio.

### Opção B: deixar o GitHub compilar

1. Crie um repositório no GitHub e envie esta pasta.
2. Abra a aba **Actions**. O fluxo **Build Windows** roda sozinho, compila e roda os testes.
3. Ao terminar, baixe o arquivo **WilboRig-Windows** em *Artifacts*. Ele traz o `.exe` pronto.

---

## 2. Configurar o áudio com o cabo USB/P10 da Alesis

O cabo da Alesis aparece no Windows como uma placa de som USB (geralmente "USB Audio CODEC"). Ele não tem driver ASIO, então use o modo **WASAPI Exclusivo**, que dá pouca latência:

1. Abra o Wilbo Rig e clique em **Options › Audio/MIDI Settings**.
2. Em **Audio device type**, escolha **Windows Audio (Exclusive Mode)**. Se der erro, use **Windows Audio (Low Latency Mode)**.
3. Em **Input**, selecione o cabo da Alesis. Em **Output**, selecione o fone ou a caixa de som.
4. Em **Audio buffer size**, comece com **128 amostras**. Se ouvir estalos, suba para 256.
5. Na faixa amarela "Audio input is muted…", clique em **Settings…** e desmarque **Mute audio input**. O programa lembra essa escolha. O JUCE deixa a entrada muda por padrão para evitar microfonia com microfone.

### Se continuar com chiado

- Com o notebook ligado na tomada, teste usando só a bateria. Ruído de terra do carregador é a causa mais comum de zumbido em cabos USB de guitarra.
- Deixe o volume da guitarra no máximo e ajuste o **INPUT** do programa. Assim o sinal fica bem acima do ruído do cabo.
- Use o **Noise Gate** (knob GATE entre −65 e −50 dB).
- Abaixe o **Presence** e o **Treble** do amplificador, ou carregue um IR de gabinete. Agudos sem filtro de caixa soam como "lixa".

---

## 3. IRs de gabinete (opcional, mas melhora muito o som)

Um IR (*impulse response*) é a "impressão digital" sonora de uma caixa de som com microfone. Na internet há vários pacotes gratuitos em `.wav`. Pesquise por "free guitar cab IR". Em **GABINETE › Carregar IR (.wav)**, escolha o arquivo. O botão **Usar simulador interno** volta para o filtro interno.

---

## 4. Estrutura do código

```
CMakeLists.txt              configuração da compilação (baixa o JUCE)
Source/
  Parameters.h/.cpp         lista de todos os knobs/botões (IDs, faixas, valores padrão)
  PluginProcessor.h/.cpp    cadeia de áudio: liga os blocos na ordem
  PluginEditor.h/.cpp       tela principal (layout)
  DSP/                      processamento de áudio, um arquivo por efeito
    Tuner  Wah  Drive  Amp  Cabinet  Delay
  UI/                       visual: knobs, pedais, afinador, cores
Tests/TestMain.cpp          testes automáticos (afinador, estabilidade, delay, presets)
```

### Como adicionar um pedal novo (ex.: Chorus)

1. Em `Parameters.h/.cpp`, crie os parâmetros (`chorusOn`, `chorusRate`, …).
2. Em `Source/DSP/`, crie `Chorus.h/.cpp` com `prepare()` e `process(float*, int)`, seguindo o modelo do `Delay`.
3. Em `PluginProcessor.cpp`, chame `chorus.process(x, n)` no ponto da cadeia que você quiser.
4. Em `PluginEditor`, crie um `PedalComponent` com os knobs e posicione-o em `resized()`.
5. Acrescente o arquivo `.cpp` em `WILBO_SOURCES`, no `CMakeLists.txt`.

### Rodar os testes

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DWILBO_BUILD_TESTS=ON
cmake --build build --config Release --target WilboTests
build\WilboTests_artefacts\Release\WilboTests.exe
```

Os testes também geram um `demo.wav` com um riff sintético passando pela pedaleira.

---

## 5. Próximos passos sugeridos

- **Presets**: salvar e carregar timbres com nome (Clean, Crunch, Solo…).
- **Pedal de expressão / controladora MIDI** para o wah e para ligar e desligar os pedais com o pé.
- **Driver ASIO**, para interfaces de áudio com driver próprio.
- Mais pedais: chorus, phaser, compressor, octaver.
- Modo de **reordenar pedais** arrastando.

## Licença do JUCE

O JUCE é gratuito para projetos pessoais e de código aberto (AGPLv3). Para **vender** o programa com código fechado, é preciso uma licença do JUCE. Veja os planos em juce.com.
