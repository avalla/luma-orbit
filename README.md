# Luma Orbit / Synth + RMS

**[Apri Orbit](https://orbit.avalla.dev)**: synth e sequencer generativo nel browser,
con batteria elettronica e pianificazione sul clock audio Web Audio.

Il repository contiene il prototipo web Orbit e due plugin nativi macOS.

## Orbit web

Apri `prototypes/orbit/index.html` in un browser. Non richiede un build o dipendenze
per l'esecuzione. Per funzionalità, limiti e pubblicazione Cloudflare vedi
[la guida Orbit](prototypes/orbit/README.md).

Test del generatore e del clock (Node.js):

```sh
node prototypes/orbit/flow.test.cjs
node prototypes/orbit/clock.test.cjs
```

La UI e il motore web sono separati dai plugin JUCE descritti sotto.

## Plugin nativi

Due plugin nativi C++ / JUCE 8.0.13 per macOS: sintetizzatore Luma e misuratore Luma RMS.

## Luma RMS

Effetto di analisi da inserire sulla traccia o sul master della DAW. Disponibile
come VST3 e Audio Unit, con ingresso/uscita mono o stereo. Non modifica i campioni
audio e non introduce latenza. La misura dipende dalla posizione nella catena:
inseriscilo dopo gli effetti di cui vuoi misurare il risultato.

- RMS in tempo reale con finestra mobile di 100, 400 o 1000 ms (default 400 ms).
- Letture separate L/R e valore principale come media energetica dei canali:
  `10 * log10((mean(L²) + mean(R²)) / 2)`. In mono usa il solo canale attivo.
- Picco campione massimo per canale, mantenuto fino a “Reset peak hold”.
  Il reset viene applicato al prossimo blocco audio.
- Durante il riempimento iniziale, RMS calcolato sui campioni disponibili.
  Un cambio di finestra riavvia il riempimento; la finestra si salva nella sessione.
- Barra da -60 a 0 dBFS, valore numerico anche oltre questi limiti; silenzio = `-inf`.
- RMS matematico, senza ponderazione o compensazione: una sinusoide con picco
  a 0 dBFS misura -3.0103 dBFS RMS. Non misura LUFS o true peak.

L'interfaccia si aggiorna a 30 Hz; il motore aggiorna la misura a ogni callback
audio. Quando la DAW sospende l'elaborazione restano visibili gli ultimi valori.
Anche premendo Stop l'ultima lettura e il picco restano visibili; al successivo
avvio il plugin inizia una nuova misura. La riga di stato distingue audio in
ingresso, ultima lettura e attesa della riproduzione.

Artefatti in `build/LumaRms_artefacts/Release/`:

- `VST3/Luma RMS.vst3` → `~/Library/Audio/Plug-Ins/VST3/`.
- `AU/Luma RMS.component` → `~/Library/Audio/Plug-Ins/Components/`.

Riavvia la DAW o esegui una scansione dopo la copia. Il plugin compare come
**Luma RMS**, effetto di analisi del produttore **Luma Audio**.

## Suonare

8 voci, due oscillatori sine / triangle / saw con miscela e detune, sub sinusoidale,
filtro passa basso risonante con inviluppo indipendente, ADSR di volume, chorus
stereo, volume e pitch bend ±2 semitoni. Sei preset: Init, Velvet Pad,
Rubber Bass, Glass Pluck, Solar Lead e Soft Keys. I preset si scelgono nell'interfaccia;
le modifiche manuali vengono indicate come “Custom / modified”.
La tastiera a schermo funziona con il mouse;
in una DAW crea una traccia strumento e invia MIDI a Luma.
La tastiera del computer suona da Do centrale (MIDI 60): `A S D F G H J K`
per i tasti bianchi, `W E T Y U` per i diesis. Tieni premuti più tasti per gli accordi.
Dopo aver modificato un valore, clicca “Suona con la tastiera” o sul pianoforte
per riprendere a suonare. La perdita del focus rilascia le note della tastiera.
In una DAW il programma ospite deve inoltrare i tasti al plugin.
Le manopole accettano valori digitati; doppio clic ripristina il valore iniziale.
Parametri automatizzabili e stato salvato con il progetto della DAW.

## Compilazione

Richiede Xcode con Command Line Tools, CMake >=3.22 e accesso a GitHub al primo avvio.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
ctest --test-dir build --output-on-failure
```

Per usare una copia locale di JUCE aggiungi `-DJUCE_SOURCE_DIR=/percorso/JUCE`.
Gli artefatti sono in `build/Luma_artefacts/Release/`:

- `Standalone/Luma.app`: applicazione per prova diretta.
- `VST3/Luma.vst3`: copia in `~/Library/Audio/Plug-Ins/VST3/`.
- `AU/Luma.component`: copia in `~/Library/Audio/Plug-Ins/Components/`.

La build non installa automaticamente i plugin. Dopo l'installazione riavvia la DAW
o esegui una nuova scansione. La build locale usa l'architettura del Mac corrente;
per una build universale configura `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

Build e pacchetto universale (Intel + Apple Silicon):

```sh
cmake -S . -B build-universal -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13
cmake --build build-universal -j 8
ctest --test-dir build-universal --output-on-failure
python3 scripts/package_plugins.py --build-dir build-universal --universal \
  --juce-dir /percorso/JUCE
```

Lo ZIP viene creato in `build/Delivery/Luma-Plugins-macOS-Universal.zip`.
I bundle contengono entrambe le architetture. Minimo di compilazione: macOS 10.13
su Intel e macOS 11 su Apple Silicon; la compatibilità con vecchie versioni del
sistema va verificata su tali macchine. Lo script verifica architetture, firme
ad-hoc e integrità dello ZIP e include i sorgenti esatti usati per la build.

## Struttura e limiti

`Source/Parameters.h` definisce parametri e preset del synth; `Voice.h` contiene
le voci, `Plugin.cpp` il processore ed `Editor.cpp` l'interfaccia. Un solo albero di
parametri alimenta DSP, automazione e controlli. Il motore usa voci preallocate,
una saw PolyBLEP e una triangle additiva limitata a 8 armoniche dispari.
Cutoff, risonanza, miscela, sub, detune, profondità dell'inviluppo e volume sono
smussati. Due chorus mono con modulazioni diverse producono l'apertura stereo;
la memoria viene allocata in preparazione. Le sessioni v0.1 mantengono i loro
parametri e ricevono valori neutri per i nuovi controlli.

`Source/Rms/RmsEngine.h` contiene il calcolo della finestra mobile, indipendente
da JUCE. Il processore RMS legge il buffer senza modificarlo e pubblica le misure
tramite atomiche; la conversione in dBFS avviene nell'interfaccia.
Non ci sono campioni, browser web o asset esterni.

`Tests/Smoke.cpp` verifica suono finito, otto voci, tre forme d'onda e frequenze
di campionamento, silenzio dopo il rilascio e ripristino dello stato.
Il consumo CPU deve ancora essere misurato nella DAW di destinazione.
`Tests/Rms.cpp` verifica calibrazione su sinusoide e segnale costante, finestra
mobile, indipendenza dai blocchi, mono/stereo, audio identico bit per bit,
picco, reset, bypass e stato. Genera anche `build/Luma-RMS-preview.png`.
Mancano validazione multi-host, firma Developer ID e notarizzazione.
I nomi e gli identificatori del produttore sono provvisori.

## Licenza

Il codice originale di questo progetto è distribuito con [licenza MIT](LICENSE).
JUCE e le altre dipendenze mantengono le proprie licenze: la licenza MIT del
progetto non le sostituisce. JUCE è disponibile con licenza AGPL/commerciale;
scegliere e rispettare la licenza appropriata prima della distribuzione dei plugin.
