# Changelog

Cronologia di Luma Orbit, versione web e plugin nativo. Le date seguono i tag
del repository; gli aggiornamenti web tra due tag sono riportati nella release successiva.
Luma Synth e Luma RMS mantengono versioni proprie.

## [0.4.0](https://github.com/avalla/luma-orbit/releases/tag/orbit-v0.4.0) — 2026-11-08

### Aggiunto

- Quattro scene: Intro, Groove, Apertura e Finale. Salvataggio esplicito e richiamo
  alla prossima battuta di 4/4 durante l'ascolto, immediato a trasporto fermo.
- Swing, probabilità degli eventi melodici e dei colpi di batteria, macro Energia
  e Fill temporaneo di una battuta, sul web e nel plugin.
- Cattura nativa completa: note, pause, durate e pattern batteria conservati dopo
  Stop, caricamento del preset e riapertura della sessione DAW.
- Preset JSON condivisi `luma-orbit`, versione 2, con quattro scene; importazione
  dei vecchi JSON web e, nel plugin, dei file binari `.orbit`.
- Export della frase in MIDI con tre tracce: synth, sub e batteria GM, tempo e swing.
- Registrazione simultanea di tre WAV stereo asciutti, separati e allineati,
  fino a cinque minuti; resta disponibile la registrazione del mix con gli effetti.
- Pagina nativa **Effetti e scene**, sezione web **Composizione** e test di scambio
  preset tra i due motori nella pipeline GitHub Actions.

### Corretto e verificato

- Richiesta della modalità audio musicale sui browser che supportano Audio Session,
  per il problema di Safari/iPhone muto con sequencer in movimento; messaggi di stato
  e indicazioni per i browser senza supporto. Output su iPhone fisico ancora da verificare.
- Verificati test web e nativi, esecuzione x86_64 tramite Rosetta, build universali,
  scene a tempo, MIDI leggibile, WAV allineati e layout mobile a 390 px.

### Compatibilità

- I vecchi preset ricevono Swing 0, Probabilità 100% ed Energia 50% (neutra).
- Il formato condiviso trasferisce lo stato musicale, non waveform identiche,
  cronologia Annulla, posizione di riproduzione o future scelte casuali.
- L'export MIDI descrive la frase; non registra Probabilità, Fill, effetti o automazioni.
- Il plugin non riceve MIDI esterno. Le build macOS hanno firma ad-hoc, non notarizzazione.

## [0.3.0](https://github.com/avalla/luma-orbit/releases/tag/orbit-v0.3.0) — 2026-11-08

- Delay sincronizzato a 1/8, 1/8 puntato o 1/4, con feedback massimo 75%.
- Riverbero con dimensione stanza e mixer separato per synth, sub e batteria.
- Preset su file e registrazione WAV del mix finale fino a cinque minuti,
  sia sul web sia nel plugin.
- Volumi smussati, cancellazione delle code a Stop e valori neutri per gli effetti
  nelle sessioni anteriori alla loro introduzione.
- In questa versione i preset web JSON e nativi `.orbit` avevano formati distinti;
  la cattura nativa conservava soltanto le note.

## [0.2.1](https://github.com/avalla/luma-orbit/releases/tag/orbit-v0.2.1) — 2026-11-08

- Corrette le selezioni delle note nell'editor nativo durante l'aggiornamento
  delle etichette al cambio di scala.

## 0.2.0 — 2026-11-08

- Introdotto Orbit nativo come VST3, Audio Unit e app standalone macOS.
- Sequencer Flusso, sincronizzazione con la DAW, cinque timbri, sub intonato
  e batteria elettronica; estesa anche la versione web.
- Build universali Intel/Apple Silicon e pacchetti scaricabili tramite GitHub Actions.

Vedi il [tag 0.2.0](https://github.com/avalla/luma-orbit/tree/orbit-v0.2.0),
la [guida nativa](docs/Luma-Orbit.md) e la [guida web](prototypes/orbit/README.md).
