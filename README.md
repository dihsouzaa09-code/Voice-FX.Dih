# Worship Voice FX

Plugin de efeitos vocais para louvor (VST3 / AU / Standalone), feito em C++ com JUCE.

## Cadeia de sinal

Entrada → Low Cut → De-Esser → Compressor (+makeup) → Saturação → EQ (Calor / Presença / Ar) → Coro → Delay → Reverb → Mix Global → Saída

- **Delay:** livre (ms) ou sincronizado ao BPM da DAW (1/4, 1/8, 1/8 pontuada, 1/2), com filtro no feedback
- **Reverb:** pre-delay, low cut fixo (220 Hz) e filtro de agudos ajustável, para um espaço limpo
- **Presets:** Worship Hall, Catedral, Intimista/Acústico, Ambiente (delay longo), Líder (presença), Coro largo, Neutro

## Como compilar

Requisitos: CMake 3.22+, Git e um compilador C++17
(Windows: Visual Studio 2022 · macOS: Xcode · Linux: GCC/Clang + libs do JUCE).

```bash
cd WorshipVoiceFX
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

O JUCE é baixado automaticamente. Ao final, o plugin é copiado para a pasta de plugins do sistema:

- Windows: `C:\Program Files\Common Files\VST3`
- macOS: `~/Library/Audio/Plug-Ins/VST3` e `.../Components`

Abra a DAW, rescaneie os plugins e insira **Worship Voice FX** na faixa de voz.
Também é gerado um app Standalone para testar com o microfone.

## Dicas de uso

- Use como insert na faixa vocal (Mix Global em 100%) ou em um bus de efeitos (Mix Global em 30–50%).
- Para um som mais ambiente, aumente Reverb Mix e use o preset "Ambiente (delay longo)".

## Sem compilar no seu PC (GitHub Actions)

1. Crie um repositório no GitHub e envie esta pasta (branch `main`).
2. Abra a aba **Actions**, escolha **Build Windows VST3** e clique em **Run workflow** (ou espere o build automático do push).
3. Ao terminar (uns 10 minutos), baixe o artefato **Worship-Voice-FX-VST3**, descompacte e copie a pasta `Worship Voice FX.vst3` para `C:\Program Files\Common Files\VST3`.
4. No FL Studio: *Options → Manage plugins → Find installed plugins*.
