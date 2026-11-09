# Luma Orbit 0.4.0 · plugin nativo

Synth e sequencer generativo con batteria elettronica, sub intonato e cinque timbri:
Glass, Rubber, Mist, Soft Keys e Bell. Il motore C++ genera l'audio direttamente
nel callback della DAW; non incorpora browser, JavaScript o servizi remoti.

[Guida rapida](Orbit-quickstart.md) · [Changelog](../CHANGELOG.md) · [Guida web](../prototypes/orbit/README.md)

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
o loop della DAW riavviano la sequenza; una frase catturata conserva pause e durate.
Non riceve MIDI esterno ma esporta la frase corrente come file MIDI.

**Flusso** sviluppa frasi da 8, 12 o 16 eventi. Esplora controlla variazione melodica,
pause e durate. Con Energia neutra ed Esplora a zero, o con tutte le note bloccate,
non modifica i gradi melodici durante l’evoluzione.
Gli accenti hanno un ciclo indipendente di 5, 7 o 8 passi. I controlli sono automatizzabili.

**Cattura la frase** conserva gli otto gradi, la sequenza completa di eventi,
pause, durate e pattern batteria, e disattiva Flusso. La cattura riparte dall'inizio
della frase; resta intatta dopo Stop, riapertura del preset e ripristino della sessione.
Non conserva il punto di riproduzione né lo stato futuro del generatore casuale.
La DAW conserva controlli, timbro, scala, note, blocchi e scene. Il trasporto interno
viene ripristinato fermo. Undo nativo non è ancora presente.

## Scene e movimento

Apri **Effetti e scene**. I quattro slot **Intro, Groove, Apertura, Finale** sono
inizialmente vuoti: **Salva qui** sostituisce esplicitamente il contenuto dello slot
con l'idea corrente. Richiamare una scena ferma è immediato; durante l'ascolto il
cambio avviene alla prossima battuta di quattro quarti. Con Sync DAW il tempo resta
quello dell'host. Il cambio riguarda suono, frase, batteria, mixer ed effetti.
Salva il preset o la sessione DAW per conservare tutti gli slot.

- **Swing**: 0–0.60, ritarda il secondo ottavo di ogni coppia mantenendo la lunghezza della battuta.
- **Probabilità colpi**: 0–1, probabilità di suonare ciascun evento melodico e colpo
  di batteria. Le note bloccate suonano sempre, salvo le pause già nella frase.
- **Energia**: 0.50 è neutra; modula luminosità, esplorazione e densità senza modificare
  i loro valori salvati o il volume generale. Il ritmo evolve solo in Flusso.
- **Fill**: arma una variazione di batteria di una battuta, dal prossimo inizio battuta.
  Non riscrive il pattern salvato; rispetta le voci attive e la probabilità.

Il **Sub intonato** segue le note due ottave sotto ed è attenuato durante la cassa.
Ritmo, Cassa, Clap e Hi-hat si attivano separatamente. Il volume generale è smussato;
uscita mono o stereo (gli effetti aprono il campo stereo), zero latenza dichiarata. Lo Stop silenzia tutte le voci.

## Effetti

Il pulsante **Effetti e scene** apre i controlli di delay e riverbero; **Torna al suono**
ripristina i controlli del synth. Le quantità partono da zero nello strumento appena
inizializzato e nelle sessioni anteriori alla loro introduzione in 0.3. Le sessioni
0.3 conservano gli effetti salvati. Tutti i parametri degli effetti si salvano nella
sessione e sono automatizzabili.

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

**Salva preset** crea un file JSON `luma-orbit`, versione 2, leggibile anche sul web.
**Apri preset** accetta JSON condivisi, vecchi JSON web e vecchi file binari `.orbit`.
Un file non valido viene rifiutato prima di modificare lo strumento. Il caricamento
ferma il trasporto interno; una DAW già in Play continua a comandare il trasporto.
Il formato conserva frase, pattern, controlli e quattro scene; non conserva Undo,
punto di riproduzione o il futuro casuale. I due motori hanno timbri analoghi, non
waveform identiche. I vecchi preset ricevono Swing 0, Probabilità 1 ed Energia 0.50.

**Esporta MIDI** salva tre tracce: melodia (canale 1), sub (2), batteria GM (10;
cassa 36, clap 39, hi-hat 42), a 480 tick per quarto. Include tempo, pause, durate,
accenti e swing della frase corrente. Esclude la casualità di Probabilità, il Fill,
le future evoluzioni di Flusso, effetti e automazioni: è materiale da arrangiare nella DAW.

**Registra 3 tracce** sceglie un nome base e crea `-synth.wav`, `-sub.wav`, `-drums.wav`.
Scegli un nome base nuovo: file già esistenti non vengono sovrascritti.
Sono tre file stereo PCM 16 bit, simultanei, dopo i volumi degli strumenti ma prima
di saturazione del mix, effetti e volume generale. Il sub include l'attenuazione della cassa.
Premi di nuovo per terminarli; massimo cinque minuti. Non sono una scomposizione
matematicamente identica del mix, che ha saturazione ed effetti condivisi.

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
ctest --test-dir build-universal -R orbit_sequencer --output-on-failure
node prototypes/orbit/flow.test.cjs
node prototypes/orbit/clock.test.cjs
node prototypes/orbit/session.test.cjs
node prototypes/orbit/interchange.test.cjs build-universal/orbit-native-preset.json
```

Aggiungi `-DJUCE_SOURCE_DIR=/percorso/JUCE` se disponi di JUCE 8.0.13 locale.
I test verificano cinque timbri a tre sample rate, clock, blocchi di dimensioni diverse,
blocchi delle note, trasporto host simulato, stato, preset condivisi, cattura dopo riavvio, scene a battuta, swing, probabilità,
MIDI leggibile, mixer, WAV allineati e silenzio dopo Stop. La validazione
in DAW reali resta necessaria. Il codice originale è MIT; JUCE mantiene la propria licenza.

Il test `orbit_sequencer` scrive il JSON usato dal test di scambio e le anteprime
`Luma-Orbit-preview.png` e `Luma-Orbit-scenes-preview.png` nella directory di build.
La pipeline della [release 0.4.0](https://github.com/avalla/luma-orbit/actions/runs/42527775440)
ha completato build universale, test, scambio preset e verifica del pacchetto.
Le verifiche locali includono anche gli altri due plugin e l'esecuzione Orbit x86_64
tramite Rosetta; non equivalgono a una prova su un Mac Intel fisico o su tutte le DAW.
