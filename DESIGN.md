# Luma Orbit: direzione UI

## Scena d'uso

Un musicista compone la sera in uno studio con luce morbida e osserva a lungo una sequenza: superfici antracite calde riducono l'abbagliamento, testi color gesso restano grandi e leggibili.

## Colore

Strategia restrained: neutri caldi, verde lime per note e azioni, albicocca per accenti. Colori definiti in OKLCH. Il colore distingue ruoli, non sostituisce etichette, numeri o stato.

## Tipografia

Font di sistema; testo operativo 16–18 px, titoli 24–36 px. Numeri tabulari. Nessun asset remoto.

## Struttura

Toolbar con trasporto e tempo. Grande area orbitale a sinistra; controlli Esplora e timbro a destra. Editor dei passi lineare sotto le orbite per un'alternativa precisa e accessibile. Nessuna griglia di card decorative.

## Organizzazione delle funzioni in 0.4

Sul web, dopo sequenza e suono, le sezioni ospitano batteria, effetti, file e composizione.
I quattro slot scena hanno comandi separati per salvare e richiamare; uno slot vuoto
resta disabilitato. Su schermi stretti gli slot sono su due colonne e i controlli su una.

Nel plugin, **Effetti e scene** apre una pagina dedicata mantenendo accessibili trasporto,
note, blocchi e comandi dei file. **Torna al suono** ripristina il pannello principale.
Il valore neutro di Energia è esplicito: 50% sul web, 0.50 nel nativo.

Distinguere sempre preset (stato musicale), MIDI (note da arrangiare), WAV del mix
(con effetti) e tracce separate (asciutte). Mostrare l'attesa del cambio scena e
lo stato della registrazione senza affidarsi soltanto al colore.

## Componenti e movimento

Pulsanti alti almeno 44 px, selezione esplicita, focus ad alto contrasto, range nativi. Le orbite sono un diagramma del sequencer: si illumina solo il passo corrente. Nessuna rotazione decorativa. Layout su una colonna sotto 850 px. Cattura e annulla disabilitati quando non applicabili.

## Orbit 0.5

Cinque lingue con selettore leggibile e preferenza locale. Corpo arricchisce il synth con due sinusoidi a ±7 cent e una all’ottava inferiore, normalizzando il guadagno. Default 35%, preset precedenti 0%. Nessuna preferenza di lingua entra nello stato musicale.
