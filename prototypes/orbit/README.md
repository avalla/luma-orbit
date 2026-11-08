# Luma Orbit 0.3 · web

Apri https://orbit.avalla.dev e premi **Ascolta**. Per sviluppo locale usa `python3 -m http.server 8765 --directory prototypes/orbit` dalla radice e apri http://localhost:8765. La registrazione AudioWorklet richiede HTTPS o localhost. Nessuna dipendenza frontend o accesso al microfono.

- Otto note modificabili nella scala scelta; ogni passo può essere bloccato.
- Accenti su un ciclo indipendente di 5, 7 o 8 passi.
- Flusso, attivo inizialmente, evolve automaticamente alla fine di ogni frase: 8, 12 o 16 eventi, note gradualmente diverse, pause e durate di uno o due ottavi. I primi otto eventi attraversano tutti i passi; le code più lunghe riprendono liberamente il materiale. Gli accenti continuano sul loro ciclo indipendente.
- Esplora controlla ampiezza delle variazioni e probabilità di pause e durate lunghe. A zero (o con tutti i passi bloccati), dalla frase successiva non vengono introdotte variazioni. La prima frase presenta l'idea originale. Nuova variazione resta disponibile per una modifica manuale.
- Cattura conserva la frase corrente, incluse pause e durate, e passa a Ripeti. Flusso riattiva l'evoluzione. Non garantisce musica che non si ripeta mai: sviluppa il materiale di otto note in frasi variabili.
- Annulla ripristina modifiche di note, blocchi, scala e catture, fino a 32 operazioni.

Le evoluzioni automatiche non riempiono la cronologia. Annullare Cattura ripristina la modalità precedente e la frase al momento della cattura. Salva preset conserva un file JSON da riaprire con Apri preset.

Verifica del generatore: `node prototypes/orbit/flow.test.cjs` dalla radice del progetto. Il test usa casualità deterministica e verifica 200 frasi, blocchi, limiti, pause, lunghezze e variazione zero.
- Cinque timbri: Glass, Rubber, Mist, Soft Keys (armoniche morbide) e Bell (sintesi FM). Luminosità e volume generale restano indipendenti.
- Sub intonato: una sinusoide segue la nota due ottave sotto, con volume dedicato e attenuazione iniziale quando coincide con la cassa. Stop cancella anche sub e modulatore FM.
- Batteria elettronica sintetizzata: cassa sinusoidale con discesa di frequenza, clap e hi-hat da rumore filtrato. Nessun campione esterno. Cassa, clap e hi-hat si attivano separatamente; densità controlla colpi aggiuntivi, profondità regola frequenza finale e coda della cassa. Volume ritmo regola il kit; Volume generale regola synth e batteria insieme.
- Il ritmo usa una griglia di 16 ottavi con cassa e clap di riferimento e variazioni in Flusso. Le pause melodiche non interrompono la batteria. Cattura ferma anche l'evoluzione del pattern ritmico. Swing non ancora presente.

## Audio su iPhone

Al tocco di Ascolta Orbit richiede `navigator.audioSession.type = "playback"`,
se disponibile, prima di creare o riprendere il contesto audio. Questo evita
che Safari tratti la musica come audio ambientale soggetto al silenzioso.
Nei browser senza Audio Session API può essere necessario disattivare la modalità
silenziosa; controlla anche il volume multimediale e l'uscita Bluetooth/AirPlay.
La pagina segnala un contesto che non riesce ad avviarsi. La verifica dell'audio
sull'altoparlante di un iPhone fisico resta distinta dai test browser desktop.

## Effetti, mixer e file

- Delay a tempo: 1/8, 1/8 puntato, 1/4; quantità e feedback (massimo 75%).
- Riverbero stereo sintetico con dimensione stanza. Le quantità a zero disattivano gli effetti; Stop cancella le code.
- Volumi separati di synth, sub e batteria; volume generale dopo gli effetti.
- **Salva preset / Apri preset**: file JSON versionato con note, blocchi, frase (pause e durate incluse), pattern batteria e controlli. Il caricamento valida il file e lascia il trasporto fermo. Non salva il punto di riproduzione né il futuro del generatore casuale. I file `.orbit` nativi non sono intercambiabili con i JSON web.
- **Registra WAV**: avvia l'ascolto se necessario e cattura il mix stereo finale, PCM 16 bit alla frequenza del browser. **Termina registrazione** o **Ferma** prepara il link **Scarica WAV**. Limite cinque minuti; scarica il file prima di chiudere la pagina. Nessun caricamento su server. Cambiare scheda ferma ascolto e registrazione.

Test aggiuntivi: `node prototypes/orbit/session.test.cjs` (preset, WAV, stereo, clipping, limite e stop).

## Pubblicazione

Sito: https://orbit.avalla.dev, Cloudflare Workers Static Assets, Worker `luma-orbit`.
La configurazione è `../../wrangler.orbit.jsonc`. Dalla radice:

```sh
node prototypes/orbit/flow.test.cjs
node prototypes/orbit/clock.test.cjs
python3 scripts/prepare_orbit_web.py
npx wrangler@4.158.0 deploy --config wrangler.orbit.jsonc
```

Lo staging copia solo i nove file pubblici in `build/orbit-web`; test, documentazione e plugin nativi non vengono caricati. L'autenticazione resta nella configurazione locale di Wrangler.

Il prototipo è distinto dai plugin Luma e Luma RMS esistenti: usa Web Audio, non il motore JUCE. Non è ancora un VST3/AU, non riceve MIDI né sincronizzazione DAW ; i preset JSON conservano le sequenze dopo la chiusura. Il sequencer pianifica note e batteria su AudioContext.currentTime con 100 ms di anticipo; un timer di 25 ms alimenta la coda senza determinare gli istanti di attacco. I cursori seguono il tempo di uscita audio tramite getOutputTimestamp, quando disponibile. Le modifiche ai parametri si applicano ai prossimi eventi non ancora programmati. Blocchi del thread principale oltre la finestra di anticipo possono interrompere il ritmo: il clock riparte senza raffiche di eventi arretrati. Cambiare scheda ferma l'audio.

Direzione visiva sviluppata con la skill Impeccable: essenziale e precisa, sperimentale e giocosa, testi operativi da 16 px. Contesto e scelte in `../../PRODUCT.md` e `../../DESIGN.md`.

Verifica eseguita in Chromium: avvio/stop del contesto audio e delle voci, blocco delle note durante le variazioni, cattura e annullamento; nessun overflow orizzontale a 1360, 850 e 390 px. L'ascolto soggettivo e l'integrazione nativa restano fasi successive.

Verifica clock: oltre 500 attacchi con polling irregolare, cambio tempo, recupero da stallo, stop e riavvio. In Chromium verificati attacchi futuri senza deriva con blocchi JS di 35 ms, cambio BPM, cursore, cancellazione delle voci future e stop senza eventi UI residui.
