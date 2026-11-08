# Luma Orbit 0.3.0 · plugin nativo

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
interno viene ripristinato fermo. Undo non è ancora presente.

Il **Sub intonato** segue le note due ottave sotto ed è attenuato durante la cassa.
Ritmo, Cassa, Clap e Hi-hat si attivano separatamente. Il volume generale è smussato;
uscita mono o stereo (gli effetti aprono il campo stereo), zero latenza dichiarata. Lo Stop silenzia tutte le voci.

## Effetti

Il pulsante **Effetti** apre i controlli di delay e riverbero; **Torna al suono**
ripristina i controlli del synth. Le quantità partono da zero, anche caricando una
sessione di una versione precedente. Tutti i parametri degli effetti si salvano
nella sessione e sono automatizzabili.

- **Quantita delay** aggiunge echi sincronizzati al tempo interno o della DAW.
  Scegli 1/8, 1/8 puntato o 1/4 dal menu. Il tempo massimo del delay è 4 secondi;
  i cambi di tempo sono smussati e possono produrre una breve variazione di intonazione.
- **Ripetizioni** controlla il feedback, limitato al 75%.
- **Riverbero** aggiunge ambiente stereo; **Dimensione stanza** regola la coda.
- Stop e i salti di posizione della DAW cancellano le code. Non è prevista una coda dopo Stop.

Gli stessi controlli sono disponibili sul web, con un motore audio distinto.

## Mixer, preset e registrazione

**Volume synth**, **Sub intonato** e **Volume ritmo** regolano le tre parti.
**Volume generale** è dopo delay e riverbero: a zero silenzia anche gli effetti.

**Salva preset** crea un file `.orbit`; **Apri preset** ripristina controlli,
effetti, scala, note e blocchi. Il salvataggio durante la riproduzione cattura le
note correnti. Il caricamento ferma il trasporto interno. I preset non conservano
pause, durate o posizione della frase generata; i file web JSON sono un formato distinto.

**Registra WAV** sceglie il file di destinazione e registra il mix finale PCM 16 bit,
alla frequenza di campionamento e con i canali dell'host. Premi **Ascolta** o avvia
la DAW, poi **Ferma e salva WAV** per chiudere il file. Il limite è cinque minuti;
Stop musicale può registrare silenzio finché non termini la registrazione.
Chiudere l'editor o cambiare dispositivo audio termina la registrazione.
La scrittura usa una coda e un thread separato; un overflow viene segnalato come
WAV incompleto. Non registra il microfono e non include altri strumenti della DAW.

## Compilazione e verifiche

```sh
cmake -S . -B build-universal -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13
cmake --build build-universal --target LumaOrbit_VST3 LumaOrbit_AU LumaOrbit_Standalone LumaOrbitTests -j 8
ctest --test-dir build-universal --output-on-failure
```

Aggiungi `-DJUCE_SOURCE_DIR=/percorso/JUCE` se disponi di JUCE 8.0.13 locale.
I test verificano cinque timbri a tre sample rate, clock, blocchi di dimensioni diverse,
blocchi delle note, trasporto host simulato, stato, preset, mixer, WAV e silenzio dopo Stop. La validazione
in DAW reali resta necessaria. Il codice originale è MIT; JUCE mantiene la propria licenza.
