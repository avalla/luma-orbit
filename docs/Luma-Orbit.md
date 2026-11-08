# Luma Orbit · plugin nativo

Synth e sequencer generativo con batteria elettronica, sub intonato e cinque timbri:
Glass, Rubber, Mist, Soft Keys e Bell. Il motore C++ genera l'audio direttamente
nel callback della DAW; non incorpora browser, JavaScript o servizi remoti.

## Installazione macOS

- Copia `VST3/Luma Orbit.vst3` in `~/Library/Audio/Plug-Ins/VST3/`.
- Oppure copia `AU/Luma Orbit.component` in `~/Library/Audio/Plug-Ins/Components/`.
- Riavvia la DAW ed esegui la scansione. Crea una traccia strumento e scegli Luma Orbit.
- Per provarlo senza DAW, apri `Standalone/Luma Orbit.app` e premi **Ascolta**.

Il pacchetto Universal include Intel e Apple Silicon. Minimi di compilazione:
macOS 10.13 Intel e macOS 11 Apple Silicon; i vecchi sistemi e tutte le DAW non
sono stati verificati. Firma ad-hoc, senza Developer ID o notarizzazione.

## Trasporto e composizione

**Sync DAW** segue Play/Stop, BPM e posizione musicale forniti dall'host. Quando
è attivo un trasporto host, Ascolta e Tempo interno sono disabilitati. Disattiva
Sync DAW per usare il trasporto interno. L'app standalone usa il trasporto interno.
Le note vengono attivate sul campione della griglia degli ottavi; cambi di posizione
o loop della DAW ripartono con una nuova frase deterministica. Non riceve MIDI esterno
e non esporta MIDI in questa versione.

**Flusso** sviluppa frasi da 8, 12 o 16 eventi. Esplora controlla variazione melodica,
pause e durate. A zero, o con tutte le note bloccate, non introduce nuove variazioni.
Gli accenti hanno un ciclo indipendente di 5, 7 o 8 passi. I controlli sono automatizzabili.

**Cattura le note** salva gli otto gradi correnti nei parametri e disattiva Flusso.
La frase in corso continua; al riavvio si parte dagli otto gradi salvati. Questa prima
versione nativa non serializza le pause e le durate generate né la posizione del generatore.
La DAW conserva controlli, timbro, scala, note e blocchi nella sessione. Il trasporto
interno viene ripristinato fermo. Non sono ancora presenti Undo e preset su file.

Il **Sub intonato** segue le note due ottave sotto ed è attenuato durante la cassa.
Ritmo, Cassa, Clap e Hi-hat si attivano separatamente. Il volume generale è smussato;
uscita mono o stereo dual-mono, zero latenza dichiarata. Lo Stop silenzia tutte le voci.

## Compilazione e verifiche

```sh
cmake -S . -B build-universal -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13
cmake --build build-universal --target LumaOrbit_VST3 LumaOrbit_AU LumaOrbit_Standalone LumaOrbitTests -j 8
ctest --test-dir build-universal --output-on-failure
```

Aggiungi `-DJUCE_SOURCE_DIR=/percorso/JUCE` se disponi di JUCE 8.0.13 locale.
I test verificano cinque timbri a tre sample rate, clock, blocchi di dimensioni diverse,
blocchi delle note, trasporto host simulato, stato e silenzio dopo Stop. La validazione
in DAW reali resta necessaria. Il codice originale è MIT; JUCE mantiene la propria licenza.
