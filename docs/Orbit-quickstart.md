# Orbit 0.5 · guida rapida

## Lingua e suono pieno · 0.5

Il selettore Lingua offre italiano, inglese, francese, tedesco e spagnolo. La scelta
si conserva sul dispositivo e può cambiare durante l'ascolto. È una preferenza
dell'interfaccia, esclusa dai preset; nomi tecnici dei parametri host e finestre
di sistema seguono le rispettive convenzioni.

**Corpo** aggiunge al synth due sinusoidi scordate di ±7 cent e una componente
un'ottava sotto, con guadagno normalizzato. Parti dal 35% e prova 35–60% per dare
più consistenza alle note. Sub e batteria conservano i propri volumi indipendenti.
Le nuove sessioni partono dal 35%; i vecchi preset vengono caricati con Corpo a
zero per conservarne il suono: alza il controllo per usare la nuova sonorità.

Guide: [Italiano](https://github.com/avalla/luma-orbit/blob/main/docs/Orbit-quickstart.md) ·
[English](https://github.com/avalla/luma-orbit/blob/main/docs/en/README.md) ·
[Français](https://github.com/avalla/luma-orbit/blob/main/docs/fr/README.md) ·
[Deutsch](https://github.com/avalla/luma-orbit/blob/main/docs/de/README.md) ·
[Español](https://github.com/avalla/luma-orbit/blob/main/docs/es/README.md).


[Apri la versione web](https://orbit.avalla.dev) oppure
[scarica VST3, Audio Unit e standalone per macOS](https://github.com/avalla/luma-orbit/releases/latest/download/Luma-Orbit-macOS-Universal.zip).
Sul web ricarica la pagina se non compare **ORBIT 0.5**. Per installare il plugin
segui la [guida nativa](Luma-Orbit.md#installazione-macos).

## Dalla prima frase alle scene

1. Premi **Ascolta**. Sul web cambia il numero **BPM**; nel nativo usa **Tempo interno**.
   Con **Sync DAW** attivo e un trasporto host disponibile, cambia il tempo nella DAW.
2. Scegli scala e timbro, modifica le otto note e usa **Fisso** per proteggere i gradi
   melodici preferiti. Regola separatamente synth, sub e batteria.
3. Lascia **Flusso** attivo per generare variazioni. **Cattura** sul web o
   **Cattura la frase** nel nativo conserva note, pause, durate e pattern batteria.
   Per una ripetizione senza salti casuali lascia Probabilità al massimo.
4. Apri **Effetti e scene** nel plugin, oppure scorri a **Composizione** sul web.
   Premi **Salva qui** sotto Intro. Modifica il suono e salva un'altra idea in Groove.
5. Premi il nome della scena per richiamarla. Durante l'ascolto il cambio è in coda
   per la prossima battuta di quattro quarti; da fermo è immediato.
6. Premi **Salva preset**: il file JSON conserva l'idea corrente e tutti e quattro
   gli slot. Gli slot vuoti sono disabilitati finché non vi salvi un'idea.

Il richiamo delle scene usa battute di 4/4, anche se la DAW ha un'altra indicazione
di tempo. Un preset non conserva il punto esatto della riproduzione.

## Controlli di movimento

| Controllo | Web | Plugin | Effetto |
| --- | --- | --- | --- |
| Swing | 0–60% | 0–0.60 | Ritarda il secondo ottavo di ogni coppia senza allungare la battuta. |
| Probabilità | 0–100% | 0–1 | Decide se suonare ogni evento e colpo; le note fisse ignorano il sorteggio, ma rispettano le pause. |
| Energia | 0–100%, neutra 50% | 0–1, neutra 0.50 | Modula luminosità, esplorazione automatica e densità, senza riscrivere i controlli o cambiare il master. |
| Fill | Pulsante | Pulsante | Arma una variazione di batteria per una battuta, rispettando voci attive e probabilità. |

## Quale file salvare?

| Formato | Contiene | Uso |
| --- | --- | --- |
| Preset JSON | Controlli, note, frase, batteria e scene | Riprendere il lavoro in Orbit web o nativo. Non contiene audio. |
| MIDI | Frase corrente, sub e batteria GM; tempo, durate, pause e swing | Arrangiare le note nella DAW e assegnare altri strumenti. Esclude probabilità, Fill ed effetti. |
| WAV del mix | Esecuzione finale con effetti e volume generale | Conservare o condividere quello che suona. |
| Tre WAV separati | Synth, sub e batteria, simultanei e asciutti | Mixare le parti nella DAW, allineandole allo stesso punto iniziale. |

Le registrazioni WAV sono PCM 16 bit e durano al massimo cinque minuti. Le tracce
separate sono dopo i volumi degli strumenti ma prima di effetti e master; nel nativo
precedono anche la saturazione del mix. La loro somma non ricrea necessariamente il mix finale.

Sul web **Ferma** termina anche la registrazione: scarica i file prima di ricaricare
o chiudere la pagina. Nel nativo Stop musicale può continuare a registrare silenzio:
termina la registrazione con il relativo pulsante. Per i tre file nativi scegli un
nome base nuovo; i file già esistenti non vengono sovrascritti.

## Riprendere un progetto precedente

- Apri direttamente i vecchi JSON web in Orbit 0.5.
- Per convertire un vecchio `.orbit` binario, aprilo nel plugin e risalvalo come JSON;
  poi apri quel JSON sul web.
- Swing parte da zero, Probabilità dal massimo ed Energia dal punto neutro.
- Web e plugin usano motori diversi: condividono il materiale musicale, non un suono
  identico campione per campione. Annulla è disponibile solo sul web e non si salva nel preset.

## Se non senti audio

Controlla volume generale, volumi delle parti, Probabilità e trasporto. Nella DAW
avvia Play oppure disattiva Sync DAW per usare Ascolta. Su iPhone verifica volume
multimediale, uscita Bluetooth/AirPlay e, se necessario, modalità silenziosa;
prova ad aprire direttamente la pagina in Safari e premi di nuovo Ascolta.

Dettagli: [guida web](../prototypes/orbit/README.md), [guida nativa](Luma-Orbit.md),
[changelog](../CHANGELOG.md).
